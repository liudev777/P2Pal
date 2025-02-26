#include "mainwindow.h"
#include "udphandler.h"
#include "test_p2p.h"

#include <QApplication>
#include <QTest>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QStringList args = a.arguments();

    // checks for --test flag to run tests
    if (args.contains("--test")) {
        // --test gets passed to qExec and causes it to braek, so we want to remove it from the arguments before passing it.
        int newArgc = argc - 1;
        char *newArgv[argc];
        int index = 0;
        for (int i = 0; i < argc; i++) {
            if (QString(argv[i]) != "--test") {
                newArgv[index++] = argv[i];
            }
        }
        newArgv[index] = nullptr;

        Test_P2P test;
        return QTest::qExec(&test, newArgc, newArgv);
    }

    // main logic for handling connections
    UDPHandler udpHandler(nullptr, 5000);

    MainWindow w(nullptr, &udpHandler);
    w.setWindowTitle(QString("%1").arg(udpHandler.myPort));
    w.show();

    udpHandler.sendIntro();

    return a.exec();
}
