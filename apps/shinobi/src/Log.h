#pragma once

#include <QLoggingCategory>

// ── Logging categories ──────────────────────────────────────
// Usage:
//   qCInfo(logPty) << "Spawned shell" << pid;
//   qCDebug(logVt) << "Feed" << data.size() << "bytes";
//   qCWarning(logSession) << "Process exited" << code;

Q_DECLARE_LOGGING_CATEGORY(logApp)
Q_DECLARE_LOGGING_CATEGORY(logPty)
Q_DECLARE_LOGGING_CATEGORY(logVt)
Q_DECLARE_LOGGING_CATEGORY(logRender)
Q_DECLARE_LOGGING_CATEGORY(logSession)
Q_DECLARE_LOGGING_CATEGORY(logConfig)
