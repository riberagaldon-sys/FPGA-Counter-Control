#pragma once

#include "protocol.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QSerialPort>

#include <array>

class QCloseEvent;
class QComboBox;
class QFrame;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class QToolButton;

class LedBar;
class SevenSegmentDisplay;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void loadDemoState();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void refreshPorts();
    void toggleConnection();
    void handleReadyRead();
    void handleSerialError(QSerialPort::SerialPortError error);
    void sendPauseToggle();
    void sendIncrement();
    void sendDecrement();
    void sendLedDirection();
    void sendClear();
    void sendFrequency();
    void sendPing();
    void checkWatchdog();

private:
    void buildUi();
    void applyStatus(const DeviceStatus &status);
    void updateConnectionVisual();
    void appendLog(const QString &message, const QString &kind = QString());
    bool writeCommand(const QByteArray &command, const QString &description,
                      bool showInLog);
    QString selectedPortName() const;
    static QLabel *makeValueLabel(QWidget *parent = nullptr);

    QSerialPort *m_serial = nullptr;
    QTimer *m_pingTimer = nullptr;
    QTimer *m_watchdogTimer = nullptr;

    QComboBox *m_portCombo = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_connectButton = nullptr;
    QLabel *m_connectionBadge = nullptr;

    SevenSegmentDisplay *m_sevenSegment = nullptr;
    LedBar *m_ledBar = nullptr;
    QLabel *m_lcdLine1 = nullptr;
    QLabel *m_lcdLine2 = nullptr;
    QPlainTextEdit *m_log = nullptr;

    std::array<QToolButton *, 10> m_fButtons{};
    std::array<QFrame *, 6> m_functionCards{};
    std::array<QLabel *, 6> m_functionValues{};

    QLabel *m_portValue = nullptr;
    QLabel *m_portOpenValue = nullptr;
    QLabel *m_fpgaLinkValue = nullptr;
    QLabel *m_uartConfigValue = nullptr;
    QLabel *m_clockValue = nullptr;
    QLabel *m_frequencyValue = nullptr;
    QLabel *m_dividerValue = nullptr;
    QLabel *m_frequencyLevelValue = nullptr;
    QLabel *m_countHexValue = nullptr;
    QLabel *m_countDecimalValue = nullptr;
    QLabel *m_runningValue = nullptr;
    QLabel *m_countDirectionValue = nullptr;
    QLabel *m_ledMaskValue = nullptr;
    QLabel *m_activeLedValue = nullptr;
    QLabel *m_ledDirectionValue = nullptr;
    QLabel *m_actionValue = nullptr;
    QLabel *m_eventCounterValue = nullptr;
    QLabel *m_rxPacketsValue = nullptr;
    QLabel *m_txCommandsValue = nullptr;
    QLabel *m_lastUpdateValue = nullptr;

    QByteArray m_receiveBuffer;
    DeviceStatus m_status;
    QElapsedTimer m_lastStatusTimer;
    quint64 m_rxPackets = 0;
    quint64 m_txCommands = 0;
    bool m_haveStatus = false;
    bool m_protocolAlive = false;
    bool m_demoMode = false;
};
