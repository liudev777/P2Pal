#ifndef UDPHANDLER_H
#define UDPHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QVector>
#include <QByteArray>
#include <QVariantMap>

class UDPHandler : public QObject
{
    Q_OBJECT

public:
    explicit UDPHandler(QObject *parent = nullptr, quint16 port = 5000);
    void sendIntro();
    void sendMessage(QString message);
    QByteArray serializeVariantMap(QVariantMap &messageMap);
    QVariantMap deserializeVariantMap(QByteArray &buffer);

private slots:
    void readyRead();

signals:
    void messageReceived(quint16 senderPort, QString message);

private:
    void initNeighbors();
    QVariantMap msg(QString message, quint16 origin, int sequenceNum = 0);

private:
    QUdpSocket *socket;
    quint16 myPort;
    QVector<quint16> myNeighbors;
};



#endif // UDPHANDLER_H
