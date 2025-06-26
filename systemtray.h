#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QMenu>
#include <QDebug>
#include <QObject>
#include <QDialog>
#include <QScreen>
#include <windows.h>
#include <QApplication>
#include <QGuiApplication>
#include <QSystemTrayIcon>
#include "monitoroptions.h"

class SystemTray : public QObject
{
    Q_OBJECT
public:
    explicit SystemTray(QObject *parent = nullptr);
    ~SystemTray();

signals:

private:
    QMenu *trayMenu;
    MonitorOptions *monopt;
    QSystemTrayIcon *trayIcon;
    void createTrayIcon();
    void createActions();

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void showApp();
    void quitApp();

};

#endif // SYSTEMTRAY_H
