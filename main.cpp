#include "mainwindow.h"
#include "udphandler.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    quint16 port = 5000;

    UDPHandler udpHandler(nullptr, port);

    MainWindow w(nullptr, &udpHandler);
    w.setWindowTitle(QString("%1").arg(udpHandler.myPort));
    w.show();

    udpHandler.sendIntro();

    return a.exec();
}
