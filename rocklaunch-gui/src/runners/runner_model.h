#pragma once

#include "rocklaunch/core/runners/runner_manager.h"

#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

class RunnerModel final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QStringList runners READ runners NOTIFY runnersChanged)
    Q_PROPERTY(QString currentRunner READ currentRunner WRITE setCurrentRunner NOTIFY currentRunnerChanged)

public:
    explicit RunnerModel(rocklaunch::RunnerManager *manager, QObject *parent = nullptr);

    static RunnerModel *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(jsEngine);
        Q_ASSERT(s_instance);
        Q_ASSERT(qmlEngine->thread() == s_instance->thread());
        QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
        return s_instance;
    }

    static void setInstance(RunnerModel *instance)
    {
        s_instance = instance;
    }

    QStringList runners() const;
    QString currentRunner() const;
    void setCurrentRunner(const QString &id);

    Q_INVOKABLE void refresh();

signals:
    void runnersChanged();
    void currentRunnerChanged();

private:
    // Shared with LaunchController: one discovery pass.
    rocklaunch::RunnerManager *m_runnerManager = nullptr;
    QStringList m_runners;
    QString m_currentRunner;
    inline static RunnerModel *s_instance = nullptr;
};
