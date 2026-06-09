#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("XrayQtClient");
    app.setOrganizationName("Arena");

    MainWindow window;
    window.show();

    return app.exec();
}
