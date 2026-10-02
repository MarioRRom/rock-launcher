#pragma once

#include "rocklaunch/core/profile_manager.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

#include <utility>

class GameProfileModel;

// One profile as QML needs it: id, optional name tag, the (runnerName,
// runnerSource) pair and the install dir. Anonymous: QML only sees modelData.
class ProfileEntry
{
    Q_GADGET
    QML_ANONYMOUS
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString runnerName MEMBER runnerName)
    Q_PROPERTY(QString runnerSource MEMBER runnerSource)
    Q_PROPERTY(QString installDir MEMBER installDir)

public:
    ProfileEntry() = default;
    ProfileEntry(QString profileId, QString profileName)
        : id(std::move(profileId))
        , name(std::move(profileName))
    {
    }

    friend bool operator==(const ProfileEntry &left, const ProfileEntry &right)
    {
        return left.id == right.id && left.name == right.name
            && left.runnerName == right.runnerName
            && left.runnerSource == right.runnerSource
            && left.installDir == right.installDir;
    }

    QString id;
    QString name;
    QString runnerName;
    QString runnerSource;
    QString installDir;
};

class ProfileModel final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QList<ProfileEntry> profiles READ profiles NOTIFY profilesChanged)
    Q_PROPERTY(QString currentProfile READ currentProfile WRITE setCurrentProfile NOTIFY currentProfileChanged)
    Q_PROPERTY(QString currentProfileName READ currentProfileName NOTIFY currentProfileNameChanged)
    Q_PROPERTY(QString freeProfileId READ freeProfileId NOTIFY profilesChanged)

public:
    ProfileModel(rocklaunch::ProfileManager *manager,
                 GameProfileModel *gameModel,
                 QObject *parent = nullptr);

    static ProfileModel *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(jsEngine);
        Q_ASSERT(s_instance);
        Q_ASSERT(qmlEngine->thread() == s_instance->thread());
        QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
        return s_instance;
    }

    static void setInstance(ProfileModel *instance)
    {
        s_instance = instance;
    }

    QList<ProfileEntry> profiles() const;
    QString currentProfile() const;
    void setCurrentProfile(const QString &id);
    QString currentProfileName() const;
    QString freeProfileId() const;

    Q_INVOKABLE QString createProfile(const QString &name = QString());
    Q_INVOKABLE bool renameProfile(const QString &id, const QString &name);
    Q_INVOKABLE bool nameValid(const QString &name) const;
    Q_INVOKABLE QString profileName(const QString &id) const;
    Q_INVOKABLE ProfileEntry profileEntry(const QString &id) const;
    Q_INVOKABLE bool setRunner(const QString &id, const QString &name,
                               const QString &source);

    // Empty: the name did not save; failed() carries the reason. An id: the
    // name saved even when the runner failed and announced that too.
    Q_INVOKABLE QString saveProfile(const QString &id, const QString &name,
                                    const QString &runnerName,
                                    const QString &runnerSource);

    Q_INVOKABLE bool removeProfile(const QString &id);
    Q_INVOKABLE void refresh();

signals:
    void profilesChanged();
    void currentProfileChanged();
    void currentProfileNameChanged();
    // Core logged the detail; QML only renders this text in the error dialog.
    void failed(const QString &message);

private:
    const ProfileEntry *entryFor(const QString &id) const;

    rocklaunch::ProfileManager *m_profiles = nullptr;
    GameProfileModel *m_gameProfileModel = nullptr;
    QList<ProfileEntry> m_entries;
    QString m_currentProfile;
    inline static ProfileModel *s_instance = nullptr;
};
