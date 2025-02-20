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
    void sendIntro();
    void sendMessage(QString message);

private slots:
    void readyRead();

signals:
    void messageReceived(int sequenceNum, quint16 senderPort, QString message);
    void peerJoined(quint16 senderPort);

private:
    void initNeighbors();
    QByteArray msg(QString message, quint16 origin, int sequenceNum = 0, QString type = "chat");
    void resendMessages(int sequenceNum);
    void sendAcknowledgement(quint16 senderPort, int sequenceNum);
    QByteArray serializeVariantMap(QVariantMap &messageMap);
    QVariantMap deserializeVariantMap(QByteArray &buffer);
    QByteArray serializeMessageHistory(QVector<QByteArray> &messageHistory);
    QVector<QByteArray> deserializeMessageHistory(QByteArray &data);
    void saveToHistory(QByteArray data);
    void requestHistoryFromNeighbors();
    void handleHistoryMessage(QByteArray data);
    void compareAndSelectHistory();
    void waitForHistories();
    void useHistory(QVector<QByteArray>);
    void sendHistory(quint16 senderPort);
    QByteArray hstry(QVector<QByteArray> messageHistory, quint16 origin, QString type);

private:
    QUdpSocket *socket;
    quint16 myPort;
    QVector<quint16> myNeighbors;
    int sequenceNum = 0;
    QTimer *resendTimer;
    struct MessageInfo {
        QByteArray data;
        QSet<quint16> pendingNeighbors;
    };
    QMap<int, MessageInfo> pendingMessages;
    QVector<QByteArray> messageHistory;
    QMap<quint16, QVector<QByteArray>> messageHistories;

};



#endif // UDPHANDLER_H
