#include <QtTest/QtTest>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include "mainwindow.h"


class GuiTest : public QObject {
    Q_OBJECT

private slots:
    void testSendMessage();
};

void GuiTest::testSendMessage() {

    quint16 port = 5000;

    UDPHandler udpHandler(nullptr, port);

    MainWindow w(nullptr, &udpHandler);
    w.setWindowTitle(QString("%1").arg(udpHandler.myPort));
    w.show();

    QPlainTextEdit *messageBox = w.findChild<QPlainTextEdit *>("editMessageBox");
    QPushButton *sendButton = w.findChild<QPushButton *>("sendButton");
    QTextBrowser *messageHistory = w.findChild<QTextBrowser *>("receivedMessageBox");

    QVERIFY(messageBox);
    QVERIFY(sendButton);
    QVERIFY(messageHistory);


    messageBox->setPlainText("Hello from test!");


    QTest::mouseClick(sendButton, Qt::LeftButton);

    QString historyText = messageHistory->toPlainText();
    QVERIFY(historyText.contains("Hello from test!"));
}

// Run the test suite
QTEST_MAIN(GuiTest)
#include "test_p2p.moc"
