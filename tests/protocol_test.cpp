#include <QtTest>

#include "protocol.h"

class ProtocolTest final : public QObject
{
    Q_OBJECT

private slots:
    void parsesValidPacket()
    {
        QString error;
        const auto status = GxProtocol::parseStatusLine(
            "S,12AB34CD,1,U,R,2,05F5E100,020,04,1,003A\r\n", &error);
        QVERIFY2(status.has_value(), qPrintable(error));
        QCOMPARE(status->count, quint32(0x12AB34CD));
        QVERIFY(status->running);
        QVERIFY(!status->countDown);
        QVERIFY(status->ledRight);
        QCOMPARE(status->frequencyIndex, 2);
        QCOMPARE(status->divider, quint32(100000000));
        QCOMPARE(status->ledMask, quint16(0x020));
        QCOMPARE(status->actionCode, quint8(0x04));
        QVERIFY(status->linkOnline);
        QCOMPARE(status->eventCounter, quint16(0x003A));
        QCOMPARE(status->lcdLine1().size(), 16);
        QCOMPARE(status->lcdLine2(), QStringLiteral("INC     F:1.00Hz"));
    }

    void rejectsMalformedPacket()
    {
        QString error;
        const auto status = GxProtocol::parseStatusLine(
            "S,NOTHEX,1,U,R,2,05F5E100,001,00,1,0000", &error);
        QVERIFY(!status.has_value());
        QVERIFY(!error.isEmpty());
    }

    void encodesAllFunctionKeyCommands()
    {
        QCOMPARE(GxProtocol::pauseToggleCommand(), QByteArray("T\n"));
        QCOMPARE(GxProtocol::incrementCommand(), QByteArray("U\n"));
        QCOMPARE(GxProtocol::decrementCommand(), QByteArray("D\n"));
        QCOMPARE(GxProtocol::ledDirectionCommand(), QByteArray("L\n"));
        QCOMPARE(GxProtocol::clearCommand(), QByteArray("C\n"));
        QCOMPARE(GxProtocol::frequencyCommand(), QByteArray("F\n"));
    }
};

QTEST_APPLESS_MAIN(ProtocolTest)
#include "protocol_test.moc"
