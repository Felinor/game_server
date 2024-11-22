#ifndef GAMESERVER_H
#define GAMESERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonObject>
#include <QList>
#include <QDebug>
#include "ioccontainer.h"

class GameServer;

struct Command {
    QString gameId;
    QString objectId;
    QString operationId;
    QJsonObject args;

    void execute(IoCContainer& container, GameServer* server);
};

class GameServer : public QObject {
    Q_OBJECT

public:
    explicit GameServer(quint16 port, QObject* parent = nullptr);
    ~GameServer();

    void setupIoC();
    void onMessageReceived(const QString& message);
    void sendGameState(const QString& gameId);
    // Отправка сообщения всем клиентам
    void broadcastMessage(const QString& message);
    QSet<QString> getAllowedOperations();

private slots:
    void onNewConnection();
    void onClientMessage(const QString& message);
    void onClientDisconnected();

private:
    QWebSocketServer* m_server;
    QList<QWebSocket*> m_clients;
    IoCContainer m_ioc;  // IoC контейнер
    // Список разрешенных операций
    const QSet<QString> m_allowedOperations = {"move", "get_state"};
};

#endif // GAMESERVER_H
