#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, UDPHandler *udpHandler)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , udpHandler(udpHandler)
{
    ui->setupUi(this);
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

    ui->receivedMessageBox->append(message);

    if (udpHandler) {
        udpHandler->SendMessage(message);
    }

    ui->editMessageBox->clear();

}

