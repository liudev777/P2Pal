#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, UDPHandler *udpHandler)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , udpHandler(udpHandler)
{
    ui->setupUi(this);

    ui->editMessageBox->setFocus(); // autofocus on launch

    connect(udpHandler, &UDPHandler::messageReceived, this, &MainWindow::displayMessage);
    connect(udpHandler, &UDPHandler::peerJoined, this, &MainWindow::displayJoinedPeer);
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
    ui->receivedMessageBox->append(QString("<b>Peer %1 has joined the session!</b>").arg(senderPort));
}
