#pragma once

#include <QObject>

class QTcpServer;
class QTcpSocket;

class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(QObject *parent = nullptr);
    ~TcpServer() override;

public slots:
    bool start(quint16 port);
    void stop();

signals:
    void incomingSocket(QTcpSocket *socket);
    void errorOccurred(QString message);

private:
    QTcpServer *m_server = nullptr;
};
