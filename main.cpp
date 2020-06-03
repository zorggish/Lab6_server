#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("ITMO");
    QCoreApplication::setOrganizationDomain("itmo.ru");
    QCoreApplication::setApplicationName("WebSocketChat");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
