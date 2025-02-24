#include <QtTest/QtTest>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QScreen>
#include "mainwindow.h"
#include "test_p2p.h"

void Test_P2P::initTestCase() {
    createWindow(4);
}

// test message propagation
void Test_P2P::test1() {

    MainWindow *w_5000 = windows[5000];
    MainWindow *w_5001 = windows[5001];
    MainWindow *w_5002 = windows[5002];
    MainWindow *w_5003 = windows[5003];

    QString testMessage1 = "First Test Message!";
    QString testMessage2 = "Second Test Message!";
    QString testMessage3 = "Third Test Message!";

    QTest::qWait(3000);
    testSendMessage(*w_5000, testMessage1);
    QTest::qWait(2000);
    testSendMessage(*w_5002, testMessage2);
    QTest::qWait(2000);
    testSendMessage(*w_5003, testMessage3);

    // test if the sender themselves got the message
    QTest::qWait(3000); // let message propagate
    testReceivedMessage(*w_5000, testMessage1);
    testReceivedMessage(*w_5002, testMessage2);
    testReceivedMessage(*w_5003, testMessage3);

    // test if peer gets the progagated message
    QTest::qWait(3000);
    testReceivedMessage(*w_5000, testMessage1);
    testReceivedMessage(*w_5000, testMessage2);
    testReceivedMessage(*w_5000, testMessage3);

    testReceivedMessage(*w_5001, testMessage1);
    testReceivedMessage(*w_5001, testMessage2);
    testReceivedMessage(*w_5001, testMessage3);

    testReceivedMessage(*w_5002, testMessage1);
    testReceivedMessage(*w_5002, testMessage2);
    testReceivedMessage(*w_5002, testMessage3);

    testReceivedMessage(*w_5003, testMessage1);
    testReceivedMessage(*w_5003, testMessage2);
    testReceivedMessage(*w_5003, testMessage3);

}

// test peer discovery
void Test_P2P::test2() {
    MainWindow *w_5000 = windows[5000];
    MainWindow *w_5001 = windows[5001];
    MainWindow *w_5002 = windows[5002];
    MainWindow *w_5003 = windows[5003];

    QVector<quint16> neighbors_5000 = w_5000->udpHandler->myNeighbors;
    QVector<quint16> neighbors_5002 = w_5002->udpHandler->myNeighbors;

    // verify that port 5000 only has 5001 as neighbor
    QVERIFY(neighbors_5000.size() == 1); // since 5000 is the start of the network, it doesn't have a left neighbor
    QVERIFY(neighbors_5000.contains(5001)); // check that the only neighbor is 5001

    // verify that port 5002 has 5001 and 5003 as neighbors
    QVERIFY(neighbors_5002.size() == 2); // check that there are two neighbors
    QVERIFY(neighbors_5002.contains(5001)); // check that one neighbor is 5001
    QVERIFY(neighbors_5002.contains(5003)); // check that the other neighbor is 5003


}

void Test_P2P::test3() {
    MainWindow *w_5000 = windows[5000];
    MainWindow *w_5002 = windows[5002];
    MainWindow *w_5003 = windows[5003];

    killInstance(5001);

    QString testMessage4 = "4_5000";
    QString testMessage5 = "5_5002";
    QString testMessage6 = "6_5000";
    QString testMessage7 = "7_5000";
    QString testMessage8 = "8_5003";

    QTest::qWait(500);
    testSendMessage(*w_5000, testMessage4);
    QTest::qWait(500);
    testSendMessage(*w_5002, testMessage5);
    QTest::qWait(500);
    testSendMessage(*w_5000, testMessage6);
    QTest::qWait(500);
    testSendMessage(*w_5000, testMessage7);
    QTest::qWait(500);
    testSendMessage(*w_5003, testMessage8);
    QTest::qWait(500);

    // since instance 5001 has been killed, 5000 is disconnected from the rest of the peer, therefore it should not be able to send or receive messages from 5002 and 5003
    testDidntReceiveMessage(*w_5002, testMessage4);
    testDidntReceiveMessage(*w_5003, testMessage4);
    testDidntReceiveMessage(*w_5000, testMessage5);
    testDidntReceiveMessage(*w_5002, testMessage6);
    testDidntReceiveMessage(*w_5003, testMessage6);
    testDidntReceiveMessage(*w_5002, testMessage7);
    testDidntReceiveMessage(*w_5003, testMessage7);
    testDidntReceiveMessage(*w_5000, testMessage8);

    QTest::qWait(2000);
    createWindow(1); // this should spin up 5001 again

    QTest::qWait(10000); // wait for 5001 to update message and progagate message to its neighbors (NOTE THAT PEERS PICK NEIGHBORS TO COMPARE HISTORY AT RANDOM, THERE IS A CHANCE THAT 10 SECONDS ISN"T ENOUGH TO FULLY UPDATE)

    QStringList expectedMessageOrder = {
        "First Test Message!",
        "Second Test Message!",
        "Third Test Message!",
        "4_5000",
        "5_5002",
        "6_5000",
        "7_5000",
        "8_5003"
    };

    MainWindow *w_5001 = windows[5001];

    testReceivedMessageOrder(*w_5000, expectedMessageOrder);
    testReceivedMessageOrder(*w_5001, expectedMessageOrder);
    testReceivedMessageOrder(*w_5002, expectedMessageOrder);
    testReceivedMessageOrder(*w_5003, expectedMessageOrder);
}

void Test_P2P::test4() {
    killInstance(5003);
    MainWindow *w_5002 = windows[5002];
    testSendMessage(*w_5002, "test resend message");
    createWindow(1); // should create 5003 after 5002 sends message

}

// kill and remove from map
void Test_P2P::killInstance(quint16 port) {
    if (!windows.contains(port)) return;

    MainWindow *window = windows[port];
    if (window) {

        if (window->udpHandler) {
            window->udpHandler->deleteLater();
        }

        window->close();
        delete window;
        QCoreApplication::processEvents();

    }
    windows.remove(port);
}

QMap<quint16, MainWindow*> Test_P2P::createWindow(int numWindow) {

    quint16 port = 5000;
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    int windowWidth = 328; // Approximate width of each window
    int windowHeight = 531; // Approximate height of each window
    int spacing = 0; // Space between windows


    for (int i = 1; i < numWindow + 1; i++) {
        UDPHandler *udpHandler = new UDPHandler(nullptr, port);
        MainWindow *w = new MainWindow(nullptr, udpHandler);

        // udpHandler->setParent(w);

        int position = udpHandler->myPort - 5000;
        w->setWindowTitle(QString("Port %1").arg(udpHandler->myPort));
        w->move(screenGeometry.x() + position * (windowWidth + position + spacing), screenGeometry.y() + spacing);
        w->show();
        windows[udpHandler->myPort] = w;
    }

    return windows;
}

void Test_P2P::testSendMessage(MainWindow &w, QString message) {

    QPlainTextEdit *messageBox = w.findChild<QPlainTextEdit *>("editMessageBox");
    QPushButton *sendButton = w.findChild<QPushButton *>("sendButton");
    QTextBrowser *messageHistory = w.findChild<QTextBrowser *>("receivedMessageBox");

    QVERIFY(messageBox);
    QVERIFY(sendButton);
    QVERIFY(messageHistory);

    messageBox->setPlainText(message);


    QTest::mouseClick(sendButton, Qt::LeftButton);

    QString historyText = messageHistory->toPlainText();
    QVERIFY(historyText.contains(message));
}

void Test_P2P::testReceivedMessage(MainWindow &w, QString message) {
    QPlainTextEdit *messageBox = w.findChild<QPlainTextEdit *>("editMessageBox");
    QPushButton *sendButton = w.findChild<QPushButton *>("sendButton");
    QTextBrowser *messageHistory = w.findChild<QTextBrowser *>("receivedMessageBox");

    QString historyText = messageHistory->toPlainText();
    QVERIFY(historyText.contains(message));
}

void Test_P2P::testReceivedMessageOrder(MainWindow &w, QStringList expectedOrder) {
    QTextBrowser *messageHistory = w.findChild<QTextBrowser *>("receivedMessageBox");
    QVERIFY(messageHistory);

    QString historyText = messageHistory->toPlainText();
    QStringList receivedMessages = historyText.split("\n", Qt::SkipEmptyParts); // split messages by new lines

    // extract only the message part (after Peer X: )
    for (int i = 0; i < receivedMessages.size(); i++) {
        receivedMessages[i] = receivedMessages[i].section(":", 1).trimmed();
    }


    QVERIFY(receivedMessages == expectedOrder);
}

void Test_P2P::testDidntReceiveMessage(MainWindow &w, QString message) {
    QPlainTextEdit *messageBox = w.findChild<QPlainTextEdit *>("editMessageBox");
    QPushButton *sendButton = w.findChild<QPushButton *>("sendButton");
    QTextBrowser *messageHistory = w.findChild<QTextBrowser *>("receivedMessageBox");

    QString historyText = messageHistory->toPlainText();
    QVERIFY(!historyText.contains(message));
}
