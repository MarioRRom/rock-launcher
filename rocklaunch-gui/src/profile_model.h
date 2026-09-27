#pragma once

#include "rocklaunch/core/profile_manager.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

class GameProfileModel;

// One profile as QML needs it: the generated id plus the optional name tag.
// Anonymous: QML only ever receives these as modelData, it never names the type.
class ProfileEntry
{
    Q_GADGET
    QML_ANONYMOUS
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)

public:
    ProfileEntry() = default;
    ProfileEntry(QString profileId, QString profileName)
        : id(std::move(profileId))
        , name(std::move(profileName))
    {
    }

    friend bool operator==(const ProfileEntry &left, const ProfileEntry &right)
    {
        return left.id == right.id && left.name == right.name;
    }

    QString id;
    QString name;
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

    Q_INVOKABLE bool removeProfile(const QString &id);
    Q_INVOKABLE void refresh();

signals:
    void profilesChanged();
    void currentProfileChanged();
    void currentProfileNameChanged();

private:
    const ProfileEntry *entryFor(const QString &id) const;

    rocklaunch::ProfileManager *m_profiles = nullptr;
    GameProfileModel *m_gameProfileModel = nullptr;
    QList<ProfileEntry> m_entries;
    QString m_currentProfile;
    inline static ProfileModel *s_instance = nullptr;
};
