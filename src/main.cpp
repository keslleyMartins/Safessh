#include <QApplication>
#include <spdlog/spdlog.h>
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    QApplication app(argc, argv);
    app.setApplicationName("SafeSSH");
    app.setOrganizationName("SafeSSH");
    app.setApplicationVersion("0.1.0");

    MainWindow w;
    w.show();

    return app.exec();
}
