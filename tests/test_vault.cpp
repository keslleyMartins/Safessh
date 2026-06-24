#include <QTest>
#include <crypto/Vault.h>
#include <QTemporaryFile>

class TestVault : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // libsodium must be initialized before use
    }

    void testCreateAndUnlock()
    {
        QTemporaryFile tmp;
        QVERIFY(tmp.open());

        Vault v;
        QVERIFY(v.create(tmp.fileName(), "hunter2"));
        QVERIFY(v.isUnlocked());

        v.lock();
        QVERIFY(!v.isUnlocked());

        QVERIFY(v.unlock(tmp.fileName(), "hunter2"));
        QVERIFY(v.isUnlocked());
    }

    void testWrongPassword()
    {
        QTemporaryFile tmp;
        QVERIFY(tmp.open());

        Vault v;
        QVERIFY(v.create(tmp.fileName(), "correct"));
        v.lock();
        QVERIFY(!v.unlock(tmp.fileName(), "wrong"));
        QVERIFY(!v.isUnlocked());
    }

    void testRoundtrip()
    {
        QTemporaryFile tmp;
        QVERIFY(tmp.open());

        Vault v;
        QVERIFY(v.create(tmp.fileName(), "master"));

        VaultEntry e;
        e.id = "conn-1";
        e.service = "My Server";
        e.username = "root";
        e.password = "s3cret!";

        v.upsert(e);
        QCOMPARE(v.entries().size(), 1);

        v.lock();
        QVERIFY(v.unlock(tmp.fileName(), "master"));

        auto* found = v.find("conn-1");
        QVERIFY(found != nullptr);
        QCOMPARE(found->username, "root");
        QCOMPARE(found->password, "s3cret!");
    }
};

QTEST_MAIN(TestVault)
#include "test_vault.moc"
