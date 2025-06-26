#ifndef MONITOROPTIONS_H
#define MONITOROPTIONS_H

#include <QDialog>
#include <QSystemTrayIcon>
#include <windows.h>
#include <highlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <iostream>
#include <QCloseEvent>
#include <QApplication>
#include <QDebug>
#include <QAction>

namespace Ui {
class MonitorOptions;
}

class MonitorOptions : public QDialog
{
    Q_OBJECT

public:
    explicit MonitorOptions(QWidget *parent = nullptr);
    ~MonitorOptions();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MonitorOptions *ui;
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    HMONITOR hMonitor;
    DWORD monitorCount = 0;
    PHYSICAL_MONITOR* pMonitors;
    void initMonitor();
    void readCurrentBrightness();
    void readCurrentContrast();

private slots:
    void on_brightnessSlider_sliderMoved(int position);
    void on_contrastSlider_sliderMoved(int position);

};

#endif // MONITOROPTIONS_H
