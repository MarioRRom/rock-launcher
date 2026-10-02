#pragma once

#include "rocklaunch/core/progress.h"

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

// Domain-agnostic half of a background job: one blocking core call at a time on a
// worker thread. A subclass supplies the work and whether it can be interrupted.
class JobWorker : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString stage READ stage NOTIFY progressChanged)
    Q_PROPERTY(double percent READ percent NOTIFY progressChanged)
    Q_PROPERTY(bool determinate READ determinate NOTIFY progressChanged)
    Q_PROPERTY(bool cancellable READ cancellable NOTIFY progressChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit JobWorker(QObject *parent);
    ~JobWorker() override;

    bool busy() const;
    QString stage() const;
    double percent() const;
    // False while a stage measures no bytes (verify, extract): the percentage
    // would sit at 0 for the whole step.
    bool determinate() const;
    QString error() const;
    // Pure because interruptibility belongs to the core call, not to the
    // mechanism: one domain cancels on the progress callback, another never does.
    virtual bool cancellable() const = 0;

    Q_INVOKABLE void cancel();

signals:
    void busyChanged();
    void progressChanged();
    void errorChanged();

protected:
    enum class Outcome
    {
        Ok,
        Cancelled,
        Failed,
    };

    struct JobResult
    {
        Outcome outcome = Outcome::Ok;
        QString error;
    };

    // Captured by the worker by value and read from the GUI timer, so a close
    // mid-job cannot leave the worker writing through a dangling worker object.
    struct JobState
    {
        std::atomic<bool> cancelled{ false };
        std::atomic<bool> hasProgress{ false };
        std::mutex progressMutex;
        rocklaunch::Progress progress;
    };

    using JobWork = std::function<JobResult(const std::shared_ptr<JobState> &)>;

    // Ignored while a job runs. Each subclass owns its own, so domains are
    // serialized against themselves and never against each other.
    void StartJob(JobWork work);
    // Called once busy has cleared. Never reached from the destructor:
    // waitForFinished does not pump the event loop, so a close mid-job drops it.
    virtual void OnWorkFinished(const JobResult &result);

    // The enum behind stage(), for policy that compares stages. stage() is the
    // display form and comparing it would make the labels load-bearing.
    std::optional<rocklaunch::ProgressStage> CurrentStage() const;
    // Hands core a callback that stores each tick and returns false on a cancel.
    // Static so a work lambda can build it without capturing this.
    static rocklaunch::ProgressCallback ProgressReporter(
        const std::shared_ptr<JobState> &state);

private:
    void OnJobFinished();
    void PollProgress();

    QFutureWatcher<JobResult> m_watcher;
    QTimer m_timer;
    std::shared_ptr<JobState> m_state;
    bool m_busy = false;
    QString m_error;
    std::optional<rocklaunch::ProgressStage> m_stage;
    double m_percent = 0.0;
    bool m_determinate = false;
};
