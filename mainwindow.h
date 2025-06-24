#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <windows.h>
#include <highlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <iostream>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_brightnessSlider_sliderMoved(int position);
    void on_contrastSlider_sliderMoved(int position);

private:
    Ui::MainWindow *ui;
    HMONITOR hMonitor;
    DWORD monitorCount = 0;
    PHYSICAL_MONITOR* pMonitors;

    void initMonitor();
    void readCurrentBrightness();
    void readCurrentContrast();
};
#endif // MAINWINDOW_H
