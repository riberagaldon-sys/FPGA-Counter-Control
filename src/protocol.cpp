#include "protocol.h"

#include <QStringList>

namespace
{
constexpr double kFpgaClockHz = 100000000.0;

bool parseHex(const QByteArray &field, int exactLength, quint32 maximum, quint32 *value)
{
    if (field.size() != exactLength)
        return false;

    bool ok = false;
    const quint64 parsed = field.toULongLong(&ok, 16);
    if (!ok || parsed > maximum)
        return false;

    *value = static_cast<quint32>(parsed);
    return true;
}

bool parseFlag(const QByteArray &field, bool *value)
{
    if (field == "0") {
        *value = false;
        return true;
    }
    if (field == "1") {
        *value = true;
        return true;
    }
    return false;
}

QString actionAsciiField(quint8 code)
{
    switch (code) {
    case 0x01: return QStringLiteral("RUN     ");
    case 0x02: return QStringLiteral("PAUSE   ");
    case 0x03: return QStringLiteral("CLEAR   ");
    case 0x04: return QStringLiteral("INC     ");
    case 0x05: return QStringLiteral("DEC     ");
    case 0x06: return QStringLiteral("LED->   ");
    case 0x07: return QStringLiteral("LED<-   ");
    case 0x08: return QStringLiteral("FREQ    ");
    default:   return QStringLiteral("RUN     ");
    }
}

QString frequencyAsciiField(int index)
{
    switch (index) {
    case 0: return QStringLiteral("F:0.25Hz");
    case 1: return QStringLiteral("F:0.50Hz");
    case 2: return QStringLiteral("F:1.00Hz");
    case 3: return QStringLiteral("F:2.00Hz");
    default: return QStringLiteral("F:4.00Hz");
    }
}
}

double DeviceStatus::frequencyHz() const
{
    return divider == 0 ? 0.0 : kFpgaClockHz / static_cast<double>(divider);
}

QString DeviceStatus::actionText() const
{
    switch (actionCode) {
    case 0x01: return QStringLiteral("继续运行");
    case 0x02: return QStringLiteral("暂停");
    case 0x03: return QStringLiteral("清零");
    case 0x04: return QStringLiteral("设置递增");
    case 0x05: return QStringLiteral("设置递减");
    case 0x06: return QStringLiteral("LED 向右");
    case 0x07: return QStringLiteral("LED 向左");
    case 0x08: return QStringLiteral("切换频率");
    default: return QStringLiteral("上电初始化");
    }
}

QString DeviceStatus::lcdLine1() const
{
    return (linkOnline ? QStringLiteral("UART:ONLINE")
                       : QStringLiteral("UART:OFFLINE"))
        .leftJustified(16, QLatin1Char(' '), true);
}

QString DeviceStatus::lcdLine2() const
{
    return (actionAsciiField(actionCode) + frequencyAsciiField(frequencyIndex))
        .left(16)
        .leftJustified(16, QLatin1Char(' '));
}

std::optional<DeviceStatus> GxProtocol::parseStatusLine(const QByteArray &line,
                                                       QString *errorMessage)
{
    const QByteArray trimmed = line.trimmed();
    const QList<QByteArray> fields = trimmed.split(',');
    auto fail = [&](const QString &message) -> std::optional<DeviceStatus> {
        if (errorMessage)
            *errorMessage = message;
        return std::nullopt;
    };

    if (fields.size() != 11)
        return fail(QStringLiteral("字段数量应为 11，实际为 %1").arg(fields.size()));
    if (fields.at(0) != "S")
        return fail(QStringLiteral("不是 S 状态包"));

    DeviceStatus result;
    quint32 value = 0;

    if (!parseHex(fields.at(1), 8, 0xFFFFFFFFU, &result.count))
        return fail(QStringLiteral("计数字段无效"));
    if (!parseFlag(fields.at(2), &result.running))
        return fail(QStringLiteral("运行状态无效"));

    if (fields.at(3) == "D")
        result.countDown = true;
    else if (fields.at(3) == "U")
        result.countDown = false;
    else
        return fail(QStringLiteral("计数方向无效"));

    if (fields.at(4) == "R")
        result.ledRight = true;
    else if (fields.at(4) == "L")
        result.ledRight = false;
    else
        return fail(QStringLiteral("LED 方向无效"));

    bool indexOk = false;
    result.frequencyIndex = fields.at(5).toInt(&indexOk, 10);
    if (!indexOk || result.frequencyIndex < 0 || result.frequencyIndex > 4)
        return fail(QStringLiteral("频率档位无效"));

    if (!parseHex(fields.at(6), 8, 0xFFFFFFFFU, &result.divider)
        || result.divider == 0) {
        return fail(QStringLiteral("分频系数无效"));
    }

    if (!parseHex(fields.at(7), 3, 0x0FFFU, &value))
        return fail(QStringLiteral("LED 掩码无效"));
    result.ledMask = static_cast<quint16>(value);

    if (!parseHex(fields.at(8), 2, 0x00FFU, &value))
        return fail(QStringLiteral("动作码无效"));
    result.actionCode = static_cast<quint8>(value);

    if (!parseFlag(fields.at(9), &result.linkOnline))
        return fail(QStringLiteral("联机字段无效"));

    if (!parseHex(fields.at(10), 4, 0xFFFFU, &value))
        return fail(QStringLiteral("事件序号无效"));
    result.eventCounter = static_cast<quint16>(value);
    result.rawLine = trimmed;

    if (errorMessage)
        errorMessage->clear();
    return result;
}

QByteArray GxProtocol::pingCommand()         { return QByteArrayLiteral("P\n"); }
QByteArray GxProtocol::pauseToggleCommand()  { return QByteArrayLiteral("T\n"); }
QByteArray GxProtocol::incrementCommand()    { return QByteArrayLiteral("U\n"); }
QByteArray GxProtocol::decrementCommand()    { return QByteArrayLiteral("D\n"); }
QByteArray GxProtocol::ledDirectionCommand() { return QByteArrayLiteral("L\n"); }
QByteArray GxProtocol::clearCommand()        { return QByteArrayLiteral("C\n"); }
QByteArray GxProtocol::frequencyCommand()    { return QByteArrayLiteral("F\n"); }
QByteArray GxProtocol::queryCommand()        { return QByteArrayLiteral("Q\n"); }
