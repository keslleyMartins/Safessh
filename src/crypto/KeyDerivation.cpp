#include "KeyDerivation.h"
#include <sodium.h>
#include <cstring>
#include <spdlog/spdlog.h>

QByteArray deriveKey(const QString& password, const QByteArray& salt,
                     int opsLimit, int memLimit)
{
    if (sodium_init() < 0) {
        spdlog::error("libsodium initialization failed");
        return {};
    }

    const auto pw = password.toUtf8();
    QByteArray key(crypto_secretstream_xchacha20poly1305_KEYBYTES, '\0');

    if (crypto_pwhash(
            reinterpret_cast<unsigned char*>(key.data()),
            key.size(),
            pw.constData(), pw.size(),
            reinterpret_cast<const unsigned char*>(salt.constData()),
            opsLimit,
            memLimit,
            crypto_pwhash_ALG_ARGON2ID13) != 0)
    {
        spdlog::error("Argon2id key derivation failed");
        return {};
    }

    return key;
}
