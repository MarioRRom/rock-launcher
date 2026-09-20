#pragma once

#include "rocklaunch/core/runners/runner_manager.h"

#include <QObject>
#include <QStringList>

class ProfileModel;

class RunnerModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList runners READ runners NOTIFY runnersChanged)
    Q_PROPERTY(QString currentRunner READ currentRunner WRITE setCurrentRunner NOTIFY currentRunnerChanged)

public:
    explicit RunnerModel(QObject *parent = nullptr);

    void SetProfileModel(ProfileModel *model);

    QStringList runners() const;
    QString currentRunner() const;
    void setCurrentRunner(const QString &id);

    Q_INVOKABLE void refresh();

signals:
    void runnersChanged();
    void currentRunnerChanged();

private:
    ProfileModel *m_profileModel = nullptr;
    rocklaunch::RunnerManager m_runnerManager;
    QStringList m_runners;
    QString m_currentRunner;
};
