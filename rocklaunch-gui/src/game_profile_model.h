#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

class GameProfileModel final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString gameId READ gameId WRITE setGameId NOTIFY gameIdChanged)

public:
    explicit GameProfileModel(QObject *parent);

    static GameProfileModel *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(jsEngine);
        Q_ASSERT(s_instance);
        Q_ASSERT(qmlEngine->thread() == s_instance->thread());
        QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
        return s_instance;
    }

    static void setInstance(GameProfileModel *instance)
    {
        s_instance = instance;
    }

    QString gameId() const;
    void setGameId(const QString &id);

signals:
    void gameIdChanged();

private:
    QString m_gameId;
    inline static GameProfileModel *s_instance = nullptr;
};
