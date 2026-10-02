#include "runner_model.h"

#include "runner_jobs.h"

#include "rocklaunch/core/runners/runners.h"

#include <QHash>
#include <QJsonObject>
#include <QSet>

namespace
{

QString Key(const QString &source, const QString &name)
{
    return source + '/' + name;
}

// The page shows managed runners only: Steam and system runners belong to the
// profile editor, which picks them and never deletes them.
bool IsManaged(const QString &source)
{
    return source != QLatin1String(rocklaunch::kSteamRunnerSource)
        && source != QLatin1String(rocklaunch::kSystemRunnerSource);
}

RunnerEntry InstalledEntry(const QJsonObject &row)
{
    RunnerEntry entry;
    entry.source = row.value(QLatin1String("source")).toString();
    entry.name = row.value(QLatin1String("name")).toString();
    entry.kind = row.value(QLatin1String("kind")).toString();
    entry.installed = true;
    entry.diskSize = static_cast<qulonglong>(row.value(QLatin1String("size")).toDouble());
    return entry;
}

QList<RunnerEntry> Join(const QJsonArray &installed, const QJsonArray &releases)
{
    QHash<QString, QJsonObject> installedByKey;
    for (const QJsonValue &value : installed) {
        const QJsonObject row = value.toObject();
        const QString source = row.value(QLatin1String("source")).toString();
        if (IsManaged(source)) {
            installedByKey.insert(Key(source, row.value(QLatin1String("name")).toString()), row);
        }
    }

    QList<RunnerEntry> entries;
    QSet<QString> placed;

    for (const QJsonValue &value : releases) {
        const QJsonObject row = value.toObject();
        const QString source = row.value(QLatin1String("source")).toString();
        const QString name = row.value(QLatin1String("name")).toString();
        const QString key = Key(source, name);
        if (placed.contains(key)) {
            continue;
        }
        placed.insert(key);

        RunnerEntry entry;
        entry.source = source;
        entry.name = name;
        entry.downloadSize = static_cast<qulonglong>(
            row.value(QLatin1String("size")).toDouble());
        const auto found = installedByKey.constFind(key);
        if (found != installedByKey.constEnd()) {
            entry.installed = true;
            entry.kind = found->value(QLatin1String("kind")).toString();
            entry.diskSize = static_cast<qulonglong>(
                found->value(QLatin1String("size")).toDouble());
        }
        entries.append(entry);
    }

    // Releases keep the API's order and the installed leftovers follow; never
    // sort by name, which puts GE-Proton9-20 ahead of GE-Proton10-32.
    for (const QJsonValue &value : installed) {
        const RunnerEntry entry = InstalledEntry(value.toObject());
        if (!IsManaged(entry.source)
            || placed.contains(Key(entry.source, entry.name))) {
            continue;
        }
        placed.insert(Key(entry.source, entry.name));
        entries.append(entry);
    }

    return entries;
}

QStringList UniqueSources(const QList<RunnerEntry> &entries)
{
    QStringList sources;
    for (const RunnerEntry &entry : entries) {
        if (!sources.contains(entry.source)) {
            sources.append(entry.source);
        }
    }
    return sources;
}

} // namespace

RunnerModel::RunnerModel(RunnerJobs *jobs, QObject *parent)
    : QObject(parent)
{
    if (jobs) {
        connect(jobs, &RunnerJobs::installedReady, this, &RunnerModel::OnInstalled);
        connect(jobs, &RunnerJobs::releasesReady, this, &RunnerModel::OnReleases);
    }
}

QList<RunnerEntry> RunnerModel::runners() const
{
    QList<RunnerEntry> visible;
    for (const RunnerEntry &entry : m_entries) {
        if (!m_currentSource.isEmpty() && entry.source != m_currentSource) {
            continue;
        }
        if (!m_search.isEmpty() && !entry.name.contains(m_search, Qt::CaseInsensitive)) {
            continue;
        }
        visible.append(entry);
    }
    return visible;
}

QList<RunnerEntry> RunnerModel::installedRunners() const
{
    return m_installedEntries;
}

QStringList RunnerModel::sources() const
{
    return m_sources;
}

QString RunnerModel::currentSource() const
{
    return m_currentSource;
}

void RunnerModel::setCurrentSource(const QString &source)
{
    if (m_currentSource == source) {
        return;
    }
    m_currentSource = source;
    emit currentSourceChanged();
    emit runnersChanged();
}

QString RunnerModel::search() const
{
    return m_search;
}

void RunnerModel::setSearch(const QString &search)
{
    if (m_search == search) {
        return;
    }
    m_search = search;
    emit searchChanged();
    emit runnersChanged();
}

void RunnerModel::OnInstalled(const QJsonArray &rows)
{
    m_installedRows = rows;
    Rebuild();
}

void RunnerModel::OnReleases(const QJsonArray &rows)
{
    m_releaseRows = rows;
    Rebuild();
}

void RunnerModel::Rebuild()
{
    QList<RunnerEntry> installed;
    for (const QJsonValue &value : m_installedRows) {
        installed.append(InstalledEntry(value.toObject()));
    }
    if (m_installedEntries != installed) {
        m_installedEntries = installed;
        emit installedRunnersChanged();
    }

    const QList<RunnerEntry> entries = Join(m_installedRows, m_releaseRows);
    if (m_entries != entries) {
        m_entries = entries;
        emit runnersChanged();
    }

    const QStringList sources = UniqueSources(m_entries);
    if (m_sources != sources) {
        m_sources = sources;
        emit sourcesChanged();
    }
}
