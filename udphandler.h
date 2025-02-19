#ifndef UDPHANDLER_H
#define UDPHANDLER_H

#include <QObject>
#include <QUdpSocket>

class UDPHandler : public QObject
{
    Q_OBJECT

public:
    explicit UDPHandler(QObject *parent = nullptr, quint16 port = 5000);
    void SendIntro();

signals:
    void messageReceived();

private slots:
    void readyRead();

private:
    QUdpSocket *socket;
    quint16 myPort;
};



#endif // UDPHANDLER_H
