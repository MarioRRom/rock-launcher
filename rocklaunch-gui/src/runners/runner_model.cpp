#include "runner_model.h"

#include "../profile_model.h"

#include <rocklaunch/core/config_store.h>

RunnerModel::RunnerModel(QObject *parent)
    : QObject(parent)
    , m_runnerManager(rocklaunch::RunnerManager::CreateDefault())
{
}

void RunnerModel::SetProfileModel(ProfileModel *model)
{
    m_profileModel = model;
}

QStringList RunnerModel::runners() const
{
    return m_runners;
}

QString RunnerModel::currentRunner() const
{
    return m_currentRunner;
}

void RunnerModel::setCurrentRunner(const QString &id)
{
    if (m_currentRunner == id) {
        return;
    }

    m_currentRunner = id;
    emit currentRunnerChanged();
}

void RunnerModel::refresh()
{
    std::vector<rocklaunch::Runner> runners = m_runnerManager.List();

    m_runners.clear();
    for (const rocklaunch::Runner &runner : runners) {
        m_runners.append(QString::fromStdString(runner.id));
    }

    emit runnersChanged();
}
