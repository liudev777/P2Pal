#ifndef UDPHANDLER_H
#define UDPHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QVector>

class UDPHandler : public QObject
{
    Q_OBJECT

public:
    explicit UDPHandler(QObject *parent = nullptr, quint16 port = 5000);
    void SendIntro();
    void SendMessage(QString message);

private slots:
    void readyRead();

private:
    void InitNeighbors();

private:
    QUdpSocket *socket;
    quint16 myPort;
    QVector<quint16> myNeighbors;
};



#endif // UDPHANDLER_H
