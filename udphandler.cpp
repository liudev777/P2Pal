#include "udphandler.h"
#include<QUdpSocket>
#include<QVariantMap>
#include<QDataStream>
#include<QByteArray>
#include<QSet>

UDPHandler::UDPHandler(QObject *parent, quint16 port) : QObject(parent), myPort(port){

    // make and bind socket to address and port
    socket = new QUdpSocket(this);
    bool success = socket->bind(QHostAddress::LocalHost,myPort);

    while(!success && myPort < 5010) {
        qDebug() << "Error: Failed to bind UDP socket on port." << myPort << ". Port already in use. Trying port" << myPort + 1 << "instead.";
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

struct MessageInfo {
    QByteArray data;
    QSet<quint16> pendingNeighbors;
};

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

    qDebug() << "My neighbors:" << myNeighbors;
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
    // ping neighbor peer to let them know you joined
    QByteArray data = msg("", myPort, -1, "ping");

    for (quint16 neighbor : myNeighbors) {
        if (neighbor < 5000 || neighbor > 5009) {
            qDebug() << "Error: Invalid neighbor port" << neighbor;
            return;
        }
        socket->writeDatagram(data,QHostAddress::LocalHost,neighbor);
    }

    QString message = QString("Hello from port %1").arg(myPort);
    sendMessage(message);

}

// function to send out info
void UDPHandler::sendMessage(QString message) {
    if (myNeighbors.isEmpty()) {
        qDebug() << "Error: empty neighbor";
        return;
    }

    QByteArray data = msg(message, myPort, sequenceNum);
    MessageInfo msgInfo;
    msgInfo.data = data;

    // write datagram to socket
    for (quint16 neighbor : myNeighbors) {
        if (neighbor < 5000 || neighbor > 5009) {
            qDebug() << "Error: Invalid neighbor port" << neighbor;
            return;
        }
        socket->writeDatagram(data,QHostAddress::LocalHost,neighbor);
        msgInfo.pendingNeighbors.insert(neighbor);
    }

    pendingMessages.insert(sequenceNum, msgInfo);

    int localSequenceNum = sequenceNum;
    QTimer::singleShot(2000, this, [this, localSequenceNum]() {
        resendMessages(localSequenceNum);
    });

    // increment message num
    sequenceNum++;
}

void UDPHandler::resendMessages(int sequenceNum) {
    if (!pendingMessages.contains(sequenceNum)) return;

    MessageInfo &msgInfo = pendingMessages[sequenceNum];

    if (!msgInfo.pendingNeighbors.isEmpty()) {
        qDebug() << "Resending message" << sequenceNum << "to neighbors:" << msgInfo.pendingNeighbors;

        QByteArray data = msgInfo.data;

        for (quint16 neighbor : msgInfo.pendingNeighbors) {
            socket->writeDatagram(data, QHostAddress::LocalHost,neighbor);
        }

        qDebug() << "Max resend attempts reached for message" << sequenceNum;
        pendingMessages.remove(sequenceNum);
    } else {
        qDebug() << "All neighbors received message" << sequenceNum;
        pendingMessages.remove(sequenceNum);
    }

    return;
}

// store message, origin, and sequence number in QVariantMap
QByteArray UDPHandler::msg(QString message, quint16 origin, int sequenceNum, QString type) {
    qDebug() << "message:" << message << "origin:" << origin << "sequence number:" << sequenceNum;

    QVariantMap messageMap;
    messageMap["message"] = message;
    messageMap["origin"] = origin;
    messageMap["sequenceNum"] = sequenceNum;
    messageMap["type"] = type;

    QByteArray data = serializeVariantMap(messageMap);

    return data;
}

// sends an ack message to let sender know we got the message
void UDPHandler::sendAcknowledgement(quint16 senderPort, int sequenceNum) {
    QByteArray data = msg("", myPort, sequenceNum, "ack");
    socket->writeDatagram(data, QHostAddress::LocalHost, senderPort);
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

    // qDebug() << "Sender IP:" << sender.toString();
    // qDebug() << "Sender Port:" << senderPort;
    // qDebug() << "Message:" << buffer;

    QVariantMap messageMap = deserializeVariantMap(buffer);

    QString type = messageMap.value("type").toString();
    QString message = messageMap.value("message").toString();
    quint16 origin = messageMap.value("origin").toUInt();
    int sequenceNum = messageMap.value("sequenceNum").toInt();

    if (type == "chat") {
        qDebug() << "Received message:" << messageMap;

        // let sender know we got the msg
        sendAcknowledgement(senderPort, sequenceNum);

        // emits signal that gets picked up by receivedMessageBox and displayed
        emit messageReceived(origin, message);
    } else if (type == "ack") {
        qDebug() << "Received acknowledgement for message" << sequenceNum << "from" << origin;

        // since we got the ack, we don't want to resend them the message
        if (pendingMessages.contains(sequenceNum)) {
            pendingMessages[sequenceNum].pendingNeighbors.remove(senderPort);
        }
    } else if (type == "ping") {

        emit peerJoined(senderPort);
    }

}
