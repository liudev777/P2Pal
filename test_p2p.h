#ifndef TEST_P2P_H
#define TEST_P2P_H

#include <QTest>
#include "mainwindow.h"


class Test_P2P : public QObject {
    Q_OBJECT

private:
    QMap<quint16, MainWindow*> windows;
    QMap<quint16, MainWindow*>  createWindow(int numWindow);

private:
    void testSendMessage(MainWindow &w, QString message);
    void testReceivedMessage(MainWindow &w, QString message);
    void testDidntReceiveMessage(MainWindow &w, QString message);
    void killInstance(quint16 port);
    void testReceivedMessageOrder(MainWindow &w, QStringList expectedOrder);



private slots:
    void initTestCase();
    void test1();
    void test2();
    void test3();
};

#endif // TEST_P2P_H
