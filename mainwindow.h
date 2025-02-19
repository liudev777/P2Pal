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
    void displayReceivedMessage(quint16 senderPort, QString message);

private:
    Ui::MainWindow *ui;
    UDPHandler *udpHandler;

};
#endif // MAINWINDOW_H
