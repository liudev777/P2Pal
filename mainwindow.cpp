#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, UDPHandler *udpHandler)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , udpHandler(udpHandler)
{
    ui->setupUi(this);

    ui->editMessageBox->setFocus(); // autofocus on launch
    ui->myPortLabel->setText(QString("%1").arg(udpHandler->myPort));

    connect(udpHandler, &UDPHandler::messageReceived, this, &MainWindow::displayMessage);
    connect(udpHandler, &UDPHandler::peerJoined, this, &MainWindow::displayJoinedPeer);
    connect(udpHandler, &UDPHandler::updatedHistory, this, &MainWindow::displayMessageHistory);

}

MainWindow::~MainWindow()
{
    delete ui;
}

QString message;

void MainWindow::on_sendButton_clicked()
{
    message = ui->editMessageBox->toPlainText();
    // qDebug() << message;

    // ui->receivedMessageBox->append(message);

    if (udpHandler) {
        udpHandler->sendMessage(message);
    }

    ui->editMessageBox->clear();

}

void MainWindow::displayMessage(int sequenceNum, quint16 senderPort, QString message) {
    ui->receivedMessageBox->append(QString("<b>%1. Peer %2:</b> %3").arg(sequenceNum).arg(senderPort).arg(message));
}

void MainWindow::displayJoinedPeer(quint16 senderPort) {
    // ui->receivedMessageBox->append(QString("<b>Peer %1 has joined the session!</b>").arg(senderPort));
}

void MainWindow::displayMessageHistory(QMap<int, QVariantMap> messageHistory) {
    ui->receivedMessageBox->clear();


    for (auto it = messageHistory.constBegin(); it != messageHistory.constEnd(); ++it) {
        int sequenceNum = it.value()["sequenceNum"].toInt();
        quint16 senderPort = it.value()["origin"].toUInt();
        QString message = it.value()["message"].toString();
        ui->receivedMessageBox->append(QString("<b>%1. Peer %2:</b> %3").arg(sequenceNum).arg(senderPort).arg(message));
    }
}

void MainWindow::on_printHistoryButton_clicked()
{
    qDebug() << "----------------------------------------------------";
    QMap<int, QVariantMap> messageHistory = udpHandler->messageHistory;
    for (auto it = messageHistory.constBegin(); it != messageHistory.constEnd(); ++it) {
        int k = it.key();
        int sequenceNum = it.value()["sequenceNum"].toInt();
        quint16 senderPort = it.value()["origin"].toUInt();
        QString message = it.value()["message"].toString();
        int tick = it.value()["clock"].toInt();
        qDebug() << k << "."<< "'" << sequenceNum << "'" << senderPort << ":" << message << "|" << tick;
    }
    qDebug() << "----------------------------------------------------";
}


void MainWindow::on_antiEntropyButton_clicked()
{
    udpHandler->requestHistoryFromNeighbors(udpHandler->getRandomNeighbor());
}
