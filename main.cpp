#include <QApplication>

#include "mainwindow.h"

#ifdef STATIC_PLUGINS
#include <QtPlugin>
Q_IMPORT_PLUGIN(qjpeg)
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
