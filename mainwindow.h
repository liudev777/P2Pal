#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "udphandler.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr, UDPHandler *udpHandler = nullptr);
    ~MainWindow();

private slots:
    void on_sendButton_clicked();
    void displayMessage(int sequenceNum, quint16 senderPort, QString message);
    void displayJoinedPeer(quint16 senderPort);
    void displayMessageHistory(QMap<int, QVariantMap> messageHistory);

    void on_printHistoryButton_clicked();

    void on_antiEntropyButton_clicked();

private:
    Ui::MainWindow *ui;
    UDPHandler *udpHandler;

};
#endif // MAINWINDOW_H
