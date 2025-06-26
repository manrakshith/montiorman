#include "systemtray.h"

SystemTray::SystemTray(QObject *parent)
    : QObject{parent}
{
    monopt = new MonitorOptions();
    showMontitorOptions();
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

void SystemTray::showMontitorOptions(){
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect availableGeometry = screen->availableGeometry(); // Excludes taskbar
    // Calculate bottom-right corner (system tray is usually here)
    int x = availableGeometry.right() - monopt->width() - 10;
    int y = availableGeometry.bottom() - monopt->height() - 10;

    monopt->move(x, y);  // Move the dialog near the tray
    monopt->show();
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
