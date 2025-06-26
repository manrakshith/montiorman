#include "monitoroptions.h"
#include "ui_monitoroptions.h"


MonitorOptions::MonitorOptions(QWidget *parent)
    : QDialog(parent, Qt::Popup | Qt::FramelessWindowHint)
    , ui(new Ui::MonitorOptions)
{
    ui->setupUi(this);
    initMonitor();
}

MonitorOptions::~MonitorOptions()
{
    delete ui;
}

void MonitorOptions::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore(); // Prevent app from quitting
}

void MonitorOptions::initMonitor(){
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
    readCurrentContrast();

    QString monitorName;
    for (DWORD i = 0; i < monitorCount; ++i) {
        monitorName = QString::fromWCharArray(pMonitors[i].szPhysicalMonitorDescription);
        qDebug() << "Monitor" << i << ":" << monitorName;
    }


    ui->monitorName->setText(monitorName);

}
void MonitorOptions::readCurrentBrightness()
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
            ui->brightness_value->setText(QString("%1").arg(currBrightness));

        } else {
            qDebug() << "Failed to get monitor brightness.";
        }
    }
}

void MonitorOptions::readCurrentContrast(){
    if (pMonitors && monitorCount > 0) {
        DWORD minConstrast = 0, currConstrast = 0, maxConstrast = 0;
        if (GetMonitorBrightness(pMonitors[0].hPhysicalMonitor, &minConstrast, &currConstrast, &maxConstrast)) {
            qDebug() << "Monitor brightness range:" << minConstrast << "-" << maxConstrast;
            qDebug() << "Current brightness:" << currConstrast;
            // You can update your slider here:
            // ui->horizontalSlider->setMinimum(minBrightness);
            // ui->hosrizontalSlider->setMaximum(maxBrightness);
            // ui->horizontalSlider->setValue(currBrightness);

            ui->contrastSlider->setMinimum(minConstrast);
            ui->contrastSlider->setMaximum(maxConstrast);
            ui->contrastSlider->setValue(currConstrast);
            ui->contrast_values->setText(QString("%1").arg(currConstrast));

        } else {
            qDebug() << "Failed to get monitor brightness.";
        }
    }
}

void MonitorOptions::on_brightnessSlider_sliderMoved(int position)
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

void MonitorOptions::on_contrastSlider_sliderMoved(int position)
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
