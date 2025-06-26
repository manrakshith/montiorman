#include "systemtray.h"

SystemTray::SystemTray(QObject *parent)
    : QObject{parent}
{
    monopt = new MonitorOptions();
    monopt->show();
    createTrayIcon();
    trayIcon->show();
}

SystemTray::~SystemTray(){

}

void SystemTray::createTrayIcon()
{
    trayMenu = new QMenu(); // or trayMenu = new QMenu(nullptr);

    trayMenu->addAction("Show", this, SLOT(showApp()));
    trayMenu->addAction("Quit", this, SLOT(quitApp()));

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/img/appicon.png")); // Make sure this exists
    trayIcon->setToolTip("Tray Application");
    trayIcon->setContextMenu(trayMenu);

    connect(trayIcon, &QSystemTrayIcon::activated, this, &SystemTray::iconActivated);
}


void SystemTray::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
        QPoint cursorPos = QCursor::pos();

        int dialogWidth = monopt->width();
        int dialogHeight = monopt->height();

        int x = cursorPos.x() - dialogWidth / 2;
        int y = cursorPos.y() - dialogHeight - 10;

        x = std::clamp(x, screenGeometry.left(), screenGeometry.right() - dialogWidth);
        y = std::clamp(y, screenGeometry.top(), screenGeometry.bottom() - dialogHeight);

        monopt->move(x, y);
        monopt->show();
        monopt->raise();
        monopt->activateWindow();
    }
}

void SystemTray::showApp()
{
    monopt->showNormal();
    monopt->raise();
    monopt->activateWindow();
}

void SystemTray::quitApp()
{
    QApplication::quit();
}
