#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QDateTime>

class ScreenCapture : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool hasGrim        READ hasGrim        CONSTANT)
    Q_PROPERTY(bool hasSlurp       READ hasSlurp       CONSTANT)
    Q_PROPERTY(bool hasRecorder    READ hasRecorder    CONSTANT)
    Q_PROPERTY(bool recording      READ recording      NOTIFY recordingChanged)
    Q_PROPERTY(int  recordSeconds  READ recordSeconds  NOTIFY recordTimeChanged)
    Q_PROPERTY(QString lastScreenshot READ lastScreenshot NOTIFY screenshotTaken)

public:
    explicit ScreenCapture(QObject *parent = nullptr);

    bool hasGrim()     const { return m_hasGrim; }
    bool hasSlurp()    const { return m_hasSlurp; }
    bool hasRecorder() const { return m_hasRecorder; }
    bool recording()   const { return m_recording; }
    int  recordSeconds() const { return m_recordSeconds; }
    QString lastScreenshot() const { return m_lastScreenshot; }

    Q_INVOKABLE void screenshotFull();
    Q_INVOKABLE void screenshotRegion();
    Q_INVOKABLE void screenshotDelayed(int seconds);

    Q_INVOKABLE void startRecording(bool withAudio);
    Q_INVOKABLE void startRecordingRegion(bool withAudio);
    Q_INVOKABLE void stopRecording();

signals:
    void screenshotTaken(const QString &path);
    void recordingChanged();
    void recordTimeChanged();
    void recordingStopped(const QString &path);

private:
    void detectTools();
    QString screenshotDir();
    QString recordingDir();
    QString timestamp();
    void copyToClipboard(const QString &path);
    void finishScreenshot(const QString &path);

    bool m_hasGrim     = false;
    bool m_hasSlurp    = false;
    bool m_hasRecorder = false;
    QString m_recorderBin;

    bool     m_recording     = false;
    int      m_recordSeconds = 0;
    QProcess *m_recorderProc = nullptr;
    QTimer   *m_recordTimer  = nullptr;
    QDateTime m_recordStart;
    QString  m_recordPath;
    QString  m_lastScreenshot;
};

#endif
