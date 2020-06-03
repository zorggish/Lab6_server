#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QMessageBox>
#include <QQueue>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QAction>
#include <QStyle>
#include <QSettings>
#include <QtSql>
#include <QSqlDatabase>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QWebSocketServer *server;
    QList<QWebSocket*> clients;
    QQueue<QString> messages;
    bool enabled = false;
    QSystemTrayIcon *trayIcon;
    int port = 1234;
    QSqlDatabase db;
    int messageNumber;
    void closeEvent(QCloseEvent * event);
    void updateTextBox();

private Q_SLOTS:
    void startServer();
    void stopServer();
    void iconClicked(QSystemTrayIcon::ActivationReason reason);
    void enableButtonClicked();
    void saveSettingsButtonClicked();
    void closeProgram();
    void processNewConnection();
    void processTextMessage(QString message);
};
#endif // MAINWINDOW_H
