#include "SessionManager.h"
#include "TerminalSession.h"

#include <QDir>

namespace chiterm {

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
{
}

void SessionManager::createTab(const QString &workingDir)
{
    auto *session = new TerminalSession(this);

    connect(session, &TerminalSession::titleChanged,
            this, &SessionManager::onSessionTitleChanged);
    connect(session, &TerminalSession::sessionEnded,
            this, &SessionManager::onSessionEnded);

    m_sessions.append(session);

    int idx = m_sessions.size() - 1;
    emit tabCountChanged();
    emit tabTitlesChanged();
    emit tabCreated(idx);

    // Start the shell
    session->start({}, workingDir);

    // Switch to the new tab
    setActiveTabIndex(idx);
}

void SessionManager::closeTab(int index)
{
    if (index < 0 || index >= m_sessions.size())
        return;

    TerminalSession *session = m_sessions.takeAt(index);
    session->close();
    session->deleteLater();

    emit tabClosed(index);
    emit tabCountChanged();
    emit tabTitlesChanged();

    // Adjust active index
    if (m_sessions.isEmpty()) {
        m_activeIndex = -1;
        emit activeTabChanged();
        emit activeSessionChanged();
    } else {
        int newIndex = qMin(m_activeIndex, m_sessions.size() - 1);
        if (m_activeIndex >= index && m_activeIndex > 0)
            newIndex = m_activeIndex - 1;
        newIndex = qBound(0, newIndex, m_sessions.size() - 1);
        setActiveTabIndex(newIndex);
    }
}

void SessionManager::switchTab(int index)
{
    setActiveTabIndex(index);
}

void SessionManager::nextTab()
{
    if (m_sessions.size() <= 1)
        return;
    setActiveTabIndex((m_activeIndex + 1) % m_sessions.size());
}

void SessionManager::previousTab()
{
    if (m_sessions.size() <= 1)
        return;
    int idx = m_activeIndex - 1;
    if (idx < 0) idx = m_sessions.size() - 1;
    setActiveTabIndex(idx);
}

void SessionManager::setActiveTabIndex(int idx)
{
    idx = qBound(0, idx, m_sessions.size() - 1);
    if (idx == m_activeIndex)
        return;

    m_activeIndex = idx;
    emit activeTabChanged();
    emit activeSessionChanged();
}

TerminalSession *SessionManager::activeSession() const
{
    if (m_activeIndex >= 0 && m_activeIndex < m_sessions.size())
        return m_sessions[m_activeIndex];
    return nullptr;
}

QObject *SessionManager::activeSessionAsObject() const
{
    return activeSession();
}

QStringList SessionManager::tabTitles() const
{
    QStringList titles;
    for (const auto *s : m_sessions)
        titles.append(s->title());
    return titles;
}

QObject *SessionManager::sessionAt(int index) const
{
    if (index >= 0 && index < m_sessions.size())
        return m_sessions[index];
    return nullptr;
}

void SessionManager::onSessionTitleChanged()
{
    emit tabTitlesChanged();
}

void SessionManager::onSessionEnded(int /*exitCode*/)
{
    auto *session = qobject_cast<TerminalSession *>(sender());
    if (!session)
        return;

    int idx = m_sessions.indexOf(session);
    if (idx >= 0)
        closeTab(idx);
}

} // namespace chiterm
