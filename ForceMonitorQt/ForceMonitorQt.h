#pragma once

#define VERSION "1.0.1"

#include <QtWidgets/QMainWindow>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <QRegularExpressionValidator>

#include <QtCharts/QLineSeries>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

#include "ui_ForceMonitorQt.h"
// #include "Plc.h"
#include "VirtualPlc.h"
#include "DataLogger.h"
#include "timer.hpp"
#include <string>

// #define PLOT_STEP 10
#define PLOT_STEP 100
#define PLOT_DURATION 15.0

#define ABOUT_PAGE_INFO "\nAbout ForceMonitor, version 1.0.1, 2023\n \
\nThis is a software intended for force sensor data real-time sampling, collecting and visualization. \
This software is designed by Dept. of Mechanical Engineering, Tsinghua University, unofficially. \
This software is only intended for teaching purpose. \n"

enum IPCStatus {
    Offline, Ready, Runing
};

class ForceMonitorQt : public QMainWindow
{
    Q_OBJECT

public:
    ForceMonitorQt(QWidget *parent = nullptr);
    ~ForceMonitorQt();

    /* User Interface */
    void Start();

    void Stop();

    void Connect();

    void Disconnect();

    /* Chart Operation */
    void initData();

    void updateData();

    /* Data Persistence */
    // TODO:
    void logData(PLC_BUFFER_TYPE** data, unsigned int* length);

    void getDataFileName();

    void SetWorkDirectory();

    void InfoWorkDirectory();

    void AboutPage();

signals:
    void updateUi();

private:
    Ui::ForceMonitorQtClass ui;
    // Plc* plc;
    VirtualPlc* plc;
    IPCStatus status;
    QLineSeries* data[3];

    unsigned long time_stamp[3] = { 0, 0, 0 };
    PLC_BUFFER_TYPE data_max[3] = { 4000, 4000, 4000 };
    PLC_BUFFER_TYPE data_min[3] = { 0, 0, 0 };
    QChart* chart[3];
    QLabel* status_vis_label;

    QRegExpValidator* regular_expression_validator;

    string data_path;
    string data_file;
    DataLogger* recorder;

    Timer timer;

    PLC_BUFFER_TYPE* temp1;
    PLC_BUFFER_TYPE* temp2;
    PLC_BUFFER_TYPE* temp3;
    PLC_BUFFER_TYPE* temp[3];
};

bool checkFile(string log_file_directory, string log_file_name);
