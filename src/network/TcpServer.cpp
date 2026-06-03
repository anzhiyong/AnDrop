#include "network/TcpServer.h"

#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

TcpServer::TcpServer(QObject *parent)
    : QObject(parent)
{
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start(quint16 port)
{
    stop();

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, [this]() {
        while (m_server && m_server->hasPendingConnections()) {
            QTcpSocket *socket = m_server->nextPendingConnection();
            socket->setParent(nullptr);
            emit incomingSocket(socket);
        }
    });

    if (!m_server->listen(QHostAddress::AnyIPv4, port)) {
        emit errorOccurred(QStringLiteral("无法启动 TCP 服务：%1").arg(m_server->errorString()));
        m_server->deleteLater();
        m_server = nullptr;
        return false;
    }

    return true;
}

void TcpServer::stop()
{
    if (!m_server) {
        return;
    }

    m_server->close();
    m_server->deleteLater();
    m_server = nullptr;
}
