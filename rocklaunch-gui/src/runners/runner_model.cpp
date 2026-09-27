#include "runner_model.h"

RunnerModel::RunnerModel(rocklaunch::RunnerManager *manager, QObject *parent)
    : QObject(parent)
    , m_runnerManager(manager)
{
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
    if (!m_runnerManager) {
        return;
    }

    QStringList updated;
    for (const rocklaunch::Runner &runner : m_runnerManager->List()) {
        updated.append(QString::fromStdString(runner.id));
    }

    if (m_runners != updated) {
        m_runners = updated;
        emit runnersChanged();
    }
}
