#pragma once

#include <QJsonArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

class RunnerJobs;

// One row of the runners page: both core listings joined on (source, name).
// A field a row cannot have is empty, never a placeholder value.
class RunnerEntry
{
    Q_GADGET
    QML_ANONYMOUS
    Q_PROPERTY(QString source MEMBER source)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString kind MEMBER kind)
    Q_PROPERTY(bool installed MEMBER installed)
    Q_PROPERTY(qulonglong downloadSize MEMBER downloadSize)
    Q_PROPERTY(qulonglong diskSize MEMBER diskSize)

public:
    friend bool operator==(const RunnerEntry &left, const RunnerEntry &right)
    {
        return left.source == right.source && left.name == right.name
            && left.kind == right.kind && left.installed == right.installed
            && left.downloadSize == right.downloadSize
            && left.diskSize == right.diskSize;
    }

    QString source;
    QString name;
    QString kind;
    bool installed = false;
    qulonglong downloadSize = 0;
    qulonglong diskSize = 0;
};

// The view over both listings; RunnerJobs owns the operations that produce
// them and publishes progress and failures.
class RunnerModel final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QList<RunnerEntry> runners READ runners NOTIFY runnersChanged)
    Q_PROPERTY(QList<RunnerEntry> installedRunners READ installedRunners NOTIFY installedRunnersChanged)
    Q_PROPERTY(QStringList sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(QString currentSource READ currentSource WRITE setCurrentSource NOTIFY currentSourceChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)

public:
    explicit RunnerModel(RunnerJobs *jobs, QObject *parent = nullptr);

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

    QList<RunnerEntry> runners() const;
    QList<RunnerEntry> installedRunners() const;
    QStringList sources() const;
    QString currentSource() const;
    void setCurrentSource(const QString &source);
    QString search() const;
    void setSearch(const QString &search);

signals:
    void runnersChanged();
    void installedRunnersChanged();
    void sourcesChanged();
    void currentSourceChanged();
    void searchChanged();

private:
    void OnInstalled(const QJsonArray &rows);
    void OnReleases(const QJsonArray &rows);
    void Rebuild();

    QJsonArray m_installedRows;
    QJsonArray m_releaseRows;
    QList<RunnerEntry> m_entries;
    QList<RunnerEntry> m_installedEntries;
    QStringList m_sources;
    QString m_currentSource;
    QString m_search;
    inline static RunnerModel *s_instance = nullptr;
};
