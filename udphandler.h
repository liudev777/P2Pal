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
    void resendMessages(int sequenceNum);
    void sendAcknowledgement(quint16 senderPort, int sequenceNum);
    QByteArray serializeVariantMap(QVariantMap &messageMap);
    QVariantMap deserializeVariantMap(QByteArray &buffer);
    int sequenceNum = 0;

    struct MessageInfo {
        QByteArray data;
        QSet<quint16> pendingNeighbors;
    };

private slots:
    void readyRead();

signals:
    void messageReceived(quint16 senderPort, QString message);

private:
    void initNeighbors();
    QByteArray msg(QString message, quint16 origin, int sequenceNum = 0, QString type = "chat");
    QMap<int, MessageInfo> pendingMessages;
    QTimer *resendTimer;

private:
    QUdpSocket *socket;
    quint16 myPort;
    QVector<quint16> myNeighbors;
};



#endif // UDPHANDLER_H
