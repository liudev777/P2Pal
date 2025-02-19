#include "udphandler.h"
#include<QUdpSocket>
#include<QVariantMap>
#include<QDataStream>
#include<QByteArray>

UDPHandler::UDPHandler(QObject *parent, quint16 port) : QObject(parent), myPort(port){

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

// find neighbors
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

QByteArray UDPHandler::serializeVariantMap(QVariantMap &messageMap) {
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream << messageMap;
    return buffer;
}

QVariantMap UDPHandler::deserializeVariantMap(QByteArray &buffer) {
    QVariantMap messageMap;
    QDataStream stream(buffer);
    stream >> messageMap;
    return messageMap;
}

// function to send out info
void UDPHandler::sendIntro() {
    QString message = QString("Hello from port %1").arg(myPort);

    QVariantMap messageMap = msg(message, myPort, 0);

    QByteArray data = serializeVariantMap(messageMap);

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

// store message, origin, and sequence number in QVariantMap
QVariantMap UDPHandler::msg(QString message, quint16 origin, int sequenceNum) {
    qDebug() << "message: " << message << "origin: " << origin << "sequence number: " << sequenceNum;
    QVariantMap messageMap;
    messageMap["message"] = message;
    messageMap["origin"] = origin;
    messageMap["sequenceNum"] = sequenceNum;
    return messageMap;
}

// function to handle receiving signal from socket
void UDPHandler::readyRead() {
    QByteArray buffer;
    buffer.resize(socket->pendingDatagramSize()); // resize so we don't accidentally lose data

    // sender info
    QHostAddress sender;
    quint16 senderPort;

    // read datagram from socket
    socket->readDatagram(buffer.data(), buffer.size(), &sender, &senderPort);

    // qDebug() << "Sender IP: " << sender.toString();
    // qDebug() << "Sender Port: " << senderPort;
    // qDebug() << "Message: " << buffer;

    QVariantMap messageMap = deserializeVariantMap(buffer);

    qDebug() << "map: " << messageMap;

    QString message = messageMap.value("message").toString();
    quint16 origin = messageMap.value("origin").toUInt();

    // emits signal that gets picked up by receivedMessageBox and displayed
    emit messageReceived(origin, message);
}
