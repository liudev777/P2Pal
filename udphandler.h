#ifndef UDPHANDLER_H
#define UDPHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QVector>
#include <QByteArray>
#include <QVariantMap>
#include <QMap>
#include <QTimer>
#include <QSet>

class UDPHandler : public QObject
{
    Q_OBJECT

public:
    explicit UDPHandler(QObject *parent = nullptr, quint16 port = 5000);
    ~UDPHandler();
    void sendIntro();
    void sendMessage(QString message);
    QMap<int, QVariantMap> messageHistory;
    quint16 myPort;
    int tick = 0;
    void requestHistoryFromNeighbors(quint16 neighborPort);
    quint16 getRandomNeighbor();
    QVector<quint16> myNeighbors;

private slots:
    void readyRead();

signals:
    void messageReceived(int sequenceNum, quint16 senderPort, QString message);
    void peerJoined(quint16 senderPort);
    void updatedHistory(QMap<int, QVariantMap> messageHistory);

private:
    void startTimer();
    void initNeighbors();
    QByteArray msg(QString message, quint16 origin, int sequenceNum = 0, QString type = "chat");
    void resendMessages(int sequenceNum);
    void sendAcknowledgement(quint16 senderPort, int sequenceNum);
    QByteArray serializeVariantMap(QVariantMap &messageMap);
    QVariantMap deserializeVariantMap(QByteArray &buffer);
    QByteArray serializeMessageHistory(QMap<int, QVariantMap> &messageHistory);
    QMap<int, QVariantMap> deserializeMessageHistory(QByteArray &data);
    void saveToHistory(QVariantMap messageMap);
    void requestHistoryFromNeighbors();
    void handleHistoryMessage(QByteArray data);
    void compareAndSelectHistory();
    void waitForHistories();
    void useHistory(QMap<int, QVariantMap>);
    void sendHistory(quint16 senderPort);
    QByteArray hstry(QMap<int, QVariantMap> messageHistory, quint16 origin, QString type);
    void propagateToNeighbors(QByteArray data, quint16 excludedNeighbor);
    void checkAndHandleHistoryStatus();
    void antiEntropy();

    void compareHistoryWithNeighbor();
    void startEntropyTimer();

    void compareHistoryAndUpdate(QVariantMap historyMap);
    void reconcileHistoryDifference(QMap<int, QVariantMap> messageHistory);
    void insertHistory(QMap<int, QVariantMap> &historyMap, int insertKey, QVariantMap incomingMessageMap);
    void updateClock(int tick);


private:
    QTimer *antiEntropyTimer;
    bool isUpToDate = false;
    QUdpSocket *socket;
    int sequenceNum = 1;
    QTimer *resendTimer;
    struct MessageInfo {
        QByteArray data;
        QSet<quint16> pendingNeighbors;
    };
    QMap<int, MessageInfo> pendingMessages;
    QMap<quint16, QMap<int, QVariantMap>> messageHistories;

};



#endif // UDPHANDLER_H
