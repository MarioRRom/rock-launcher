#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QString>
#include <QtQml/qqmlregistration.h>

class DialogController;

class DialogState final : public QObject
{
    Q_OBJECT

    QML_NAMED_ELEMENT(DialogState)
    QML_UNCREATABLE("created by DialogController")
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QQmlPropertyMap *payload READ payload CONSTANT)

public:
    explicit DialogState(DialogController *host, const QString &id);

    QString id() const;
    QQmlPropertyMap *payload();

    Q_INVOKABLE void open(const QVariantMap &payload);
    Q_INVOKABLE void close();

private:
    DialogController *m_host;
    QString m_id;
    QQmlPropertyMap *m_payload;
};

class DialogController final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString currentDialog READ currentDialog NOTIFY currentDialogChanged)
    Q_PROPERTY(DialogState *editProfile READ editProfile CONSTANT)

public:
    explicit DialogController(QObject *parent);

    static DialogController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(jsEngine);
        Q_ASSERT(s_instance);
        Q_ASSERT(qmlEngine->thread() == s_instance->thread());
        QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
        return s_instance;
    }

    static void setInstance(DialogController *instance)
    {
        s_instance = instance;
    }

    QString currentDialog() const;
    DialogState *editProfile();

    Q_INVOKABLE void open(const QString &id);
    Q_INVOKABLE void close();

signals:
    void currentDialogChanged();

private:
    QString m_currentDialog;
    DialogState m_editProfile;
    inline static DialogController *s_instance = nullptr;
};