#include "gameserver.h"
#include <QCoreApplication>
#include <QTimer>

void simulateClientMessages(GameServer& server) {
    QTimer::singleShot(1000, [&]() {
        qDebug() << "[TEST] Simulating client message...";

        QString testMessage = R"({
            "game_id": "game1",
            "object_id": "tank1",
            "operation_id": "move",
            "args": {
                "speed": 10
            }
        })";

        // Эмулируем сообщение от клиента
        server.onMessageReceived(testMessage);
    });
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    quint16 port = 8080;
    GameServer server(port);

    qDebug() << "[INFO] Server started for WebSocket communication.";

    // Симуляция клиента
    simulateClientMessages(server);

    return a.exec();
}
