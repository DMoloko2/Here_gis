    #include "mainwindow.h"
    #include "ui_mainwindow.h"
    #include <QDebug>
    #include <QJsonDocument>
    #include <QJsonObject>
    #include <QJsonArray>



    MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
    {
        ui->setupUi(this);

        web=new QWebSocketServer("Server",QWebSocketServer::NonSecureMode);

        web->listen(QHostAddress::Any, 1796);

//        qDebug()<<web->sslConfiguration().protocol();
//        qDebug()<<web->supportedVersions();
        m_ptcpServer=new QTcpServer;
        connect(m_ptcpServer, SIGNAL(newConnection() ),this, SLOT(slotNewConnection())) ;


        sqlConnect();

        connect(web, SIGNAL(newConnection()),
                this, SLOT(onNewConnection()));



        //qDebug()<<vec;
        if (!m_ptcpServer->listen(QHostAddress::Any, 5545))
        {
        QMessageBox::critical(nullptr,
        "Server Error",
        "Unable to start the server:"
        + m_ptcpServer->errorString()) ;
        m_ptcpServer->close( );
        }

        refreshDB();



        QObject::connect(&soc, SIGNAL(connected()), SLOT(slotConnected()));
        QObject::connect(&soc, SIGNAL(readyRead()), SLOT(slotReadyRead()));
        QObject::connect(&soc, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(slotError(QAbstractSocket::SocketError))
        );



    }

    /*virtual*/ void MainWindow::slotNewConnection()            //слот для нового подлючения
    {
    QTcpSocket* pClientSocket = m_ptcpServer->nextPendingConnection();
    connect(pClientSocket, SIGNAL(disconnected()),pClientSocket, SLOT(deleteLater())) ;
    connect(pClientSocket, SIGNAL(readyRead()),this, SLOT(slotReadClient()));
    sendToClient(pClientSocket,"yes");
    }

    MainWindow::~MainWindow()
    {
        file.setFileName("file.txt");
        file.open(QIODevice::ReadWrite |  QIODevice::Truncate);
       QByteArray b;
       QString h;
        for (int o=0;o<12;o++)
        {
            h=QString::number(vec.at(o));
            h+="\n";
            b=h.toUtf8();

            file.write(b);
        }
        file.close();
        delete ui;
        delete web;
    }

    void MainWindow::reqq(QByteArray a)             //запрос в БД
    {
        qDebug()<<a;
        QNetworkRequest request(apiUrl);
        reply = manager.post(request, a);
        QObject::connect(reply, SIGNAL(finished()),this, SLOT(getReplyFinished()));
        QObject::connect(reply, SIGNAL(readyRead()), this, SLOT(readyReadReply()));
    }

    void MainWindow::getReplyFinished()             //после запроса в БД clicHouse
    {

        reply->deleteLater();
    }

    void MainWindow::onNewConnection()              //для websocketa
    {
        auto pSocket = web->nextPendingConnection();

        pSocket->setParent(this);

        qDebug()<<"s";
        connect(pSocket, &QWebSocket::textMessageReceived,
                this, &MainWindow::processMessage);
        connect(pSocket, &QWebSocket::disconnected,
                this, &MainWindow::socketDisconnected);





    }

    void MainWindow::slotReadClient()               //чтение данных из сокета с распредилителя
    {
        QTcpSocket* pClientSocket = (QTcpSocket*)sender();
        /*QDataStream in(pClientSocket);
        in.setVersion(QDataStream::Qt_5_3);
        for (;;) {
        if (!m_nNextBlockSize) {
        if (pClientSocket->bytesAvailable() < sizeof(quint16)) {
        break;
        }
        in>> m_nNextBlockSize;
        }
        if (pClientSocket->bytesAvailable() < m_nNextBlockSize) {
        break;
        }
        QString str;

        in>>  str;  //полученные данные

        sendToClient(pClientSocket,"yes");

        }*/

        QString data(pClientSocket->readAll().toStdString().data());

        qDebug()<<data;
        //qDebug()<<pClientSocket->readAll();
    }

    void MainWindow::readyReadReply()       //ответ на запрос в БД clickHouse
    {
        QString data(reply->readAll().toStdString().data());

        qDebug()<<data;

       /* QJsonDocument doc=QJsonDocument::fromJson(reply->readAll());
        QJsonArray ar=doc.array();


        ui->pushButton->setText(ar.at(0).toString());
        qDebug()<<ar.at(0).toString()<<ar.at(1)<<ar.at(2)<<ar.at(3);*/
    }

    void MainWindow::on_pushButton_clicked()
    {
        QString m;
        m= "http://";
        m+=ui->lineEdit->text();
        m+=":";
        QString k=QString::number(ui->spinBox->value());
        m+=k;
        m+="/?user=";
        m+=ui->lineEdit_3->text();
        m+="&password=";
        m+=ui->lineEdit_4->text();
        apiUrl.setUrl(m);
        this->hide();




        //reqq("CREATE TABLE test.opc (timestamp DateTime,typee Int16,param Int16,value Float32,prev Float32) ENGINE = MergeTree() ORDER BY (timestamp) SETTINGS index_granularity = 8192");
    }

    void MainWindow::sendToClient(QTcpSocket* pSocket, const QString& str)          //отправка клиенту сокета
    {
    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << quint16(0) << str;
    out.device()->seek(0);
    out << quint16(arrBlock.size()-sizeof(quint16));
    pSocket->write(arrBlock);
    }

    void MainWindow::sendTelega(QByteArray a)                                       //отправка даннаых в Телеграмм
    {

        soc.connectToHost("172.20.10.4",8888);
        soc.write(a);


    }

    bool MainWindow::sqlConnect()                                                   //Подключение к sql
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL3");

           db.setHostName("172.20.10.10");
           db.setDatabaseName("progdb");
           db.setUserName("root");
           db.setPassword("0000");
           bool ok = db.open();
           return ok;
    }

    void MainWindow::processMessage(const QString &message)
    {
        qDebug()<<message;
        if(message!="")
        {
            refreshDB();
        }
    }

    void MainWindow::slotReadyRead()                                                  //чтение при отправке на телегу
    {
        while(soc.bytesAvailable()>0)
           {
               qDebug()<<soc.readAll();
           }

        soc.close();
    }



void MainWindow::slotConnected()    //конект с телеграмом
{
    qDebug()<<"s";
}
void MainWindow::slotError(QAbstractSocket::SocketError err)            //ошибки при конекте с телегой
{
QString strError =
"Error: " + (err == QAbstractSocket::HostNotFoundError ?
"The host was not found." :
err == QAbstractSocket::RemoteHostClosedError ?
"The remote host is closed." :
err == QAbstractSocket::ConnectionRefusedError ?
"The connection was refused." :
QString(soc.errorString())
);
qDebug()<<strError;
}

void MainWindow::socketDisconnected()
{

}

void MainWindow::refreshDB()
{
    QSqlQuery query;
    int cb=ui->comboBox->currentIndex();
    QString j=QString::number(cb);
    QString l;
    l+="SELECT porog FROM temp WHERE type=";
    l+=j;
    query.exec(l);
    query.next();
    //qDebug()<<query.size();
    for(int e=0;e<query.size();e++)
    {
    vec.push_back(query.value(0).toInt());
    query.next();
    //qDebug()<<vec.at(e);
    }
}
