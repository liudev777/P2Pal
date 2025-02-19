#include "udphandler.h"
#include<QUdpSocket>

UDPHandler::UDPHandler(QObject *parent, quint16 port) : QObject(parent), myPort(port){
    qDebug() << "Creating UDPHandler instance on port" << port;

    // make and bind socket to address and port
    socket = new QUdpSocket(this);
    bool success = socket->bind(QHostAddress::LocalHost,myPort);

    while(!success && myPort < 5010) {
        qDebug() << "Error: Failed to bind UDP socket on port." << port << ". Port already in use. Trying port " << myPort + 1 << "instead.";
        myPort = myPort + 1;
        success = socket->bind(QHostAddress::LocalHost,myPort);
    }

    qDebug() << "UDP socket successfully bound to port" << myPort;


    // connect to signal and slot
    connect(socket,&QUdpSocket::readyRead, this, &UDPHandler::readyRead);
}

// function to send out info
void UDPHandler::SendIntro() {
    QByteArray data;
    data.append(QString("Hello from port %1").arg(myPort).toUtf8());

    // write datagram to socket
    socket->writeDatagram(data,QHostAddress::LocalHost,5000);

}

void UDPHandler::readyRead() {
    QByteArray buffer;
    buffer.resize(socket->pendingDatagramSize()); // resize so we don't accidentally lose data

    // sender info
    QHostAddress sender;
    quint16 senderPort;

    // read datagram from socket
    socket->readDatagram(buffer.data(), buffer.size(), &sender, &senderPort);

    qDebug() << "Message from: " << sender.toString();
    qDebug() << "Message port: " << senderPort;
    qDebug() << "Message: " << buffer;
}
