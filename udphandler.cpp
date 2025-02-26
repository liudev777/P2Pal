#include "udphandler.h"
#include<QUdpSocket>
#include <QVariantMap>
#include <QDataStream>
#include <QByteArray>
#include <QSet>
#include <QThread>

UDPHandler::UDPHandler(QObject *parent, quint16 port) : QObject(parent), myPort(port), antiEntropyTimer(nullptr){

    // make and bind socket to address and port
    socket = new QUdpSocket(this);
    bool success = socket->bind(QHostAddress::LocalHost,myPort);

    // idea here is that we want the peer to take the next available port, and keep trying until it does
    while(!success && myPort < 5010) {
        myPort = myPort + 1;
        success = socket->bind(QHostAddress::LocalHost,myPort);
    }

    if (!success) {
        // qDebug() << "Failed to bind to socket!";
        qFatal("Failed to bind to a port");
    }

    qDebug() << "UDP socket successfully bound to port" << myPort;

    // found our neighbor ports
    initNeighbors();

    // connect to signal and slot
    connect(socket,&QUdpSocket::readyRead, this, &UDPHandler::readyRead);


    // if after a second passes and no history updates from neighbors comes in, we can assume we are the first peer, and thus we set isUpToDate to true to receive messages from future peers.
    // previously the first peer would be unable to receive messages because this was set to false.
    QTimer::singleShot(1000, [&]() {
        isUpToDate = true;
    });

    startTimer(); // timer for our tick. I have it set to 1/100th of a second
    antiEntropy();
}

// clock tick timer
void UDPHandler::startTimer() {
    QTimer *timer = new QTimer(this);

    QObject::connect(timer, &QTimer::timeout, this, [this]() {
        tick++;
    });

    timer->start(10);
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

    // qDebug() << "My neighbors:" << myNeighbors;
}

// i know the bottom 4 serialize and deserialize are kind of redundant but I couldn't figure out how to make them return as different types from one class
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

QByteArray UDPHandler::serializeMessageHistory(QMap<int, QVariantMap> &messageHistory) {
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream << messageHistory;
    return buffer;
}

QMap<int, QVariantMap> UDPHandler::deserializeMessageHistory(QByteArray &buffer) {
    QMap<int, QVariantMap> messageHistory;
    QDataStream stream(&buffer, QIODevice::ReadOnly);
    stream >> messageHistory;
    return messageHistory;
}


// function to send out info
void UDPHandler::sendIntro() {
    // ping neighbor peer to let them know you joined
    // QByteArray pingData = msg("", myPort, -1, "ping");

    // for (quint16 neighbor : myNeighbors) {
    //     if (neighbor < 5000 || neighbor > 5009) {
    //         qDebug() << "Error: Invalid neighbor port" << neighbor;
    //         return;
    //     }
    //     socket->writeDatagram(pingData,QHostAddress::LocalHost,neighbor);
    // }

    // Neighbors (1-2) will send you a copy of their history which you will compare and update yours against.
    requestHistoryFromNeighbors();
}

// function to send out message
void UDPHandler::sendMessage(QString message) {
    if (myNeighbors.isEmpty()) {
        // qDebug() << "Error: empty neighbor";
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
        // we will temporary store the sent info so we can resend if they didn't receive it the first time
        msgInfo.pendingNeighbors.insert(neighbor);
    }

    QVariantMap messageMap = deserializeVariantMap(data);
    saveToHistory(messageMap);

    // here, message isn't actually being received, but this signal double as update local chat history so I will use it here. (too lazy to rename).
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
        // qDebug() << "Resending message" << sequenceNum << "to neighbors:" << msgInfo.pendingNeighbors;

        QByteArray data = msgInfo.data;

        for (quint16 neighbor : msgInfo.pendingNeighbors) {
            socket->writeDatagram(data, QHostAddress::LocalHost,neighbor);
        }

        // qDebug() << "Max resend attempts reached for message" << sequenceNum;
        pendingMessages.remove(sequenceNum);
    } else {
        // qDebug() << "All neighbors received message" << sequenceNum;
        pendingMessages.remove(sequenceNum);
    }

    return;
}

void UDPHandler::saveToHistory(QVariantMap messageMap) {
    int sequenceNum = messageMap["sequenceNum"].toInt();
    messageHistory[sequenceNum] = messageMap;
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
    messageMap["clock"] = tick;

    QByteArray data = serializeVariantMap(messageMap);

    return data;
}

// function to pack messageHistory into a map with origin and type, then serialize it for sending.
QByteArray UDPHandler::hstry(QMap<int, QVariantMap> messageHistory, quint16 origin, QString type) {
    QVariantMap historyMap;

    // its kinda weird but I will serialize the vector of bytearray and store it in messageHistoryData
    historyMap["messageHistoryData"] = serializeMessageHistory(messageHistory);
    historyMap["origin"] = origin;
    historyMap["type"] = type;
    historyMap["clock"] = tick;

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
    QMap<int, QVariantMap> messageHistory = deserializeMessageHistory(serializedData);

    messageHistories[origin] = messageHistory;

    // if length of history is equal to length of neighbor, then compare which one has the longer history. (We might want to tell the other peer that they are behind if their length is shorter).
    if (messageHistories.size() == myNeighbors.size()) {
        compareAndSelectHistory();
    } else {
        // wait a little bit for the other history (if there is) to come in before comparing
        waitForHistories();
    }

}

// we want the longer history (gonna assume its more up to date)
void UDPHandler::compareAndSelectHistory() {
    // qDebug() << "Received histories from" << messageHistories.keys();
    QMap<int, QVariantMap> longestHistory;
    quint16 longestHistoryNeighbor = 0;

    for (quint16 neighbor : myNeighbors) {
        if (messageHistories[neighbor].size() > longestHistory.size()) {
            longestHistory = messageHistories[neighbor];
            longestHistoryNeighbor = neighbor;
        }
    }

    if (longestHistoryNeighbor != 0) {
        qDebug() << "Choose history from" << longestHistoryNeighbor;
    } else {
        qDebug() << "Got an empty history";
    }

    useHistory(longestHistory);
}

void UDPHandler::updateClock(int tick) {
    this->tick = tick;
}

// take this history and update our history to this
void UDPHandler::useHistory(QMap<int, QVariantMap> history) {
    // for (QByteArray msgData : history) {
    //     QVariantMap messageMap = deserializeVariantMap(msgData);
    //     qDebug() << "Restored msg:" << messageMap;
    // }

    int highestSequence = -1;

    for (auto it = history.constBegin(); it != history.constEnd(); ++it) {
        // qDebug() << "Key:" << it.key() << ", Value:" << it.value();
        if (it.key() > highestSequence) {
            highestSequence = it.key();
        }
    }
    messageHistory = history;

    // if history is empty, we don't want to increment sequenceNum, this will lead to skipping sequences.
    if (highestSequence != -1) {
        sequenceNum = highestSequence + 1;
        QVariantMap lastMessageMap = history.last();
        int latestTick = lastMessageMap.value("clock").toInt();
        updateClock(latestTick);
    }

    // emit signal to display all the histories
    isUpToDate = true;
    emit updatedHistory(messageHistory);
}

void UDPHandler::checkAndHandleHistoryStatus() {
    if (messageHistories.size() == myNeighbors.size()) {
        return;
    } else {
        compareAndSelectHistory();
    }
}

void UDPHandler::waitForHistories() {
    QTimer::singleShot(1000, this, &UDPHandler::checkAndHandleHistoryStatus);
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
    // qDebug() << "Sending my history to:" << senderPort;
    QByteArray historyData = hstry(messageHistory, myPort, "history");
    socket->writeDatagram(historyData, QHostAddress::LocalHost, senderPort);
}

void UDPHandler::propagateToNeighbors(QByteArray data, quint16 excludedNeighbor) {

    MessageInfo msgInfo;
    msgInfo.data = data;

    // write datagram to socket
    for (quint16 neighbor : myNeighbors) {
        if (neighbor < 5000 || neighbor > 5009) {
            qDebug() << "Error: Invalid neighbor port" << neighbor;
            return;
        }

        if (neighbor == excludedNeighbor) continue;

        socket->writeDatagram(data,QHostAddress::LocalHost,neighbor);
        msgInfo.pendingNeighbors.insert(neighbor);
    }

    QVariantMap messageMap = deserializeVariantMap(data);

    int localSequenceNum = messageMap.value("sequenceNum").toInt();

    pendingMessages.insert(localSequenceNum, msgInfo);

    QTimer::singleShot(2000, this, [this, localSequenceNum]() {
        resendMessages(localSequenceNum);
    });

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

    QVariantMap messageMap = deserializeVariantMap(data);

    QString type = messageMap.value("type").toString();
    quint16 origin = messageMap.value("origin").toUInt();

    if (type == "request_history") {
        sendHistory(origin);
        return;
    } else if (type == "history") {
        if (!isUpToDate) {
            handleHistoryMessage(data);
        } else {
            // qDebug() << "history for anti entropy from" << origin << "received!";
            compareHistoryAndUpdate(messageMap);
        }

        return;
    }

    QString message = messageMap.value("message").toString();
    int sequenceNum = messageMap.value("sequenceNum").toInt();

    if (type == "chat") {
        if (!isUpToDate) return;
        // qDebug() << "Received message:" << messageMap;

        // let sender know we got the msg
        sendAcknowledgement(senderPort, sequenceNum);
        propagateToNeighbors(data, senderPort);

        // temp solution to prevent the same message from coming in after history updates including that message
        if (messageHistory.contains(sequenceNum)) return;

        this->sequenceNum = sequenceNum + 1;
        saveToHistory(messageMap);
        // qDebug() << "Our sequenceNum" << this->sequenceNum << "sender sequenceNum" << sequenceNum;

        // emits signal that gets picked up by receivedMessageBox and displayed
        emit messageReceived(sequenceNum, origin, message);
    } else if (type == "ack") {
        // qDebug() << "Received acknowledgement for message" << sequenceNum << "from" << origin;

        // since we got the ack, we don't want to resend them the message
        if (pendingMessages.contains(sequenceNum)) {
            pendingMessages[sequenceNum].pendingNeighbors.remove(senderPort);
        }
    } else if (type == "ping") {
        qDebug() << "Peer" << origin << "joined the session";

        emit peerJoined(senderPort);
    }

}
