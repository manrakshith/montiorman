#ifndef QT_NO_SYSTEMTRAYICON
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QObject>
#include <QStyleFactory> // For available styles
#include <QScreen>

#include "systemtray.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << "Available styles:" << QStyleFactory::keys();

    // Option 1: Set a specific built-in style
    // The "Fusion" style is a modern, platform-agnostic style that looks good everywhere.
    // It's often recommended if you want a consistent look that isn't strictly native.
    a.setStyle(QStyleFactory::create("Fusion"));

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, QObject::tr("Systray"),
                              QObject::tr("I couldn't detect any system tray "
                                          "on this system."));
        return 1;
    }

    SystemTray systemTray;
    return a.exec();
}
#else

#include <QLabel>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString text("QSystemTrayIcon is not supported on this platform");

    QLabel *label = new QLabel(text);
    label->setWordWrap(true);

    label->show();
    qDebug() << text;

    app.exec();
}
#endif
