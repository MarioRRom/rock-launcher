#include "job_worker.h"

#include <QtConcurrent/QtConcurrentRun>

namespace
{

constexpr int kProgressTickMs = 100;

} // namespace

JobWorker::JobWorker(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(kProgressTickMs);
    connect(&m_timer, &QTimer::timeout, this, &JobWorker::PollProgress);
    connect(&m_watcher, &QFutureWatcher<JobResult>::finished,
            this, &JobWorker::OnJobFinished);
}

JobWorker::~JobWorker()
{
    cancel();
    m_watcher.waitForFinished();
}

bool JobWorker::busy() const
{
    return m_busy;
}

QString JobWorker::stage() const
{
    return m_stage.has_value() ? QString::fromLatin1(rocklaunch::StageName(*m_stage))
                               : QString();
}

double JobWorker::percent() const
{
    return m_percent;
}

bool JobWorker::determinate() const
{
    return m_determinate;
}

QString JobWorker::error() const
{
    return m_error;
}

void JobWorker::cancel()
{
    if (m_state) {
        m_state->cancelled.store(true);
    }
}

std::optional<rocklaunch::ProgressStage> JobWorker::CurrentStage() const
{
    return m_stage;
}

rocklaunch::ProgressCallback JobWorker::ProgressReporter(
    const std::shared_ptr<JobState> &state)
{
    return [state](const rocklaunch::Progress &progress) {
        {
            const std::lock_guard<std::mutex> lock(state->progressMutex);
            state->progress = progress;
            state->hasProgress = true;
        }
        return !state->cancelled.load();
    };
}

void JobWorker::StartJob(JobWork work)
{
    if (m_busy) {
        return;
    }

    m_state = std::make_shared<JobState>();
    m_busy = true;
    m_error.clear();
    m_stage.reset();
    m_percent = 0.0;
    m_determinate = false;
    emit busyChanged();
    emit errorChanged();
    emit progressChanged();

    const std::shared_ptr<JobState> state = m_state;
    // The typed result is what keeps rocklaunch::Cancelled observable: an
    // exception crossing the QFuture boundary loses its concrete type.
    m_watcher.setFuture(QtConcurrent::run([state, work = std::move(work)] {
        try {
            return work(state);
        } catch (const rocklaunch::Cancelled &) {
            return JobResult{ Outcome::Cancelled, QString() };
        } catch (const std::exception &exception) {
            return JobResult{ Outcome::Failed, QString::fromUtf8(exception.what()) };
        }
    }));
    m_timer.start();
}

void JobWorker::OnJobFinished()
{
    const JobResult result = m_watcher.result();
    m_timer.stop();

    m_busy = false;
    m_stage.reset();
    m_percent = 0.0;
    m_determinate = false;
    emit busyChanged();
    emit progressChanged();

    OnWorkFinished(result);

    if (result.outcome == Outcome::Failed) {
        m_error = result.error;
        emit errorChanged();
    }
}

void JobWorker::OnWorkFinished(const JobResult &result)
{
    Q_UNUSED(result);
}

void JobWorker::PollProgress()
{
    if (!m_state || !m_state->hasProgress) {
        return;
    }

    rocklaunch::Progress progress;
    {
        const std::lock_guard<std::mutex> lock(m_state->progressMutex);
        progress = m_state->progress;
    }

    const bool determinate = progress.totalBytes > 0;
    if (m_stage == progress.stage && m_percent == progress.percentage
        && m_determinate == determinate) {
        return;
    }
    m_stage = progress.stage;
    m_percent = progress.percentage;
    m_determinate = determinate;
    emit progressChanged();
}
