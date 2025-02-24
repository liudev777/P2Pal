#include "udphandler.h"
#include "QRandomGenerator"

quint16 UDPHandler::getRandomNeighbor() {
    quint16 randomNeighbor = myNeighbors[QRandomGenerator::global()->bounded(myNeighbors.size())];
    return randomNeighbor;
}

// checks to see if our history is the same as our neighbors history
void UDPHandler::compareHistoryWithNeighbor() {
    qDebug() << "Mock comparing history" << getRandomNeighbor();

}

void UDPHandler::startEntropyTimer() {
    QTimer *timer = new QTimer();

    QObject::connect(timer, &QTimer::timeout, [&]() {
        requestHistoryFromNeighbors(getRandomNeighbor());
    });

    timer->start(10000);
}

void UDPHandler::antiEntropy() {
    startEntropyTimer();
    return;
}


// function to ask specified neighbor for their history log
void UDPHandler::requestHistoryFromNeighbors(quint16 neighborPort) {
    QByteArray data = msg("", myPort, -1, "request_history");

    socket->writeDatagram(data,QHostAddress::LocalHost,neighborPort);

}

void UDPHandler::compareHistoryAndUpdate(QVariantMap historyMap) {
    // quint16 origin = historyMap.value("origin").toUInt();

    QVariant rawData = historyMap.value("messageHistoryData");
    QByteArray serializedData = rawData.toByteArray();
    QMap<int, QVariantMap> incomingHistory = deserializeMessageHistory(serializedData);
    reconcileHistoryDifference(incomingHistory);
}

void UDPHandler::insertHistory(QMap<int, QVariantMap> &historyMap, int insertKey, QVariantMap incomingMessageMap) {
    int incomingClock = incomingMessageMap["clock"].toInt();
    if (incomingClock == 0) {
        return;
    }

    if (historyMap.contains(insertKey)) {
        QVariantMap existingMessage = historyMap[insertKey];
        int existingClock = existingMessage["clock"].toInt();

        QMap<int, QVariantMap> updatedMap;

        // determine correct placement
        if (incomingClock < existingClock) {
            // incoming message has a lower clock, take the current sequence and increment the rest.
            updatedMap[insertKey] = incomingMessageMap;
            updatedMap[insertKey]["sequenceNum"] = insertKey; // since we keep track of sequenceNum in the map, we also want to increment it so there ins't a mismatch between the key and stored seq num.

            for (auto it = historyMap.constEnd(); it != historyMap.constBegin();) {
                --it;
                int oldKey = it.key();
                QVariantMap value = it.value();

                if (oldKey >= insertKey) {
                    value["sequenceNum"] = oldKey + 1;
                    updatedMap[oldKey + 1] = value;
                } else {
                    updatedMap[oldKey] = value;
                }
            }
        } else {
            // incoming message has higher clock, goes after the current sequence and increment the rest.
            for (auto it = historyMap.constEnd(); it != historyMap.constBegin();) {
                --it;
                int oldKey = it.key();
                QVariantMap value = it.value();

                if (oldKey > insertKey) {
                    value["sequenceNum"] = oldKey + 1;
                    updatedMap[oldKey + 1] = value;
                } else {
                    updatedMap[oldKey] = value;
                }
            }

            // insert the incoming message at insertKey+1
            incomingMessageMap["sequenceNum"] = insertKey + 1;
            updatedMap[insertKey + 1] = incomingMessageMap;
        }

        historyMap = updatedMap;
    } else {
        // insert normally
        incomingMessageMap["sequenceNum"] = insertKey;
        historyMap[insertKey] = incomingMessageMap;
    }
}

void UDPHandler::reconcileHistoryDifference(QMap<int, QVariantMap> incomingHistory) {
    qDebug() << "Reconciling history differences...";

    QVector<int> conflicts;

    // find largest seq between the two histories
    int latestSequenceNum = messageHistory.size();
    if (incomingHistory.size() > latestSequenceNum) {
        latestSequenceNum = incomingHistory.size();
    }

    // first pass: we want to identify the lines that have different message for its seq, ie 2. 'first and 2. 'second'
    for (int i = 1; i <= latestSequenceNum; i++) {
        if (incomingHistory.contains(i) && messageHistory.contains(i)) {

            QVariantMap incomingMessage = incomingHistory[i];
            QVariantMap localMessage = messageHistory[i];

            bool isSameMessage =
                incomingMessage["origin"] == localMessage["origin"] &&
                incomingMessage["message"] == localMessage["message"] &&
                incomingMessage["clock"] == localMessage["clock"];

            // add conflicting message to list
            if (!isSameMessage) {
                conflicts.append(i);
            }
        } else if (incomingHistory.contains(i)) {
            // add the extra messages we don't have to conflict list as well
            conflicts.append(i);
        }
    }

    // second pass: iterate through the conflict list and process difference and reconcile
    for (int seqNum : conflicts) {
        bool isExistingMessage = false;
        QVariantMap incomingMessage = incomingHistory[seqNum];
        int incomingClock = incomingMessage["clock"].toInt();

        int targetPosition = seqNum;
        // the idea is that if two lines with same seq are conflicting, there could be more than 1 messages after it that still have lower ticks (came before) the conflicting line. so we want to keep going up the message history to place the message right before another message with a higher tick.
        for (int j = seqNum + 1; j <= messageHistory.size(); j++) {
            int localClock = messageHistory[j]["clock"].toInt();
            if (incomingClock > localClock) {
                targetPosition = j;
            } else if (incomingClock == localClock) {
                QVariantMap localMessage = messageHistory[j];

                isExistingMessage =
                    incomingMessage["origin"] == localMessage["origin"] &&
                    incomingMessage["message"] == localMessage["message"] &&
                    incomingMessage["clock"] == localMessage["clock"];
                break;
            } else {
                break;  // stop when we find a bigger clock
            }
        }

        // insert message at the correct position
        QVariantMap localMessage = messageHistory.value(targetPosition);
        bool isExistingMessage2 =
            incomingMessage["origin"] == localMessage["origin"] &&
            incomingMessage["message"] == localMessage["message"] &&
            incomingMessage["clock"] == localMessage["clock"];
        if (isExistingMessage || isExistingMessage2) {
            continue;
        }

        if (!isExistingMessage) {
            insertHistory(messageHistory, targetPosition, incomingMessage);
        }

    }
    emit updatedHistory(messageHistory);
}



