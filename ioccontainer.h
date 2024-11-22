#ifndef IOCCONTAINER_H
#define IOCCONTAINER_H

#include <functional>
#include <unordered_map>
#include <QString>
#include <QDebug>

class IoCContainer {
public:
    template<typename T>
    void registerType(const QString& key, std::function<T()> creator) {
        factories[key] = [creator]() -> void* {
            return static_cast<void*>(new T(creator()));
        };
    }

    template<typename T>
    T resolve(const QString& key) {
        auto it = factories.find(key);
        if (it != factories.end()) {
            return *static_cast<T*>(it->second());
        }
        qDebug() << "[ERROR] IoC: No factory registered for key:" << key;
        return nullptr;
    }

private:
    std::unordered_map<QString, std::function<void*()>> factories;
};

#endif // IOCCONTAINER_H
