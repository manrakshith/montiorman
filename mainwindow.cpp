#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("MonitorMan");
    initMonitor();
}

MainWindow::~MainWindow()
{
    delete ui;

    // Clean up
    DestroyPhysicalMonitors(monitorCount, pMonitors);
    delete[] pMonitors;
}

void MainWindow::initMonitor(){
    hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);

    monitorCount = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &monitorCount) || monitorCount == 0) {
        qDebug()<<"Failed to get monitor count.";
        return;
    }

    pMonitors = new PHYSICAL_MONITOR[monitorCount];
    if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, monitorCount, pMonitors)) {
        qDebug()<<"Failed to get physical monitors.";
        delete[] pMonitors;
        return;
    }
    readCurrentBrightness();

}

void MainWindow::readCurrentBrightness()
{
    if (pMonitors && monitorCount > 0) {
        DWORD minBrightness = 0, currBrightness = 0, maxBrightness = 0;
        if (GetMonitorBrightness(pMonitors[0].hPhysicalMonitor, &minBrightness, &currBrightness, &maxBrightness)) {
            qDebug() << "Monitor brightness range:" << minBrightness << "-" << maxBrightness;
            qDebug() << "Current brightness:" << currBrightness;
            // You can update your slider here:
            // ui->horizontalSlider->setMinimum(minBrightness);
            // ui->hosrizontalSlider->setMaximum(maxBrightness);
            // ui->horizontalSlider->setValue(currBrightness);

            ui->brightnessSlider->setMinimum(minBrightness);
            ui->brightnessSlider->setMaximum(maxBrightness);
            ui->brightnessSlider->setValue(currBrightness);
            ui->brightness_value->setText(QString("Brightness: %1").arg(currBrightness));

        } else {
            qDebug() << "Failed to get monitor brightness.";
        }
    }
}

void MainWindow::readCurrentContrast(){
   if (pMonitors && monitorCount > 0) {
        DWORD minConstrast = 0, currConstrast = 0, maxConstrast = 0;
        if (GetMonitorBrightness(pMonitors[0].hPhysicalMonitor, &minBrightness, &currBrightness, &maxBrightness)) {
            qDebug() << "Monitor brightness range:" << minBrightness << "-" << maxBrightness;
            qDebug() << "Current brightness:" << currBrightness;
            // You can update your slider here:
            // ui->horizontalSlider->setMinimum(minBrightness);
            // ui->hosrizontalSlider->setMaximum(maxBrightness);
            // ui->horizontalSlider->setValue(currBrightness);

            ui->brightnessSlider->setMinimum(minBrightness);
            ui->brightnessSlider->setMaximum(maxBrightness);
            ui->brightnessSlider->setValue(currBrightness);
            ui->brightness_value->setText(QString("Brightness: %1").arg(currBrightness));

        } else {
            qDebug() << "Failed to get monitor brightness.";
        }
   }
}

void MainWindow::on_brightnessSlider_sliderMoved(int position)
{
    // qDebug() << "position = " << position;
    // Set brightness on the first monitor
    if (pMonitors && monitorCount > 0) {
        if (!SetMonitorBrightness(pMonitors[0].hPhysicalMonitor, position)) {
            // qDebug() << "Failed to set brightness.";
        } else {
            // qDebug() << "Brightness set to" << position;
            ui->brightness_value->setText(QString("%1").arg(position));
        }
    }
}

void MainWindow::on_contrastSlider_sliderMoved(int position)
{
    if (pMonitors && monitorCount > 0) {
        if (!SetMonitorContrast(pMonitors[0].hPhysicalMonitor, position)) {
            // qDebug() << "Failed to set brightness.";
        } else {
            // qDebug() << "Brightness set to" << position;
            ui->contrast_values->setText(QString("%1").arg(position));
        }
    }
}

