#include "gameserver.h"
#include <QJsonDocument>
#include <QJsonArray>

GameServer::GameServer(quint16 port, QObject* parent)
    : QObject(parent),
      m_server(new QWebSocketServer(QStringLiteral("Game Server"),
                                    QWebSocketServer::NonSecureMode, this)) {
    setupIoC();  // Настраиваем IoC контейнер

    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "[INFO] Server started on port" << port;
        connect(m_server, &QWebSocketServer::newConnection, this, &GameServer::onNewConnection);
    } else {
        qCritical() << "[ERROR] Failed to start server:" << m_server->errorString();
    }
}

GameServer::~GameServer() {
    m_server->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void GameServer::setupIoC() {
    qDebug() << "[DEBUG] Setting up IoC container...";

    // Регистрация игровых объектов
    m_ioc.registerType<QObject*>("Игровые объекты/tank1", []() -> QObject* {
        qDebug() << "[DEBUG] Registering object: tank1";
        return new QObject();
    });

    // Регистрация функций для установки скорости
    m_ioc.registerType<std::function<void(QObject*, int)>>("Установить начальное значение скорости", []() {
        qDebug() << "[DEBUG] Registering command: Установить начальное значение скорости";
        return [](QObject* obj, int speed) {
            qDebug() << "[DEBUG] Setting speed for object to:" << speed;
            obj->setProperty("speed", speed);
        };
    });

    // Регистрация команды движения по прямой
    m_ioc.registerType<std::function<void(QObject*)>>("Движение по прямой", []() {
        qDebug() << "[DEBUG] Registering command: Движение по прямой";
        return [](QObject* obj) {
            qDebug() << "[DEBUG] Moving object with speed:" << obj->property("speed").toInt();
        };
    });

    // Регистрация очереди команд
    m_ioc.registerType<std::function<void(std::function<void()>)>>("Очередь команд", []() {
        qDebug() << "[DEBUG] Registering command: Очередь команд";
        return [](std::function<void()> cmd) {
            qDebug() << "[DEBUG] Command added to queue.";
            cmd();  // Выполняем команду прямо из очереди
        };
    });

    qDebug() << "[DEBUG] IoC container setup complete.";
}

void GameServer::onMessageReceived(const QString& message) {
    qDebug() << "[DEBUG] Received message from client:" << message;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qDebug() << "[WARN] Invalid message format";
        return;
    }

    QJsonObject obj = doc.object();
    QString gameId = obj["game_id"].toString();
    QString objectId = obj["object_id"].toString();
    QString operationId = obj["operation_id"].toString();
    QJsonObject args = obj["args"].toObject();

    qDebug() << "[DEBUG] Parsed message: game_id =" << gameId
             << ", object_id =" << objectId
             << ", operation_id =" << operationId;

    Command cmd{gameId, objectId, operationId, args};
    cmd.execute(m_ioc, this);
}

void GameServer::sendGameState(const QString& gameId) {
    qDebug() << "[DEBUG] Sending game state for game_id:" << gameId;

    QJsonObject state;
    state["game_id"] = gameId;
    state["objects"] = QJsonArray{
        QJsonObject{{"object_id", "tank1"}, {"position", "A1"}, {"health", 100}},
        QJsonObject{{"object_id", "tank2"}, {"position", "B2"}, {"health", 80}}
    };

    QJsonDocument doc(state);
    // Отправляем состояние игры всем клиентам
    broadcastMessage(doc.toJson(QJsonDocument::Compact));
}

void GameServer::onNewConnection() {
    QWebSocket* client = m_server->nextPendingConnection();
    qDebug() << "[INFO] New client connected:" << client->peerAddress().toString();

    connect(client, &QWebSocket::textMessageReceived, this, &GameServer::onClientMessage);
    connect(client, &QWebSocket::disconnected, this, &GameServer::onClientDisconnected);

    m_clients << client;
}

void GameServer::onClientMessage(const QString& message) {
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        qDebug() << "[DEBUG] Message received from client:" << client->peerAddress().toString();
        onMessageReceived(message);
    }
}

void GameServer::onClientDisconnected() {
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        qDebug() << "[INFO] Client disconnected:" << client->peerAddress().toString();
        m_clients.removeAll(client);
        client->deleteLater();
    }
}

void GameServer::broadcastMessage(const QString& message) {
    for (QWebSocket* client : qAsConst(m_clients)) {
        if (client && client->isValid()) {
            client->sendTextMessage(message);
        }
    }
    qDebug() << "[INFO] Broadcast message to all clients:" << message;
}

QSet<QString> GameServer::getAllowedOperations()
{
    return m_allowedOperations;
}

void Command::execute(IoCContainer& container, GameServer* server) {
    // Проверка допустимости команды, чтобы избежать инъекций
    // Если команда недопустима - сервер отклонит её выполнение.
    if (!server->getAllowedOperations().contains(operationId)) {
        qDebug() << "[ERROR] Unauthorized operation requested:" << operationId;
        // Можно отправить клиенту информацию об ошибке
//        if (initiatingClient) {
//            initiatingClient->sendTextMessage(QString("{\"error\": \"Unauthorized operation: %1\"}").arg(operationId));
//        }
        return;
    }

    if (operationId == "get_state") {
        server->sendGameState(gameId);
        return;
    }

    // Получаем объект через IoC
    auto obj = container.resolve<QObject*>("Игровые объекты/" + objectId);
    if (!obj) {
        qDebug() << "[ERROR] Object not found:" << objectId;
        return;
    }

    // Устанавливаем скорость объекта
    auto setSpeedFunc = container.resolve<std::function<void(QObject*, int)>>("Установить начальное значение скорости");
    if (setSpeedFunc && args.contains("speed")) {
        setSpeedFunc(obj, args["speed"].toInt());
    }

    // Создаем движение объекта
    auto moveFunc = container.resolve<std::function<void(QObject*)>>("Движение по прямой");
    if (moveFunc) {
        std::function<void()> moveCommand = [obj, moveFunc]() {
            moveFunc(obj);
        };

        // Помещаем команду в очередь
        auto queueFunc = container.resolve<std::function<void(std::function<void()>)>>("Очередь команд");
        if (queueFunc) {
            queueFunc(moveCommand);
        } else {
            qDebug() << "[ERROR] Queue handler not found.";
        }
    } else {
        qDebug() << "[ERROR] Move command not found.";
    }

    // Уведомляем всех клиентов об изменении состояния объекта
    server->broadcastMessage(QString("{\"status\": \"success\", \"operation\": \"%1\"}").arg(operationId));
}
