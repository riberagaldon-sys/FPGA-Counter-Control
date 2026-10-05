#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

#include <optional>

struct DeviceStatus
{
    quint32 count = 0;
    bool running = true;
    bool countDown = false;
    bool ledRight = true;
    int frequencyIndex = 2;
    quint32 divider = 100000000U;
    quint16 ledMask = 0x001U;
    quint8 actionCode = 0;
    bool linkOnline = false;
    quint16 eventCounter = 0;
    QByteArray rawLine;

    double frequencyHz() const;
    QString actionText() const;
    QString lcdLine1() const;
    QString lcdLine2() const;
};

namespace GxProtocol
{
std::optional<DeviceStatus> parseStatusLine(const QByteArray &line,
                                            QString *errorMessage = nullptr);

QByteArray pingCommand();
QByteArray pauseToggleCommand();
QByteArray incrementCommand();
QByteArray decrementCommand();
QByteArray ledDirectionCommand();
QByteArray clearCommand();
QByteArray frequencyCommand();
QByteArray queryCommand();
}
