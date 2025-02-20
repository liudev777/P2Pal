#include "udphandler.h"
#include<QUdpSocket>
#include <QVariantMap>
#include <QDataStream>
#include <QByteArray>
#include <QSet>

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

QByteArray UDPHandler::serializeMessageHistory(QVector<QByteArray> &messageHistory) {
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream << messageHistory;
    return buffer;
}

QVector<QByteArray> UDPHandler::deserializeMessageHistory(QByteArray &buffer) {
    QVector<QByteArray> messageHistory;
    QDataStream stream(&buffer, QIODevice::ReadOnly);
    stream >> messageHistory;
    return messageHistory;
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

    requestHistoryFromNeighbors();


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

    saveToHistory(data);

    emit messageReceived(sequenceNum, myPort, message);

    pendingMessages.insert(sequenceNum, msgInfo);

    int localSequenceNum = sequenceNum;
    QTimer::singleShot(2000, this, [this, localSequenceNum]() {
        resendMessages(localSequenceNum);
    });

    // increment message num
    sequenceNum++;
}

// if peer doesn't acknowledge our message, we want to send it again. (1 try limit).
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

void UDPHandler::saveToHistory(QByteArray data) {
    messageHistory.append(data);
    return;
}

// store message, origin, and sequence number in QVariantMap
QByteArray UDPHandler::msg(QString message, quint16 origin, int sequenceNum, QString type) {
    // qDebug() << "message:" << message << "| origin:" << origin << "| sequence_number:" << sequenceNum << "| type:" << type;

    QVariantMap messageMap;
    messageMap["message"] = message;
    messageMap["origin"] = origin;
    messageMap["sequenceNum"] = sequenceNum;
    messageMap["type"] = type;

    QByteArray data = serializeVariantMap(messageMap);

    return data;
}

// function to pack messageHistory into a map with origin and type, then serialize it for sending.
QByteArray UDPHandler::hstry(QVector<QByteArray> messageHistory, quint16 origin, QString type) {
    QVariantMap historyMap;

    // its kinda weird but I will serialize the vector of bytearray and store it in messageHistoryData
    historyMap["messageHistoryData"] = serializeMessageHistory(messageHistory);
    historyMap["origin"] = origin;
    historyMap["type"] = type;

    QByteArray data = serializeVariantMap(historyMap);

    return data;
}

// sends an ack message to let sender know we got the message
void UDPHandler::sendAcknowledgement(quint16 senderPort, int sequenceNum) {
    QByteArray data = msg("", myPort, sequenceNum, "ack");
    socket->writeDatagram(data, QHostAddress::LocalHost, senderPort);
}

// when we receive a history, we want to wait and see if another history comes in. Do some comparison and pick the better history.
void UDPHandler::handleHistoryMessage(QByteArray data) {
    QVariantMap historyMap = deserializeVariantMap(data);
    quint16 origin = historyMap.value("origin").toUInt();

    QVariant rawData = historyMap.value("messageHistoryData");
    QByteArray serializedData = rawData.toByteArray();
    QVector<QByteArray> messageHistory = deserializeMessageHistory(serializedData);

    messageHistories[origin] = messageHistory;

    // if length of history is equal to length of neighbor, then compare which one has the longer history. (We might want to tell the other peer that they are behind if their length is shorter).
    if (messageHistories.size() == myNeighbors.size()) {
        compareAndSelectHistory();
    } else {
        waitForHistories();
    }

}

// we want the longer history (gonna assume its more up to date)
void UDPHandler::compareAndSelectHistory() {
    QVector<QByteArray> longestHistory;

    for (quint16 neighbor : myNeighbors) {
        if (messageHistories[neighbor].size() > longestHistory.size()) {
            longestHistory = messageHistories[neighbor];
        }
    }

    useHistory(longestHistory);
}

// take this history and update our history to this
void UDPHandler::useHistory(QVector<QByteArray> history) {
    for (QByteArray msgData : history) {
        QVariantMap messageMap = deserializeVariantMap(msgData);
        qDebug() << "Restored msg:" << messageMap;
    }
}

void UDPHandler::waitForHistories() {
    QTimer::singleShot(1000, this, &UDPHandler::compareAndSelectHistory);
}

// function to ask neighbors for their history log
void UDPHandler::requestHistoryFromNeighbors() {
    QByteArray data = msg("", myPort, -1, "request_history");

    for (quint16 neighbor : myNeighbors) {
        if (neighbor < 5000 || neighbor > 5009) {
            qDebug() << "Error: Invalid neighbor port" << neighbor;
            return;
        }
        socket->writeDatagram(data,QHostAddress::LocalHost,neighbor);
    }
}

void UDPHandler::sendHistory(quint16 senderPort) {
    qDebug() << "Sending my history to:" << senderPort;
    QByteArray historyData = hstry(messageHistory, myPort, "history");
    socket->writeDatagram(historyData, QHostAddress::LocalHost, senderPort);
}

// function to handle receiving signal from socket
void UDPHandler::readyRead() {
    QByteArray data;
    data.resize(socket->pendingDatagramSize()); // resize so we don't accidentally lose data

    // sender info
    QHostAddress sender;
    quint16 senderPort;

    // read datagram from socket
    socket->readDatagram(data.data(), data.size(), &sender, &senderPort);

    // qDebug() << "Sender IP:" << sender.toString();
    // qDebug() << "Sender Port:" << senderPort;
    // qDebug() << "Message:" << data;


    QVariantMap messageMap = deserializeVariantMap(data);

    QString type = messageMap.value("type").toString();
    quint16 origin = messageMap.value("origin").toUInt();

    if (type == "request_history") {
        sendHistory(origin);
        return;
    } else if (type == "history") {
        handleHistoryMessage(data);
        return;
    }


    QString message = messageMap.value("message").toString();
    int sequenceNum = messageMap.value("sequenceNum").toInt();

    if (type == "chat") {
        qDebug() << "Received message:" << messageMap;

        // let sender know we got the msg
        sendAcknowledgement(senderPort, sequenceNum);

        this->sequenceNum = sequenceNum + 1;
        saveToHistory(data);
        // qDebug() << "Our sequenceNum" << this->sequenceNum << "sender sequenceNum" << sequenceNum;

        // emits signal that gets picked up by receivedMessageBox and displayed
        emit messageReceived(sequenceNum, origin, message);
    } else if (type == "ack") {
        qDebug() << "Received acknowledgement for message" << sequenceNum << "from" << origin;

        // since we got the ack, we don't want to resend them the message
        if (pendingMessages.contains(sequenceNum)) {
            pendingMessages[sequenceNum].pendingNeighbors.remove(senderPort);
        }
    } else if (type == "ping") {
        qDebug() << "Peer" << origin << "joined the session";

        emit peerJoined(senderPort);
    }

}
