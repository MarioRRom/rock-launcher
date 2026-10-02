#pragma once

#include "job_worker.h"

#include <QJsonArray>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

#include <memory>

// The runner operations as one QML-visible job surface: refresh, install, remove.
// They share a slot with each other and with no other domain.
class RunnerJobs final : public JobWorker
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool refreshFailed READ refreshFailed NOTIFY refreshFailedChanged)
    Q_PROPERTY(QString targetSource READ targetSource NOTIFY targetChanged)
    Q_PROPERTY(QString targetName READ targetName NOTIFY targetChanged)

public:
    // No default for parent: a default-constructible singleton makes Qt skip
    // create() and hand QML a second instance nobody is wired to.
    explicit RunnerJobs(QObject *parent);
    ~RunnerJobs() override;

    static RunnerJobs *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(jsEngine);
        Q_ASSERT(s_instance);
        Q_ASSERT(qmlEngine->thread() == s_instance->thread());
        QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
        return s_instance;
    }

    static void setInstance(RunnerJobs *instance)
    {
        s_instance = instance;
    }

    // True only while the last failed job was a refresh: an install or remove
    // failure must not blame the release list (the empty state reads this).
    bool refreshFailed() const;
    // Empty on a refresh: no runner owns that progress.
    QString targetSource() const;
    QString targetName() const;

    // Ignored while a job runs: one job at a time.
    Q_INVOKABLE void refresh(bool forceReleases = false);
    Q_INVOKABLE void install(const QString &source, const QString &name);
    Q_INVOKABLE void remove(const QString &source, const QString &name);

    bool cancellable() const override;

signals:
    void refreshFailedChanged();
    void targetChanged();
    void installedReady(const QJsonArray &rows);
    void releasesReady(const QJsonArray &rows);

private:
    enum class JobKind
    {
        Refresh,
        Install,
        Remove,
    };

    // Held by the worker lambda and by this object, so the rows the worker
    // writes outlive a close that destroys the adapter mid-job.
    struct Output
    {
        QJsonArray installed;
        QJsonArray releases;
    };

    // Rejects a call made while a job runs, before writing anything: a dropped
    // call must not retarget the job in flight or clear its outcome.
    bool Begin(JobKind kind, const QString &source, const QString &name);

    void OnWorkFinished(const JobResult &result) override;

    std::shared_ptr<Output> m_output;
    JobKind m_kind = JobKind::Refresh;
    bool m_refreshFailed = false;
    QString m_targetSource;
    QString m_targetName;
    inline static RunnerJobs *s_instance = nullptr;
};
