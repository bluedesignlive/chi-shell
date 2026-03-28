#ifndef NIGHTLIGHTMANAGER_H
#define NIGHTLIGHTMANAGER_H

#include <QObject>
#include <QProcess>

class NightLightManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool active    READ active    NOTIFY activeChanged)
    Q_PROPERTY(int  temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)

public:
    explicit NightLightManager(QObject *parent = nullptr);
    ~NightLightManager();

    bool available() const { return m_available; }
    bool active()    const { return m_active; }
    int  temperature() const { return m_temperature; }

    void setTemperature(int temp);

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void enable();
    Q_INVOKABLE void disable();

signals:
    void activeChanged();
    void temperatureChanged();

private:
    void detectTool();
    void startProcess();
    void stopProcess();

    bool     m_available = false;
    bool     m_active    = false;
    int      m_temperature = 3500;
    QString  m_toolBin;
    QProcess *m_proc = nullptr;
};

#endif
