#include "mainwindow.h"

#include <QApplication>
#include <QFileInfo>
#include <QFont>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("GXCounterTerminal"));
    application.setOrganizationName(QStringLiteral("GX-BIDT"));

    QFont applicationFont(QStringLiteral("Microsoft YaHei UI"));
    applicationFont.setPointSize(12);
    application.setFont(applicationFont);

    MainWindow window;
    const QStringList arguments = application.arguments();
    const int screenshotIndex = arguments.indexOf(QStringLiteral("--screenshot"));
    const bool screenshotRequested = screenshotIndex >= 0
        && screenshotIndex + 1 < arguments.size();

    if (arguments.contains(QStringLiteral("--demo")) || screenshotRequested)
        window.loadDemoState();

    if (screenshotRequested)
        window.resize(1640, 1450);

    window.show();

    if (screenshotRequested) {
        const QString outputPath = QFileInfo(arguments.at(screenshotIndex + 1)).absoluteFilePath();
        QTimer::singleShot(900, &window, [&application, &window, outputPath] {
            const bool saved = window.grab().save(outputPath);
            application.exit(saved ? 0 : 2);
        });
    }

    return application.exec();
}
