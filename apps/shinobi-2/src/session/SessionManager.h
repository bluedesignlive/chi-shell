#pragma once

#include <QObject>
#include <QStringList>

namespace chiterm {

class TerminalSession;

class SessionManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(int tabCount READ tabCount NOTIFY tabCountChanged)
    Q_PROPERTY(int activeTabIndex READ activeTabIndex
               WRITE setActiveTabIndex NOTIFY activeTabChanged)
    Q_PROPERTY(QObject* activeSession READ activeSessionAsObject
               NOTIFY activeSessionChanged)
    Q_PROPERTY(QStringList tabTitles READ tabTitles NOTIFY tabTitlesChanged)

public:
    explicit SessionManager(QObject *parent = nullptr);

    // ── Tab management ──────────────────────────────────────
    Q_INVOKABLE void createTab(const QString &workingDir = {});
    Q_INVOKABLE void closeTab(int index);
    Q_INVOKABLE void switchTab(int index);
    Q_INVOKABLE void nextTab();
    Q_INVOKABLE void previousTab();

    // ── Properties ──────────────────────────────────────────
    int tabCount() const { return m_sessions.size(); }
    int activeTabIndex() const { return m_activeIndex; }
    void setActiveTabIndex(int idx);
    TerminalSession *activeSession() const;
    QObject *activeSessionAsObject() const;
    QStringList tabTitles() const;

    Q_INVOKABLE QObject *sessionAt(int index) const;

signals:
    void tabCountChanged();
    void activeTabChanged();
    void activeSessionChanged();
    void tabTitlesChanged();
    void tabCreated(int index);
    void tabClosed(int index);

private slots:
    void onSessionTitleChanged();
    void onSessionEnded(int exitCode);

private:
    QList<TerminalSession *> m_sessions;
    int m_activeIndex = -1;
};

} // namespace chiterm
