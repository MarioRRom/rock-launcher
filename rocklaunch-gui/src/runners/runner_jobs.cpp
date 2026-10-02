#include "runner_jobs.h"

#include "rocklaunch/core/runners/runners.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <nlohmann/json.hpp>

namespace
{

QJsonArray ToJsonArray(const nlohmann::json &rows)
{
    return QJsonDocument::fromJson(QByteArray::fromStdString(rows.dump())).array();
}

// Releases() hands back the whole cache object; the page wants its rows.
QJsonArray ReleaseRows(const nlohmann::json &cache)
{
    const nlohmann::json rows = cache.value("releases", nlohmann::json::array());
    return rows.is_array() ? ToJsonArray(rows) : QJsonArray();
}

} // namespace

RunnerJobs::RunnerJobs(QObject *parent)
    : JobWorker(parent)
{
}

RunnerJobs::~RunnerJobs() = default;

bool RunnerJobs::refreshFailed() const
{
    return m_refreshFailed;
}

QString RunnerJobs::targetSource() const
{
    return m_targetSource;
}

QString RunnerJobs::targetName() const
{
    return m_targetName;
}

bool RunnerJobs::cancellable() const
{
    if (!busy() || m_kind != JobKind::Install) {
        return false;
    }
    // HashFile reads the whole file with no callback and fs::rename is not
    // interruptible: a cancel here would only land after the stage finishes.
    const std::optional<rocklaunch::ProgressStage> stage = CurrentStage();
    return !stage.has_value()
        || (*stage != rocklaunch::ProgressStage::Verifying
            && *stage != rocklaunch::ProgressStage::Installing);
}

bool RunnerJobs::Begin(JobKind kind, const QString &source, const QString &name)
{
    if (busy()) {
        return false;
    }

    m_kind = kind;
    m_refreshFailed = false;
    m_targetSource = source;
    m_targetName = name;
    m_output = std::make_shared<Output>();
    emit targetChanged();
    return true;
}

void RunnerJobs::refresh(bool forceReleases)
{
    if (!Begin(JobKind::Refresh, QString(), QString())) {
        return;
    }

    const std::shared_ptr<Output> output = m_output;
    StartJob([output, forceReleases](const std::shared_ptr<JobState> &) {
        JobResult result;
        output->installed = ToJsonArray(rocklaunch::Runners::Installed(true));

        try {
            output->releases = ReleaseRows(rocklaunch::Runners::Releases(forceReleases));
        } catch (const std::exception &exception) {
            // Keep the scan: a failed refresh must not empty the page of the
            // runners that are already on this machine.
            result.outcome = Outcome::Failed;
            // Offline at startup is routine; only the forced refresh may interrupt.
            if (forceReleases) {
                result.error = QString::fromUtf8(exception.what());
            }
        }
        return result;
    });
}

void RunnerJobs::install(const QString &source, const QString &name)
{
    if (!Begin(JobKind::Install, source, name)) {
        return;
    }

    const std::shared_ptr<Output> output = m_output;
    StartJob([output, source, name](const std::shared_ptr<JobState> &state) {
        rocklaunch::Runners::Install(name.toStdString(), source.toStdString(), "",
                                     ProgressReporter(state));
        output->installed = ToJsonArray(rocklaunch::Runners::Installed(true));
        return JobResult{};
    });
}

void RunnerJobs::remove(const QString &source, const QString &name)
{
    if (!Begin(JobKind::Remove, source, name)) {
        return;
    }

    const std::shared_ptr<Output> output = m_output;
    StartJob([output, source, name](const std::shared_ptr<JobState> &) {
        rocklaunch::Runners::Remove(name.toStdString(), source.toStdString());
        output->installed = ToJsonArray(rocklaunch::Runners::Installed(true));
        return JobResult{};
    });
}

void RunnerJobs::OnWorkFinished(const JobResult &result)
{
    if (result.outcome == Outcome::Failed) {
        m_refreshFailed = m_kind == JobKind::Refresh;
        emit refreshFailedChanged();
    }

    if (m_kind == JobKind::Refresh) {
        emit installedReady(m_output->installed);
        if (result.outcome == Outcome::Ok) {
            emit releasesReady(m_output->releases);
        }
    } else if (result.outcome == Outcome::Ok) {
        // The worker rescanned, so the card updates without a second busy
        // period a click could fall into.
        emit installedReady(m_output->installed);
    }
}
