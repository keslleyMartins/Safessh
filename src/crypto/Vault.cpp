#include "Vault.h"
#include "KeyDerivation.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <sodium.h>
#include <spdlog/spdlog.h>

// ─── VaultEntry ──────────────────────────────────────────────────────────────

QJsonObject VaultEntry::toJson() const
{
    return {
        {"id",            id},
        {"service",       service},
        {"username",      username},
        {"password",      password},
        {"keyPassphrase", keyPassphrase},
    };
}

VaultEntry VaultEntry::fromJson(const QJsonObject& obj)
{
    return {
        .id            = obj["id"].toString(),
        .service       = obj["service"].toString(),
        .username      = obj["username"].toString(),
        .password      = obj["password"].toString(),
        .keyPassphrase = obj["keyPassphrase"].toString(),
    };
}

// ─── Vault ───────────────────────────────────────────────────────────────────

bool Vault::create(const QString& filePath, const QString& masterPassword)
{
    m_filePath = filePath;
    m_salt.resize(crypto_pwhash_SALTBYTES);
    randombytes_buf(m_salt.data(), m_salt.size());

    m_key = deriveKey(masterPassword, m_salt);
    if (m_key.isEmpty()) return false;

    m_entries.clear();
    m_unlocked = true;
    bool ok = save();
    if (ok) emit vaultUnlocked();
    return ok;
}

bool Vault::unlock(const QString& filePath, const QString& masterPassword)
{
    return load(masterPassword);
}

void Vault::lock()
{
    m_unlocked = false;
    m_key.clear();
    m_salt.clear();
    m_entries.clear();
    emit vaultLocked();
}

void Vault::upsert(const VaultEntry& entry)
{
    if (!m_unlocked) return;
    m_entries[entry.id] = entry;
    save();
    emit vaultModified();
}

bool Vault::remove(const QString& id)
{
    if (!m_unlocked) return false;
    if (!m_entries.remove(id)) return false;
    save();
    emit vaultModified();
    return true;
}

VaultEntry* Vault::find(const QString& id)
{
    if (!m_unlocked) return nullptr;
    auto it = m_entries.find(id);
    return it != m_entries.end() ? &it.value() : nullptr;
}

// ─── Private ─────────────────────────────────────────────────────────────────

bool Vault::save()
{
    if (!m_unlocked) return false;

    QJsonArray arr;
    for (const auto& e : m_entries) {
        arr.append(e.toJson());
    }
    QJsonObject outer;
    outer["salt"] = QString::fromLatin1(m_salt.toBase64());
    outer["entries"] = arr;

    auto ciphertext = encrypt(outer);
    if (ciphertext.isEmpty()) return false;

    QFile f(m_filePath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(ciphertext.toBase64());
    return true;
}

bool Vault::load(const QString& masterPassword)
{
    QFile f(m_filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;

    auto ciphertext = QByteArray::fromBase64(f.readAll());
    f.close();

    auto outer = decrypt(ciphertext);
    if (outer.isEmpty()) return false;

    m_salt = QByteArray::fromBase64(outer["salt"].toString().toLatin1());
    m_key  = deriveKey(masterPassword, m_salt);
    if (m_key.isEmpty()) return false;

    m_entries.clear();
    for (const auto& val : outer["entries"].toArray()) {
        auto e = VaultEntry::fromJson(val.toObject());
        m_entries[e.id] = e;
    }

    m_filePath = m_filePath;
    m_unlocked = true;
    emit vaultUnlocked();
    return true;
}

QByteArray Vault::encrypt(const QJsonObject& obj) const
{
    const auto plaintext = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    const auto plainLen  = static_cast<unsigned long long>(plaintext.size());
    const size_t outLen  = crypto_secretstream_xchacha20poly1305_HEADERBYTES
                         + crypto_secretstream_xchacha20poly1305_ABYTES + plainLen;

    QByteArray out(static_cast<int>(outLen), '\0');

    crypto_secretstream_xchacha20poly1305_state state;
    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];

    crypto_secretstream_xchacha20poly1305_init_push(
        &state, header,
        reinterpret_cast<const unsigned char*>(m_key.constData()));

    unsigned long long cipherLen = 0;
    crypto_secretstream_xchacha20poly1305_push(
        &state,
        reinterpret_cast<unsigned char*>(out.data()) + sizeof(header),
        &cipherLen,
        reinterpret_cast<const unsigned char*>(plaintext.constData()),
        plainLen,
        nullptr, 0,
        crypto_secretstream_xchacha20poly1305_TAG_FINAL);

    std::memcpy(out.data(), header, sizeof(header));
    out.resize(static_cast<int>(sizeof(header) + cipherLen));
    return out;
}

QJsonObject Vault::decrypt(const QByteArray& ciphertext) const
{
    if (ciphertext.size() < static_cast<int>(crypto_secretstream_xchacha20poly1305_HEADERBYTES))
        return {};

    crypto_secretstream_xchacha20poly1305_state state;
    const auto* header = reinterpret_cast<const unsigned char*>(ciphertext.constData());

    if (crypto_secretstream_xchacha20poly1305_init_pull(
            &state, header,
            reinterpret_cast<const unsigned char*>(m_key.constData())) != 0)
        return {};

    const size_t bodyLen = ciphertext.size() - crypto_secretstream_xchacha20poly1305_HEADERBYTES;
    QByteArray plain(static_cast<int>(bodyLen), '\0');

    unsigned long long plainLen = 0;
    unsigned char tag = 0;

    if (crypto_secretstream_xchacha20poly1305_pull(
            &state,
            reinterpret_cast<unsigned char*>(plain.data()),
            &plainLen, &tag,
            reinterpret_cast<const unsigned char*>(ciphertext.constData())
                + crypto_secretstream_xchacha20poly1305_HEADERBYTES,
            bodyLen,
            nullptr, 0) != 0)
        return {};

    plain.resize(static_cast<int>(plainLen));
    return QJsonDocument::fromJson(plain).object();
}
