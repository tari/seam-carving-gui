#include <QApplication>
#include <QDir>
#include "mainwindow.h"

#ifdef STATIC_PLUGINS
#include <QtPlugin>
Q_IMPORT_PLUGIN(qjpeg)
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
#ifdef Q_OS_MAC
    //Set plugins dir for mac
    QDir dir(QApplication::applicationDirPath());
    dir.cdUp();
    dir.cd("plugins"); //../plugins
    QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif
    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
