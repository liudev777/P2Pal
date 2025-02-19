#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, UDPHandler *udpHandler)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , udpHandler(udpHandler)
{
    ui->setupUi(this);

    connect(udpHandler, &UDPHandler::messageReceived, this, &MainWindow::displayReceivedMessage);
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

void MainWindow::displayReceivedMessage(quint16 senderPort, QString message) {
    ui->receivedMessageBox->append(QString("<b>Peer %1:</b> %2").arg(senderPort).arg(message));
}
