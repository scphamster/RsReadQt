#include "mainwindow.h"
#include <QtWidgets/QApplication>
#include <qtextbrowser.h>
#include <qplaintextedit.h>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    a.exec();

    return 0;
}
