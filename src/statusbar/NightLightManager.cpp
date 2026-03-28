#include "NightLightManager.h"
#include <QDebug>

NightLightManager::NightLightManager(QObject *parent)
    : QObject(parent)
{
    detectTool();
}

NightLightManager::~NightLightManager()
{
    stopProcess();
}

void NightLightManager::detectTool()
{
    auto check = [](const QString &name) -> bool {
        QProcess p;
        p.start("which", {name});
        p.waitForFinished(1000);
        return p.exitCode() == 0;
    };

    if (check("wlsunset")) {
        m_available = true;
        m_toolBin = "wlsunset";
    } else if (check("gammastep")) {
        m_available = true;
        m_toolBin = "gammastep";
    }

    qDebug() << "NightLight:" << (m_available
        ? "using " + m_toolBin : "not available");
}

void NightLightManager::toggle()
{
    if (m_active)
        disable();
    else
        enable();
}

void NightLightManager::enable()
{
    if (!m_available || m_active) return;
    startProcess();
}

void NightLightManager::disable()
{
    if (!m_active) return;
    stopProcess();
}

void NightLightManager::setTemperature(int temp)
{
    temp = qBound(1000, temp, 6500);
    if (temp == m_temperature) return;
    m_temperature = temp;
    emit temperatureChanged();

    // Restart if active
    if (m_active) {
        stopProcess();
        startProcess();
    }
}

void NightLightManager::startProcess()
{
    m_proc = new QProcess(this);

    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus) {
        m_active = false;
        emit activeChanged();
        m_proc->deleteLater();
        m_proc = nullptr;
    });

    QStringList args;
    if (m_toolBin == "wlsunset") {
        // wlsunset -T high -t low
        // Use fixed temp for both = constant night light
        args << "-T" << QString::number(m_temperature)
             << "-t" << QString::number(m_temperature);
    } else {
        // gammastep -O temperature
        args << "-O" << QString::number(m_temperature);
    }

    m_proc->start(m_toolBin, args);
    m_active = true;
    emit activeChanged();
}

void NightLightManager::stopProcess()
{
    if (m_proc) {
        m_proc->terminate();
        m_proc->waitForFinished(2000);
        if (m_proc && m_proc->state() != QProcess::NotRunning)
            m_proc->kill();
    }

    // wlsunset resets gamma on exit
    // gammastep needs explicit reset
    if (m_toolBin == "gammastep") {
        QProcess::execute("gammastep", {"-x"});
    }

    m_active = false;
    emit activeChanged();
}
