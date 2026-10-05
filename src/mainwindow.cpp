#include "mainwindow.h"

#include "widgets/ledbar.h"
#include "widgets/sevensegmentdisplay.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSerialPortInfo>
#include <QShortcut>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
QString hexValue(quint32 value, int digits)
{
    return QStringLiteral("0x%1")
        .arg(value, digits, 16, QLatin1Char('0'))
        .toUpper();
}

void repolish(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_serial(new QSerialPort(this))
    , m_pingTimer(new QTimer(this))
    , m_watchdogTimer(new QTimer(this))
{
    buildUi();

    // Three heartbeats fit inside the FPGA's 300 ms unplug/crash watchdog.
    m_pingTimer->setInterval(100);
    m_watchdogTimer->setInterval(250);

    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::handleReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleSerialError);
    connect(m_pingTimer, &QTimer::timeout, this, &MainWindow::sendPing);
    connect(m_watchdogTimer, &QTimer::timeout, this, &MainWindow::checkWatchdog);
    connect(m_fButtons.at(0), &QToolButton::clicked, this, &MainWindow::sendPauseToggle);
    connect(m_fButtons.at(1), &QToolButton::clicked, this, &MainWindow::sendIncrement);
    connect(m_fButtons.at(2), &QToolButton::clicked, this, &MainWindow::sendDecrement);
    connect(m_fButtons.at(3), &QToolButton::clicked, this, &MainWindow::sendLedDirection);
    connect(m_fButtons.at(8), &QToolButton::clicked, this, &MainWindow::sendClear);
    connect(m_fButtons.at(9), &QToolButton::clicked, this, &MainWindow::sendFrequency);

    auto *pauseShortcut = new QShortcut(QKeySequence(Qt::Key_F1), this);
    auto *incrementShortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    auto *decrementShortcut = new QShortcut(QKeySequence(Qt::Key_F3), this);
    auto *ledDirectionShortcut = new QShortcut(QKeySequence(Qt::Key_F4), this);
    auto *clearShortcut = new QShortcut(QKeySequence(Qt::Key_F9), this);
    auto *frequencyShortcut = new QShortcut(QKeySequence(Qt::Key_F10), this);
    connect(pauseShortcut, &QShortcut::activated, this, &MainWindow::sendPauseToggle);
    connect(incrementShortcut, &QShortcut::activated, this, &MainWindow::sendIncrement);
    connect(decrementShortcut, &QShortcut::activated, this, &MainWindow::sendDecrement);
    connect(ledDirectionShortcut, &QShortcut::activated, this, &MainWindow::sendLedDirection);
    connect(clearShortcut, &QShortcut::activated, this, &MainWindow::sendClear);
    connect(frequencyShortcut, &QShortcut::activated, this, &MainWindow::sendFrequency);

    refreshPorts();
    applyStatus(m_status);
    updateConnectionVisual();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("GX 数码管计数器与 LED 控制终端"));
    resize(1640, 1000);
    setMinimumSize(1240, 780);

    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("page"));
    page->setMinimumWidth(1160);
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(22, 18, 22, 22);
    pageLayout->setSpacing(14);

    auto *titleRow = new QHBoxLayout;
    auto *titleColumn = new QVBoxLayout;
    auto *title = new QLabel(QStringLiteral("GX 数码管计数器 / LED 联动终端"));
    title->setObjectName(QStringLiteral("title"));
    auto *subtitle = new QLabel(QStringLiteral("XC7A200T · 8 位十六进制显示 · 12 路 LED · LCD1602 · UART 115200 8N1"));
    subtitle->setObjectName(QStringLiteral("subtitle"));
    titleColumn->addWidget(title);
    titleColumn->addWidget(subtitle);
    titleRow->addLayout(titleColumn, 1);
    m_connectionBadge = new QLabel(QStringLiteral("未连接"));
    m_connectionBadge->setObjectName(QStringLiteral("connectionBadge"));
    m_connectionBadge->setProperty("state", "offline");
    m_connectionBadge->setAlignment(Qt::AlignCenter);
    m_connectionBadge->setMinimumSize(190, 48);
    titleRow->addWidget(m_connectionBadge);
    pageLayout->addLayout(titleRow);

    auto *connectionBox = new QGroupBox(QStringLiteral("串口连接"));
    auto *connectionLayout = new QHBoxLayout(connectionBox);
    connectionLayout->setContentsMargins(16, 14, 16, 14);
    connectionLayout->setSpacing(12);
    connectionLayout->addWidget(new QLabel(QStringLiteral("端口")));
    m_portCombo = new QComboBox;
    m_portCombo->setEditable(true);
    m_portCombo->setMinimumWidth(330);
    m_portCombo->setMinimumHeight(42);
    connectionLayout->addWidget(m_portCombo, 1);
    m_refreshButton = new QPushButton(QStringLiteral("刷新端口"));
    m_refreshButton->setMinimumSize(118, 42);
    connectionLayout->addWidget(m_refreshButton);
    auto *uartFixed = new QLabel(QStringLiteral("115200 · 8N1 · 无流控"));
    uartFixed->setObjectName(QStringLiteral("uartPill"));
    uartFixed->setAlignment(Qt::AlignCenter);
    uartFixed->setMinimumSize(220, 42);
    connectionLayout->addWidget(uartFixed);
    m_connectButton = new QPushButton(QStringLiteral("连接"));
    m_connectButton->setObjectName(QStringLiteral("connectButton"));
    m_connectButton->setMinimumSize(140, 46);
    connectionLayout->addWidget(m_connectButton);
    pageLayout->addWidget(connectionBox);

    auto *segmentBox = new QGroupBox(QStringLiteral("8 位十六进制数码管（与板上 DIG1–DIG8 同步）"));
    auto *segmentLayout = new QVBoxLayout(segmentBox);
    segmentLayout->setContentsMargins(14, 14, 14, 14);
    m_sevenSegment = new SevenSegmentDisplay;
    segmentLayout->addWidget(m_sevenSegment);
    pageLayout->addWidget(segmentBox);

    auto *ledBox = new QGroupBox(QStringLiteral("12 路 LED 流水状态（与板上 LED1–LED12 同步）"));
    auto *ledLayout = new QVBoxLayout(ledBox);
    ledLayout->setContentsMargins(14, 10, 14, 10);
    m_ledBar = new LedBar;
    ledLayout->addWidget(m_ledBar);
    pageLayout->addWidget(ledBox);

    auto *functionKeyBox = new QGroupBox(QStringLiteral("Qt 终端按键：F1–F4 / F9–F10 可操作，F5–F8 禁用"));
    auto *functionKeyLayout = new QHBoxLayout(functionKeyBox);
    functionKeyLayout->setContentsMargins(16, 14, 16, 14);
    functionKeyLayout->setSpacing(10);

    const QStringList activeFunctionLabels = {
        QStringLiteral("F1\n暂停 / 继续"),
        QStringLiteral("F2\n递增"),
        QStringLiteral("F3\n递减"),
        QStringLiteral("F4\nLED 方向")
    };
    const QStringList activeFunctionRoles = {
        QStringLiteral("pause"), QStringLiteral("increment"),
        QStringLiteral("decrement"), QStringLiteral("ledDirection")
    };
    const QStringList activeFunctionTips = {
        QStringLiteral("对应核心板 KEY0；发送 T\\n；键盘 F1 也可触发"),
        QStringLiteral("对应核心板 KEY1；发送 U\\n；键盘 F2 也可触发"),
        QStringLiteral("对应核心板 KEY2；发送 D\\n；键盘 F3 也可触发"),
        QStringLiteral("对应核心板 KEY3；发送 L\\n；键盘 F4 也可触发")
    };
    for (int i = 0; i < 4; ++i) {
        auto *button = new QToolButton;
        button->setText(activeFunctionLabels.at(i));
        button->setFixedSize(125, 78);
        button->setProperty("role", activeFunctionRoles.at(i));
        button->setToolTip(activeFunctionTips.at(i));
        m_fButtons.at(i) = button;
        functionKeyLayout->addWidget(button);
    }
    for (int i = 4; i < 8; ++i) {
        auto *button = new QToolButton;
        button->setText(QStringLiteral("F%1\n未使用").arg(i + 1));
        button->setEnabled(false);
        button->setFixedSize(100, 78);
        button->setProperty("role", "unused");
        m_fButtons.at(i) = button;
        functionKeyLayout->addWidget(button);
    }
    functionKeyLayout->addStretch(1);

    auto *activeKeys = new QVBoxLayout;
    activeKeys->setSpacing(8);
    for (int i = 8; i < 10; ++i) {
        auto *button = new QToolButton;
        button->setText(i == 8 ? QStringLiteral("F9  清零")
                               : QStringLiteral("F10  调频"));
        button->setFixedSize(150, 55);
        button->setProperty("role", i == 8 ? "clear" : "frequency");
        button->setToolTip(i == 8
            ? QStringLiteral("发送 C\\n；键盘 F9 也可触发")
            : QStringLiteral("发送 F\\n；键盘 F10 也可触发"));
        m_fButtons.at(i) = button;
        activeKeys->addWidget(button);
    }
    functionKeyLayout->addLayout(activeKeys);
    pageLayout->addWidget(functionKeyBox);

    auto *detailRow = new QHBoxLayout;
    detailRow->setSpacing(14);

    auto *hardwareBox = new QGroupBox(QStringLiteral("核心板六键功能状态"));
    auto *hardwareGrid = new QGridLayout(hardwareBox);
    hardwareGrid->setContentsMargins(14, 14, 14, 14);
    hardwareGrid->setHorizontalSpacing(10);
    hardwareGrid->setVerticalSpacing(10);
    const QStringList functionNames = {
        QStringLiteral("暂停 / 继续  ·  KEY0"),
        QStringLiteral("清零  ·  FPGA_nRST"),
        QStringLiteral("递增  ·  KEY1"),
        QStringLiteral("递减  ·  KEY2"),
        QStringLiteral("LED 方向  ·  KEY3"),
        QStringLiteral("调频  ·  KEY4")
    };
    for (int i = 0; i < 6; ++i) {
        auto *card = new QFrame;
        card->setObjectName(QStringLiteral("functionCard"));
        card->setProperty("active", false);
        card->setMinimumSize(255, 86);
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(13, 10, 13, 10);
        auto *name = new QLabel(functionNames.at(i));
        name->setObjectName(QStringLiteral("functionName"));
        auto *value = new QLabel(QStringLiteral("--"));
        value->setObjectName(QStringLiteral("functionValue"));
        cardLayout->addWidget(name);
        cardLayout->addWidget(value);
        m_functionCards.at(i) = card;
        m_functionValues.at(i) = value;
        hardwareGrid->addWidget(card, i / 2, i % 2);
    }
    detailRow->addWidget(hardwareBox, 4);

    auto *lcdBox = new QGroupBox(QStringLiteral("LCD1602 信息详细版"));
    auto *lcdLayout = new QVBoxLayout(lcdBox);
    lcdLayout->setContentsMargins(16, 14, 16, 14);
    lcdLayout->setSpacing(10);
    auto *lcdCaption = new QLabel(QStringLiteral("硬件 LCD1602 实时内容（每行 16 字符）"));
    lcdCaption->setObjectName(QStringLiteral("lcdCaption"));
    lcdLayout->addWidget(lcdCaption);
    m_lcdLine1 = new QLabel;
    m_lcdLine2 = new QLabel;
    for (QLabel *line : {m_lcdLine1, m_lcdLine2}) {
        line->setObjectName(QStringLiteral("lcdLine"));
        line->setAlignment(Qt::AlignCenter);
        line->setMinimumHeight(52);
        lcdLayout->addWidget(line);
    }

    auto *parameterGrid = new QGridLayout;
    parameterGrid->setHorizontalSpacing(13);
    parameterGrid->setVerticalSpacing(7);
    auto addParameter = [&](int row, int pair, const QString &name, QLabel **storage) {
        auto *nameLabel = new QLabel(name);
        nameLabel->setObjectName(QStringLiteral("parameterName"));
        auto *valueLabel = makeValueLabel();
        parameterGrid->addWidget(nameLabel, row, pair * 2);
        parameterGrid->addWidget(valueLabel, row, pair * 2 + 1);
        *storage = valueLabel;
    };

    addParameter(0, 0, QStringLiteral("串口端口"), &m_portValue);
    addParameter(0, 1, QStringLiteral("本地串口"), &m_portOpenValue);
    addParameter(1, 0, QStringLiteral("FPGA 心跳"), &m_fpgaLinkValue);
    addParameter(1, 1, QStringLiteral("串口参数"), &m_uartConfigValue);
    addParameter(2, 0, QStringLiteral("系统时钟"), &m_clockValue);
    addParameter(2, 1, QStringLiteral("计数频率"), &m_frequencyValue);
    addParameter(3, 0, QStringLiteral("分频系数"), &m_dividerValue);
    addParameter(3, 1, QStringLiteral("频率档位"), &m_frequencyLevelValue);
    addParameter(4, 0, QStringLiteral("计数 HEX"), &m_countHexValue);
    addParameter(4, 1, QStringLiteral("计数 DEC"), &m_countDecimalValue);
    addParameter(5, 0, QStringLiteral("运行状态"), &m_runningValue);
    addParameter(5, 1, QStringLiteral("计数方向"), &m_countDirectionValue);
    addParameter(6, 0, QStringLiteral("LED 掩码"), &m_ledMaskValue);
    addParameter(6, 1, QStringLiteral("当前亮灯"), &m_activeLedValue);
    addParameter(7, 0, QStringLiteral("LED 方向"), &m_ledDirectionValue);
    addParameter(7, 1, QStringLiteral("最近功能"), &m_actionValue);
    addParameter(8, 0, QStringLiteral("事件序号"), &m_eventCounterValue);
    addParameter(8, 1, QStringLiteral("接收包数"), &m_rxPacketsValue);
    addParameter(9, 0, QStringLiteral("发送命令"), &m_txCommandsValue);
    addParameter(9, 1, QStringLiteral("最后更新"), &m_lastUpdateValue);
    parameterGrid->setColumnStretch(1, 1);
    parameterGrid->setColumnStretch(3, 1);
    lcdLayout->addLayout(parameterGrid);
    detailRow->addWidget(lcdBox, 6);
    pageLayout->addLayout(detailRow);

    auto *logBox = new QGroupBox(QStringLiteral("通信日志"));
    auto *logLayout = new QVBoxLayout(logBox);
    logLayout->setContentsMargins(12, 10, 12, 12);
    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);
    m_log->setMinimumHeight(110);
    m_log->setMaximumHeight(160);
    m_log->setPlaceholderText(QStringLiteral("连接后显示命令、状态和错误信息……"));
    logLayout->addWidget(m_log);
    pageLayout->addWidget(logBox);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(page);
    setCentralWidget(scrollArea);

    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget#page { background: #EEF3F9; color: #172033; }
        QLabel#title { font-size: 28px; font-weight: 800; color: #102A43; }
        QLabel#subtitle { font-size: 14px; color: #52657A; padding-top: 2px; }
        QGroupBox {
            background: #FFFFFF; border: 1px solid #CAD5E2; border-radius: 12px;
            margin-top: 12px; padding-top: 10px; font-size: 16px; font-weight: 700;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 7px; color: #223B56; }
        QPushButton, QComboBox {
            font-size: 15px; border: 1px solid #AFC0D3; border-radius: 8px;
            background: #FFFFFF; padding: 7px 12px;
        }
        QComboBox {
            color: #000000;
            selection-color: #000000;
            selection-background-color: #DCEBFF;
        }
        QComboBox QLineEdit { color: #000000; background: #FFFFFF; }
        QComboBox QAbstractItemView {
            color: #000000; background: #FFFFFF;
            selection-color: #000000; selection-background-color: #DCEBFF;
        }
        QPushButton:hover { border-color: #2F80ED; background: #EFF6FF; }
        QPushButton#connectButton { background: #1769D2; color: white; font-weight: 800; border: 0; }
        QPushButton#connectButton:hover { background: #0F55B3; }
        QLabel#uartPill { background: #EDF4FC; color: #244B74; border-radius: 8px; font-weight: 700; }
        QLabel#connectionBadge { border-radius: 22px; font-size: 16px; font-weight: 800; padding: 7px 14px; }
        QLabel#connectionBadge[state="offline"] { background: #E2E8F0; color: #475569; }
        QLabel#connectionBadge[state="waiting"] { background: #FFF0C2; color: #8A5800; }
        QLabel#connectionBadge[state="online"] { background: #CFF7DD; color: #146C36; }
        QToolButton { font-size: 17px; font-weight: 800; border-radius: 10px; }
        QToolButton[role="unused"] { background: #E2E7ED; color: #8A96A5; border: 1px solid #C7D0DA; }
        QToolButton[role="pause"] { background: #D98B16; color: white; border: 0; }
        QToolButton[role="pause"]:hover { background: #B86F09; }
        QToolButton[role="increment"] { background: #168A58; color: white; border: 0; }
        QToolButton[role="increment"]:hover { background: #0F7046; }
        QToolButton[role="decrement"] { background: #C85D20; color: white; border: 0; }
        QToolButton[role="decrement"]:hover { background: #A84816; }
        QToolButton[role="ledDirection"] { background: #7357C7; color: white; border: 0; }
        QToolButton[role="ledDirection"]:hover { background: #5940A7; }
        QToolButton[role="clear"] { background: #E5484D; color: white; border: 0; }
        QToolButton[role="clear"]:hover { background: #C93439; }
        QToolButton[role="frequency"] { background: #186FD5; color: white; border: 0; }
        QToolButton[role="frequency"]:hover { background: #0D56AD; }
        QFrame#functionCard { background: #F5F8FC; border: 1px solid #D2DCE8; border-radius: 10px; }
        QFrame#functionCard[active="true"] { background: #E6F2FF; border: 2px solid #2583E3; }
        QLabel#functionName { color: #52657A; font-size: 13px; font-weight: 700; }
        QLabel#functionValue { color: #162E47; font-size: 17px; font-weight: 800; }
        QLabel#lcdCaption { color: #52657A; font-size: 13px; font-weight: 600; }
        QLabel#lcdLine {
            background: #102A1C; color: #9CFF92; border: 2px solid #385B45;
            border-radius: 6px; font-family: Consolas, "Courier New";
            font-size: 27px; font-weight: 700; letter-spacing: 2px;
        }
        QLabel#parameterName { color: #65778B; font-size: 12px; font-weight: 600; }
        QLabel#parameterValue {
            background: #F4F7FB; border-radius: 5px; color: #15395B;
            font-size: 14px; font-weight: 750; padding: 5px 8px;
        }
        QPlainTextEdit {
            background: #0D1520; color: #C9E2FF; border: 0; border-radius: 7px;
            font-family: Consolas, "Courier New"; font-size: 12px; padding: 6px;
        }
    )"));
}

QLabel *MainWindow::makeValueLabel(QWidget *parent)
{
    auto *label = new QLabel(QStringLiteral("--"), parent);
    label->setObjectName(QStringLiteral("parameterValue"));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setMinimumWidth(145);
    return label;
}

void MainWindow::refreshPorts()
{
    const QString previousPort = selectedPortName();
    m_portCombo->clear();

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        QString description = port.description().trimmed();
        if (description.isEmpty())
            description = QStringLiteral("串口设备");
        m_portCombo->addItem(QStringLiteral("%1  —  %2").arg(port.portName(), description),
                             port.portName());
    }

    const int previousIndex = m_portCombo->findData(previousPort);
    if (previousIndex >= 0)
        m_portCombo->setCurrentIndex(previousIndex);
    else if (!previousPort.isEmpty())
        m_portCombo->setEditText(previousPort);
    else if (ports.isEmpty())
        m_portCombo->setEditText(QStringLiteral("COM9"));
}

QString MainWindow::selectedPortName() const
{
    const QString stored = m_portCombo->currentData().toString().trimmed();
    if (!stored.isEmpty())
        return stored;
    return m_portCombo->currentText().trimmed();
}

void MainWindow::toggleConnection()
{
    if (m_serial->isOpen()) {
        m_pingTimer->stop();
        m_watchdogTimer->stop();
        // Notify the FPGA before closing so LCD1602 changes to OFFLINE now,
        // without waiting for the watchdog timeout.
        m_serial->write(QByteArrayLiteral("X\n"));
        m_serial->flush();
        m_serial->waitForBytesWritten(100);
        m_serial->close();
        m_protocolAlive = false;
        m_status.linkOnline = false;
        appendLog(QStringLiteral("串口已断开"), QStringLiteral("INFO"));
        applyStatus(m_status);
        updateConnectionVisual();
        return;
    }

    const QString portName = selectedPortName();
    if (portName.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择端口"),
                                 QStringLiteral("请先选择或输入 COM 端口。"));
        return;
    }

    m_serial->setPortName(portName);
    m_serial->setBaudRate(QSerialPort::Baud115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, QStringLiteral("串口连接失败"),
                              QStringLiteral("无法打开 %1：\n%2")
                                  .arg(portName, m_serial->errorString()));
        appendLog(QStringLiteral("打开 %1 失败：%2").arg(portName, m_serial->errorString()),
                  QStringLiteral("ERROR"));
        updateConnectionVisual();
        return;
    }

    m_demoMode = false;
    m_receiveBuffer.clear();
    m_protocolAlive = false;
    m_status.linkOnline = false;
    m_lastStatusTimer.invalidate();
    m_pingTimer->start();
    m_watchdogTimer->start();
    appendLog(QStringLiteral("已打开 %1，参数 115200 8N1").arg(portName),
              QStringLiteral("INFO"));
    updateConnectionVisual();
    writeCommand(GxProtocol::queryCommand(), QStringLiteral("查询状态 Q"), true);
    writeCommand(GxProtocol::pingCommand(), QStringLiteral("心跳 P"), false);
}

bool MainWindow::writeCommand(const QByteArray &command, const QString &description,
                              bool showInLog)
{
    if (!m_serial->isOpen()) {
        if (showInLog)
            appendLog(QStringLiteral("%1 未发送：串口未连接").arg(description),
                      QStringLiteral("WARN"));
        return false;
    }

    const qint64 written = m_serial->write(command);
    if (written != command.size()) {
        if (showInLog)
            appendLog(QStringLiteral("%1 发送失败：%2").arg(description, m_serial->errorString()),
                      QStringLiteral("ERROR"));
        return false;
    }

    ++m_txCommands;
    if (showInLog)
        appendLog(QStringLiteral(">> %1").arg(description), QStringLiteral("TX"));
    m_txCommandsValue->setText(QString::number(m_txCommands));
    return true;
}

void MainWindow::sendClear()
{
    if (!writeCommand(GxProtocol::clearCommand(), QStringLiteral("F9 清零 / C"), true)) {
        if (!m_demoMode)
            QMessageBox::information(this, QStringLiteral("尚未连接"),
                                     QStringLiteral("请先连接 FPGA 串口，再使用 F9 清零。"));
    }
}

void MainWindow::sendPauseToggle()
{
    if (!writeCommand(GxProtocol::pauseToggleCommand(),
                      QStringLiteral("F1 暂停/继续 / T"), true) && !m_demoMode) {
        QMessageBox::information(this, QStringLiteral("尚未连接"),
                                 QStringLiteral("请先连接 FPGA 串口，再使用 F1 暂停/继续。"));
    }
}

void MainWindow::sendIncrement()
{
    if (!writeCommand(GxProtocol::incrementCommand(),
                      QStringLiteral("F2 递增 / U"), true) && !m_demoMode) {
        QMessageBox::information(this, QStringLiteral("尚未连接"),
                                 QStringLiteral("请先连接 FPGA 串口，再使用 F2 递增。"));
    }
}

void MainWindow::sendDecrement()
{
    if (!writeCommand(GxProtocol::decrementCommand(),
                      QStringLiteral("F3 递减 / D"), true) && !m_demoMode) {
        QMessageBox::information(this, QStringLiteral("尚未连接"),
                                 QStringLiteral("请先连接 FPGA 串口，再使用 F3 递减。"));
    }
}

void MainWindow::sendLedDirection()
{
    if (!writeCommand(GxProtocol::ledDirectionCommand(),
                      QStringLiteral("F4 LED 方向 / L"), true) && !m_demoMode) {
        QMessageBox::information(this, QStringLiteral("尚未连接"),
                                 QStringLiteral("请先连接 FPGA 串口，再使用 F4 切换 LED 方向。"));
    }
}

void MainWindow::sendFrequency()
{
    if (!writeCommand(GxProtocol::frequencyCommand(), QStringLiteral("F10 调频 / F"), true)) {
        if (!m_demoMode)
            QMessageBox::information(this, QStringLiteral("尚未连接"),
                                     QStringLiteral("请先连接 FPGA 串口，再使用 F10 调频。"));
    }
}

void MainWindow::sendPing()
{
    writeCommand(GxProtocol::pingCommand(), QStringLiteral("心跳 P"), false);
}

void MainWindow::handleReadyRead()
{
    m_receiveBuffer += m_serial->readAll();
    if (m_receiveBuffer.size() > 8192) {
        appendLog(QStringLiteral("接收缓冲区过长，已清空"), QStringLiteral("WARN"));
        m_receiveBuffer.clear();
        return;
    }

    int newlineIndex = -1;
    while ((newlineIndex = m_receiveBuffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_receiveBuffer.left(newlineIndex + 1);
        m_receiveBuffer.remove(0, newlineIndex + 1);
        if (line.trimmed().isEmpty())
            continue;

        QString error;
        const auto parsed = GxProtocol::parseStatusLine(line, &error);
        if (!parsed) {
            appendLog(QStringLiteral("忽略无效数据：%1 [%2]")
                          .arg(QString::fromLatin1(line.trimmed()), error),
                      QStringLiteral("WARN"));
            continue;
        }

        const bool importantChange = !m_haveStatus
            || parsed->eventCounter != m_status.eventCounter
            || parsed->actionCode != m_status.actionCode
            || parsed->linkOnline != m_status.linkOnline;
        ++m_rxPackets;
        m_haveStatus = true;
        m_protocolAlive = true;
        if (m_lastStatusTimer.isValid())
            m_lastStatusTimer.restart();
        else
            m_lastStatusTimer.start();
        applyStatus(*parsed);

        if (importantChange)
            appendLog(QStringLiteral("<< %1").arg(QString::fromLatin1(parsed->rawLine)),
                      QStringLiteral("RX"));
    }
}

void MainWindow::applyStatus(const DeviceStatus &status)
{
    m_status = status;
    m_sevenSegment->setValue(status.count);
    m_ledBar->setMask(status.ledMask);
    m_lcdLine1->setText(status.lcdLine1());
    m_lcdLine2->setText(status.lcdLine2());

    m_portValue->setText(m_demoMode ? QStringLiteral("COM9（演示）")
                                    : (m_serial->isOpen() ? m_serial->portName()
                                                          : selectedPortName()));
    m_uartConfigValue->setText(QStringLiteral("115200 / 8N1"));
    m_clockValue->setText(QStringLiteral("100.000 MHz"));
    m_frequencyValue->setText(QStringLiteral("%1 Hz").arg(status.frequencyHz(), 0, 'f', 2));
    m_dividerValue->setText(QStringLiteral("%L1 cycles").arg(status.divider));
    m_frequencyLevelValue->setText(QStringLiteral("%1 / 5").arg(status.frequencyIndex + 1));
    m_countHexValue->setText(hexValue(status.count, 8));
    m_countDecimalValue->setText(QString::number(static_cast<qulonglong>(status.count)));
    m_runningValue->setText(status.running ? QStringLiteral("继续 / 运行中")
                                           : QStringLiteral("暂停 / 保持"));
    m_countDirectionValue->setText(status.countDown ? QStringLiteral("递减 ↓")
                                                    : QStringLiteral("递增 ↑"));
    m_ledMaskValue->setText(hexValue(status.ledMask, 3));

    int activeLed = -1;
    int activeCount = 0;
    for (int i = 0; i < 12; ++i) {
        if ((status.ledMask & (1U << i)) != 0) {
            activeLed = i + 1;
            ++activeCount;
        }
    }
    m_activeLedValue->setText(activeCount == 1
        ? QStringLiteral("LED%1").arg(activeLed)
        : QStringLiteral("%1 路点亮").arg(activeCount));
    m_ledDirectionValue->setText(status.ledRight ? QStringLiteral("向右 →")
                                                 : QStringLiteral("向左 ←"));
    m_actionValue->setText(status.actionText());
    m_eventCounterValue->setText(QStringLiteral("%1 / %2")
                                     .arg(hexValue(status.eventCounter, 4))
                                     .arg(status.eventCounter));
    m_rxPacketsValue->setText(QString::number(m_rxPackets));
    m_txCommandsValue->setText(QString::number(m_txCommands));
    m_lastUpdateValue->setText(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")));

    m_functionValues.at(0)->setText(status.running ? QStringLiteral("当前：运行中")
                                                   : QStringLiteral("当前：已暂停"));
    m_functionValues.at(1)->setText(QStringLiteral("计数：%1").arg(hexValue(status.count, 8)));
    m_functionValues.at(2)->setText(!status.countDown ? QStringLiteral("当前方向：递增")
                                                      : QStringLiteral("当前未选"));
    m_functionValues.at(3)->setText(status.countDown ? QStringLiteral("当前方向：递减")
                                                     : QStringLiteral("当前未选"));
    m_functionValues.at(4)->setText(status.ledRight ? QStringLiteral("当前：向右")
                                                    : QStringLiteral("当前：向左"));
    m_functionValues.at(5)->setText(QStringLiteral("%1 Hz · 档位 %2")
                                        .arg(status.frequencyHz(), 0, 'f', 2)
                                        .arg(status.frequencyIndex + 1));

    int activeCard = -1;
    switch (status.actionCode) {
    case 0x01:
    case 0x02: activeCard = 0; break;
    case 0x03: activeCard = 1; break;
    case 0x04: activeCard = 2; break;
    case 0x05: activeCard = 3; break;
    case 0x06:
    case 0x07: activeCard = 4; break;
    case 0x08: activeCard = 5; break;
    default: break;
    }
    for (int i = 0; i < static_cast<int>(m_functionCards.size()); ++i) {
        m_functionCards.at(i)->setProperty("active", i == activeCard);
        repolish(m_functionCards.at(i));
    }

    updateConnectionVisual();
}

void MainWindow::updateConnectionVisual()
{
    QString badgeText;
    QByteArray badgeState;

    if (m_demoMode) {
        badgeText = QStringLiteral("FPGA 联机 · 演示");
        badgeState = "online";
    } else if (!m_serial->isOpen()) {
        badgeText = QStringLiteral("未连接");
        badgeState = "offline";
    } else if (m_protocolAlive && m_status.linkOnline) {
        badgeText = QStringLiteral("FPGA 联机");
        badgeState = "online";
    } else {
        badgeText = QStringLiteral("串口已开 · 等待 FPGA");
        badgeState = "waiting";
    }

    m_connectionBadge->setText(badgeText);
    m_connectionBadge->setProperty("state", badgeState);
    repolish(m_connectionBadge);
    m_connectButton->setText(m_serial->isOpen() ? QStringLiteral("断开")
                                                : QStringLiteral("连接"));
    m_portOpenValue->setText(m_demoMode ? QStringLiteral("演示模式")
        : (m_serial->isOpen() ? QStringLiteral("已打开") : QStringLiteral("已关闭")));
    m_fpgaLinkValue->setText((m_demoMode || (m_protocolAlive && m_status.linkOnline))
        ? QStringLiteral("联机") : QStringLiteral("断开"));
}

void MainWindow::checkWatchdog()
{
    if (!m_serial->isOpen() || !m_protocolAlive || !m_lastStatusTimer.isValid())
        return;
    if (m_lastStatusTimer.elapsed() <= 1500)
        return;

    m_protocolAlive = false;
    m_status.linkOnline = false;
    appendLog(QStringLiteral("超过 1.5 秒未收到 FPGA 状态包"), QStringLiteral("WARN"));
    applyStatus(m_status);
}

void MainWindow::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::NotOpenError)
        return;
    if (error != QSerialPort::ResourceError)
        return;

    appendLog(QStringLiteral("串口资源错误：%1").arg(m_serial->errorString()),
              QStringLiteral("ERROR"));
    m_pingTimer->stop();
    m_watchdogTimer->stop();
    m_serial->close();
    m_protocolAlive = false;
    m_status.linkOnline = false;
    applyStatus(m_status);
}

void MainWindow::appendLog(const QString &message, const QString &kind)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    const QString tag = kind.isEmpty() ? QString() : QStringLiteral(" [%1]").arg(kind);
    m_log->appendPlainText(QStringLiteral("%1%2  %3").arg(timestamp, tag, message));
}

void MainWindow::loadDemoState()
{
    m_portCombo->setEditText(QStringLiteral("COM9"));
    m_demoMode = true;
    m_haveStatus = true;
    m_protocolAlive = true;
    m_rxPackets = 128;
    m_txCommands = 131;
    m_status.count = 0x12AB34CDU;
    m_status.running = true;
    m_status.countDown = false;
    m_status.ledRight = true;
    m_status.frequencyIndex = 2;
    m_status.divider = 100000000U;
    m_status.ledMask = 0x020U;
    m_status.actionCode = 0x04U;
    m_status.linkOnline = true;
    m_status.eventCounter = 0x003AU;
    applyStatus(m_status);
    appendLog(QStringLiteral("界面演示数据已载入（不访问串口）"), QStringLiteral("DEMO"));
    appendLog(QStringLiteral("<< S,12AB34CD,1,U,R,2,05F5E100,020,04,1,003A"),
              QStringLiteral("RX"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_pingTimer->stop();
    m_watchdogTimer->stop();
    if (m_serial->isOpen()) {
        m_serial->write(QByteArrayLiteral("X\n"));
        m_serial->flush();
        m_serial->waitForBytesWritten(100);
        m_serial->close();
    }
    QMainWindow::closeEvent(event);
}
