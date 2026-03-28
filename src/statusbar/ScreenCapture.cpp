#include "ScreenCapture.h"

#include <QDir>
#include <QStandardPaths>
#include <QDebug>

ScreenCapture::ScreenCapture(QObject *parent)
    : QObject(parent)
{
    m_recordTimer = new QTimer(this);
    m_recordTimer->setInterval(1000);
    connect(m_recordTimer, &QTimer::timeout, this, [this]() {
        m_recordSeconds = m_recordStart.secsTo(QDateTime::currentDateTime());
        emit recordTimeChanged();
    });

    detectTools();
}

void ScreenCapture::detectTools()
{
    auto check = [](const QString &name) -> bool {
        QProcess p;
        p.start("which", {name});
        p.waitForFinished(1000);
        return p.exitCode() == 0;
    };

    m_hasGrim  = check("grim");
    m_hasSlurp = check("slurp");

    // Try wf-recorder first, then wl-screenrec
    if (check("wf-recorder")) {
        m_hasRecorder = true;
        m_recorderBin = "wf-recorder";
    } else if (check("wl-screenrec")) {
        m_hasRecorder = true;
        m_recorderBin = "wl-screenrec";
    }

    qDebug() << "ScreenCapture: grim=" << m_hasGrim
             << "slurp=" << m_hasSlurp
             << "recorder=" << m_recorderBin;
}

QString ScreenCapture::screenshotDir()
{
    QString pictures = QStandardPaths::writableLocation(
        QStandardPaths::PicturesLocation);
    QString dir = pictures + "/Screenshots";
    QDir().mkpath(dir);
    return dir;
}

QString ScreenCapture::recordingDir()
{
    QString videos = QStandardPaths::writableLocation(
        QStandardPaths::MoviesLocation);
    QString dir = videos + "/Screencasts";
    QDir().mkpath(dir);
    return dir;
}

QString ScreenCapture::timestamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd-HHmmss");
}

void ScreenCapture::copyToClipboard(const QString &path)
{
    QProcess::startDetached("wl-copy", {"-t", "image/png"},
                             QString(), nullptr);

    // Use shell pipe: wl-copy < file
    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            proc, &QProcess::deleteLater);
    proc->setStandardInputFile(path);
    proc->start("wl-copy", {"-t", "image/png"});
}

void ScreenCapture::finishScreenshot(const QString &path)
{
    m_lastScreenshot = path;
    copyToClipboard(path);
    emit screenshotTaken(path);
    qDebug() << "Screenshot saved:" << path;
}

// ═════════════════════════════════════════════════════
// Screenshots
// ═════════════════════════════════════════════════════

void ScreenCapture::screenshotFull()
{
    if (!m_hasGrim) return;

    QString path = screenshotDir() + "/chi-screenshot-"
                   + timestamp() + ".png";

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, path](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0)
            finishScreenshot(path);
        proc->deleteLater();
    });
    proc->start("grim", {path});
}

void ScreenCapture::screenshotRegion()
{
    if (!m_hasGrim || !m_hasSlurp) return;

    // Step 1: slurp for geometry
    auto *slurp = new QProcess(this);
    connect(slurp, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, slurp](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            slurp->deleteLater();
            return;
        }

        QString geometry = slurp->readAllStandardOutput().trimmed();
        slurp->deleteLater();

        if (geometry.isEmpty()) return;

        // Step 2: grim with geometry
        QString path = screenshotDir() + "/chi-screenshot-"
                       + timestamp() + ".png";

        auto *grim = new QProcess(this);
        connect(grim, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, grim, path](int exitCode, QProcess::ExitStatus) {
            if (exitCode == 0)
                finishScreenshot(path);
            grim->deleteLater();
        });
        grim->start("grim", {"-g", geometry, path});
    });
    slurp->start("slurp", {});
}

void ScreenCapture::screenshotDelayed(int seconds)
{
    if (!m_hasGrim || seconds <= 0) return;

    QTimer::singleShot(seconds * 1000, this, [this]() {
        screenshotFull();
    });
}

// ═════════════════════════════════════════════════════
// Screen Recording
// ═════════════════════════════════════════════════════

void ScreenCapture::startRecording(bool withAudio)
{
    if (!m_hasRecorder || m_recording) return;

    m_recordPath = recordingDir() + "/chi-recording-"
                   + timestamp() + ".mp4";

    m_recorderProc = new QProcess(this);

    QStringList args;
    if (m_recorderBin == "wf-recorder") {
        args << "-f" << m_recordPath;
        if (withAudio)
            args << "-a";
    } else {
        // wl-screenrec
        args << "--filename" << m_recordPath;
        if (withAudio)
            args << "--audio";
    }

    connect(m_recorderProc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus) {
        m_recording = false;
        m_recordTimer->stop();
        m_recordSeconds = 0;
        emit recordingChanged();
        emit recordTimeChanged();
        emit recordingStopped(m_recordPath);
        qDebug() << "Recording saved:" << m_recordPath;
        m_recorderProc->deleteLater();
        m_recorderProc = nullptr;
    });

    m_recorderProc->start(m_recorderBin, args);

    m_recording = true;
    m_recordStart = QDateTime::currentDateTime();
    m_recordSeconds = 0;
    m_recordTimer->start();
    emit recordingChanged();
    emit recordTimeChanged();
}

void ScreenCapture::startRecordingRegion(bool withAudio)
{
    if (!m_hasRecorder || !m_hasSlurp || m_recording) return;

    auto *slurp = new QProcess(this);
    connect(slurp, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, slurp, withAudio](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            slurp->deleteLater();
            return;
        }

        QString geometry = slurp->readAllStandardOutput().trimmed();
        slurp->deleteLater();
        if (geometry.isEmpty()) return;

        m_recordPath = recordingDir() + "/chi-recording-"
                       + timestamp() + ".mp4";

        m_recorderProc = new QProcess(this);

        QStringList args;
        if (m_recorderBin == "wf-recorder") {
            args << "-g" << geometry << "-f" << m_recordPath;
            if (withAudio) args << "-a";
        } else {
            args << "--geometry" << geometry
                 << "--filename" << m_recordPath;
            if (withAudio) args << "--audio";
        }

        connect(m_recorderProc,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int, QProcess::ExitStatus) {
            m_recording = false;
            m_recordTimer->stop();
            m_recordSeconds = 0;
            emit recordingChanged();
            emit recordTimeChanged();
            emit recordingStopped(m_recordPath);
            m_recorderProc->deleteLater();
            m_recorderProc = nullptr;
        });

        m_recorderProc->start(m_recorderBin, args);
        m_recording = true;
        m_recordStart = QDateTime::currentDateTime();
        m_recordSeconds = 0;
        m_recordTimer->start();
        emit recordingChanged();
        emit recordTimeChanged();
    });
    slurp->start("slurp", {});
}

void ScreenCapture::stopRecording()
{
    if (!m_recording || !m_recorderProc) return;

    // Send SIGINT for clean finish (writes file headers)
    m_recorderProc->terminate();

    // Force kill after 3 seconds if it doesn't stop
    QTimer::singleShot(3000, this, [this]() {
        if (m_recorderProc && m_recorderProc->state() != QProcess::NotRunning)
            m_recorderProc->kill();
    });
}
