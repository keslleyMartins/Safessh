#pragma once
#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QJsonObject>

struct VaultEntry {
    QString id;
    QString service;   // e.g. connection name
    QString username;
    QString password;
    QString keyPassphrase;

    QJsonObject toJson() const;
    static VaultEntry fromJson(const QJsonObject& obj);
};

class Vault : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    bool isUnlocked() const { return m_unlocked; }

    bool create(const QString& filePath, const QString& masterPassword);
    bool unlock(const QString& filePath, const QString& masterPassword);
    void lock();

    void upsert(const VaultEntry& entry);
    bool remove(const QString& id);
    VaultEntry* find(const QString& id);
    const QHash<QString, VaultEntry>& entries() const { return m_entries; }

signals:
    void vaultLocked();
    void vaultUnlocked();
    void vaultModified();

private:
    bool m_unlocked = false;
    QString m_filePath;
    QByteArray m_key;
    QByteArray m_salt;
    QHash<QString, VaultEntry> m_entries;

    bool save();
    bool load(const QString& masterPassword);
    QByteArray encrypt(const QJsonObject& obj) const;
    QJsonObject decrypt(const QByteArray& ciphertext) const;
};
