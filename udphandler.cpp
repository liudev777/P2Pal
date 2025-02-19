#include "udphandler.h"
#include<QUdpSocket>
#include<vector>

UDPHandler::UDPHandler(QObject *parent, quint16 port) : QObject(parent), myPort(port){
    qDebug() << "Creating UDPHandler instance on port" << port;

    // make and bind socket to address and port
    socket = new QUdpSocket(this);
    bool success = socket->bind(QHostAddress::LocalHost,myPort);

    while(!success && myPort < 5010) {
        qDebug() << "Error: Failed to bind UDP socket on port." << myPort << ". Port already in use. Trying port " << myPort + 1 << "instead.";
        myPort = myPort + 1;
        success = socket->bind(QHostAddress::LocalHost,myPort);
    }

    if (!success) {
        qDebug() << "Failed to bind to socket!";
        return;
    }

    qDebug() << "UDP socket successfully bound to port" << myPort;

    initNeighbors();

    // connect to signal and slot
    connect(socket,&QUdpSocket::readyRead, this, &UDPHandler::readyRead);
}

void UDPHandler::initNeighbors() {
    if (myPort == 5000) {
        myNeighbors.append(myPort + 1);
    } else if (myPort == 5009) {
        myNeighbors.append(myPort - 1);
    } else {
        myNeighbors.append(myPort - 1);
        myNeighbors.append(myPort + 1);
    }

    qDebug() << "My neighbors: " << myNeighbors;
}

// function to send out info
void UDPHandler::sendIntro() {
    QByteArray data;
    data.append(QString("Hello from port %1").arg(myPort).toUtf8());

    // write datagram to socket
    for (quint16 neighbor : myNeighbors) {
        socket->writeDatagram(data,QHostAddress::LocalHost,neighbor);
    }

}

// function to send out info
void UDPHandler::sendMessage(QString message) {
    if (myNeighbors.isEmpty()) {
        qDebug() << "Error: empty neighbor";
        return;
    }

    QByteArray data;
    data.append(message.toUtf8());

    // write datagram to socket
    for (quint16 neighbor : myNeighbors) {
        if (neighbor < 5000 || neighbor > 5009) {
            qDebug() << "Error: Invalid neighbor port" << neighbor;
            return;
        }
        socket->writeDatagram(data,QHostAddress::LocalHost,neighbor);
    }

}


void UDPHandler::readyRead() {
    QByteArray buffer;
    buffer.resize(socket->pendingDatagramSize()); // resize so we don't accidentally lose data

    // sender info
    QHostAddress sender;
    quint16 senderPort;

    // read datagram from socket
    socket->readDatagram(buffer.data(), buffer.size(), &sender, &senderPort);

    qDebug() << "Sender IP: " << sender.toString();
    qDebug() << "Sender Port: " << senderPort;
    qDebug() << "Message: " << buffer;

    QString message = QString::fromUtf8(buffer);
    emit messageReceived(senderPort, message);
}
