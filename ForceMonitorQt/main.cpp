#include "ForceMonitorQt.h"
#include <QtWidgets/QApplication>

#ifndef GLOG_NO_ABBREVIATED_SEVERITIES
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#endif

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    google::SetStderrLogging(google::GLOG_INFO);
    QApplication a(argc, argv);
    ForceMonitorQt w;
    w.show();
    return a.exec();
}
