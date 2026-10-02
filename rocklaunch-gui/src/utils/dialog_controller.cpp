#include "dialog_controller.h"

DialogState::DialogState(DialogController *host, const QString &id)
    : QObject(host)
    , m_host(host)
    , m_id(id)
    , m_payload(QQmlPropertyMap::create(this))
{
}

QString DialogState::id() const
{
    return m_id;
}

QQmlPropertyMap *DialogState::payload()
{
    return m_payload;
}

void DialogState::open(const QVariantMap &payload)
{
    for (const QString &key : m_payload->keys()) {
        m_payload->clear(key);
    }
    for (auto it = payload.constBegin(); it != payload.constEnd(); ++it) {
        m_payload->insert(it.key(), it.value());
    }
    m_host->open(m_id);
}

void DialogState::close()
{
    if (m_host->currentDialog() == m_id) {
        m_host->close();
    }
}

DialogController::DialogController(QObject *parent)
    : QObject(parent)
    , m_editProfile(this, QStringLiteral("editProfile"))
{
}

QString DialogController::currentDialog() const
{
    return m_currentDialog;
}

DialogState *DialogController::editProfile()
{
    return &m_editProfile;
}

QString DialogController::errorMessage() const
{
    return m_errorMessage;
}

void DialogController::showError(const QString &message)
{
    m_errorMessage = message;
    emit errorMessageChanged();
}

void DialogController::dismissError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorMessageChanged();
}

void DialogController::open(const QString &id)
{
    if (m_currentDialog == id) {
        return;
    }
    m_currentDialog = id;
    emit currentDialogChanged();
}

void DialogController::close()
{
    if (m_currentDialog.isEmpty()) {
        return;
    }
    m_currentDialog.clear();
    emit currentDialogChanged();
}