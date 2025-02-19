#include "mainwindow.h"
#include "udphandler.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    quint16 port = 5000;

    UDPHandler udpHandler(nullptr, port);

    udpHandler.SendIntro();

    MainWindow w(nullptr, &udpHandler);
    w.show();
    return a.exec();
}
