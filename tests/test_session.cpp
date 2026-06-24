#include <QTest>
#include <core/ConnectionConfig.h>
#include <core/SessionManager.h>

class TestSession : public QObject {
    Q_OBJECT

private slots:
    void testConfigJsonRoundtrip()
    {
        ConnectionConfig c;
        c.name     = "work-server";
        c.protocol = ProtocolType::SSH;
        c.host     = "192.168.1.100";
        c.port     = 2222;
        c.username = "admin";
        c.group    = "Production";
        c.keepAlive = true;

        auto json = c.toJson();
        ConnectionConfig c2 = ConnectionConfig::fromJson(json);

        QCOMPARE(c2.name,       c.name);
        QCOMPARE(c2.protocol,   c.protocol);
        QCOMPARE(c2.host,       c.host);
        QCOMPARE(c2.port,       c.port);
        QCOMPARE(c2.username,   c.username);
        QCOMPARE(c2.group,      c.group);
        QCOMPARE(c2.keepAlive,  c.keepAlive);
    }

    void testSessionManagerAddRemove()
    {
        SessionManager mgr;

        ConnectionConfig c;
        c.name = "test-box";
        c.host = "10.0.0.1";

        mgr.addConnection(c);
        QCOMPARE(mgr.connections().size(), 1);

        auto* found = mgr.findConnection("test-box");
        QVERIFY(found != nullptr);
        QCOMPARE(found->host, "10.0.0.1");

        mgr.removeConnection("test-box");
        QCOMPARE(mgr.connections().size(), 0);
    }

    void testSessionManagerSaveLoad()
    {
        SessionManager mgr;

        ConnectionConfig c;
        c.name = "persist-test";
        c.host = "example.com";
        c.port = 22;
        mgr.addConnection(c);

        QTemporaryFile tmp;
        QVERIFY(tmp.open());

        mgr.saveConnections(tmp.fileName());

        SessionManager mgr2;
        mgr2.loadConnections(tmp.fileName());
        QCOMPARE(mgr2.connections().size(), 1);
        QCOMPARE(mgr2.connections()[0].host, "example.com");
    }
};

QTEST_MAIN(TestSession)
#include "test_session.moc"
