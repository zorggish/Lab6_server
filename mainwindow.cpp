#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlDatabase>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QIcon icon(":/icons/chat.ico");
    this->setWindowIcon(icon);

    QSettings settings;
    settings.sync();
    port = settings.value("port").toInt();
    ui->trayCheckBox->setChecked(settings.value("tray").toBool());
    ui->portLineEdit->setText(QString::number(port));

    qDebug() << "Tray is: " << settings.value("tray").toBool();

    connect(ui->enableButton, &QPushButton::clicked, this, &MainWindow::enableButtonClicked);
    connect(ui->saveSettingsButton, &QPushButton::clicked, this, &MainWindow::saveSettingsButtonClicked);

    for(int i = 0; i<10; i++)
        messages.push_back("");

    server = new QWebSocketServer(QStringLiteral("Echo Server"), QWebSocketServer::NonSecureMode, this);
    connect(server, &QWebSocketServer::newConnection, this, &MainWindow::processNewConnection);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(icon);
    QMenu *menu = new QMenu(this);

    QAction *switchVisibleAction = new QAction(trUtf8("Expand"), this);
    QAction *enableAction = new QAction(trUtf8("Enable"), this);
    QAction *disableAction = new QAction(trUtf8("Disable"), this);
    QAction *quitAction = new QAction(trUtf8("Exit"), this);

    connect(switchVisibleAction, SIGNAL(triggered()), this, SLOT(show()));
    connect(enableAction, SIGNAL(triggered()), this, SLOT(startServer()));
    connect(disableAction, SIGNAL(triggered()), this, SLOT(stopServer()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(closeProgram()));

    menu->addAction(switchVisibleAction);
    menu->addAction(enableAction);
    menu->addAction(disableAction);
    menu->addAction(quitAction);

    trayIcon->setContextMenu(menu);
    trayIcon->show();
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));


    db = QSqlDatabase::addDatabase("QSQLITE");
    //db = new QSqlDatabase(m_strFileName, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
    QString path = QCoreApplication::applicationDirPath() + "/db.db";
    db.setDatabaseName(path);
    if (!QFile::exists(path))
    {
        QFile::copy(":/database/empty.db", path);
        QFile file(path);
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        QFileDevice::Permissions p = QFile(path).permissions();
        p.setFlag(QFileDevice::WriteOwner, true);
        p.setFlag(QFileDevice::WriteGroup, true);
        p.setFlag(QFileDevice::WriteOwner, true);
        p.setFlag(QFileDevice::WriteUser, true);
        messageNumber=0;
        db.open();
    }
    else
    {
        QFile file(path);
        qDebug() << file.isWritable();

        db.open();
        QSqlQuery query;
        query.exec("SELECT COUNT(Number) FROM Messages");
        query.first();
        messageNumber = query.value(0).toInt();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateTextBox()
{
    /**
    * @brief Обновление текста на форме
    */
    QString text;
    for(int i = 0; i < 10; i++)
        text += messages[i] + "\n";
    ui->textEdit->setText(text);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    /**
    * @brief Вызывается при закрытии программы, закрывает сервер и базу данных либо сворачивает программу в трей
    * @param event Событие закрытия программы
    */
    if(ui->trayCheckBox->isChecked())
    {
        event->ignore();
        this->hide();
    }
    else
    {
        stopServer();
        db.close();
    }
}

void MainWindow::startServer()
{
    /**
    * @brief Запускает сервер
    */
    port = ui->portLineEdit->text().toInt();
    if(server->listen(QHostAddress::Any, port))
    {
        enabled=true;
        ui->statusLabel->setText("Status: enabled");
        ui->enableButton->setText("Disable");
    }
    else
    {
        QMessageBox::critical(0, "Critical", "Critical message text");
    }
}

void MainWindow::stopServer()
{
    /**
    * @brief Останавливает работу сервера
    */
    server->close();
    while(!clients.empty())
    {
        clients.last()->close(QWebSocketProtocol::CloseCodeNormal, "");
        clients.pop_back();
    }
    enabled=false;
    ui->statusLabel->setText("Status: disabled");
    ui->enableButton->setText("Enable");
}

void MainWindow::iconClicked(QSystemTrayIcon::ActivationReason reason)
{
    /**
    * @brief Обработчик нажания на иконку в трее
    */
    if(reason == QSystemTrayIcon::Trigger)
        this->show();
}

void MainWindow::enableButtonClicked()
{
    /**
    * @brief Обработчик кнопки включения сервера
    */
    if(!enabled)
        startServer();
    else
        stopServer();
}

void MainWindow::saveSettingsButtonClicked()
{
    /**
    * @brief Обработчик нажатия кнопки сохранения настроек
    */
    QSettings settings;
    port = ui->portLineEdit->text().toInt();
    settings.setValue("port", port);
    settings.setValue("tray", ui->trayCheckBox->isChecked());
    settings.sync();
}

void MainWindow::closeProgram()
{
    /**
    * @brief Закрывает программу
    */
    this->close();
}

void MainWindow::processNewConnection()
{
    /**
    * @brief Обрабатывает новое подключение к серверу
    */
    QWebSocket *pSocket = server->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &MainWindow::processTextMessage);

    clients << pSocket;

    QSqlQuery query;
    query.exec("SELECT * FROM Messages ORDER BY Number DESC LIMIT 10");

    QList<QString> messagesFromDatabase;
    while(query.next())
        messagesFromDatabase.push_back(query.value(1).toString());
    while(!messagesFromDatabase.empty()){
        pSocket->sendTextMessage(messagesFromDatabase.last());
        messagesFromDatabase.pop_back();
    }


    //for(int i = 0; i < messages.length(); i++)
    //    pSocket->sendTextMessage(messages[i]);
}

void MainWindow::processTextMessage(QString message)
{
    /**
    * @brief Обрабатывает полученное сообщение
    * @param message Сообщение
    */
    if(message == "disconnect")
    {
        QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
        pClient->close(QWebSocketProtocol::CloseCodeNormal, "");
        clients.removeOne(pClient);
    }
    else
    {
        messages.pop_front();
        messages.push_back(message);
        updateTextBox();

        qDebug() << "Got message";

        QSqlQuery query;
        qDebug() << "INSERT INTO Messages VALUES (" + QString::number(messageNumber+1) + ", '" + message + "');";
        qDebug() << query.exec("INSERT INTO Messages VALUES (" + QString::number(messageNumber+1) + ", '" + message + "');");
        qDebug() << query.lastError().text();
        messageNumber++;


        for(int i = 0; i < clients.size(); i++)
            clients[i]->sendTextMessage(message.toUtf8());
    }
}

