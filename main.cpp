#include "mainwindow.h"
#include "udphandler.h"
#include "test_p2p.h"

#include <QApplication>
#include <QTest>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    UDPHandler udpHandler(nullptr, 5000);

    MainWindow w(nullptr, &udpHandler);
    w.setWindowTitle(QString("%1").arg(udpHandler.myPort));
    w.show();

    udpHandler.sendIntro();

    return a.exec();

    // Test_P2P test;
    // return QTest::qExec(&test, argc, argv);
}
