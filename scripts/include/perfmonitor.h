#pragma once

#include <QObject>
#include <QTimer>

// Watches the machine's CPU, RAM and GPU load and decides when Dockly should switch to Lite mode:
// fewer graphics (no animations, glow or blur), cheaper icons, and Dockly's own background work slowed down.
// Lite mode is on when the user switched it on by hand, or (liteAuto) when any of the three stays at or above
// its own threshold (liteCpu / liteGpu / liteRam) for a few seconds; it drops back once all three have calmed down. Dockly never touches other programs.
class PerfMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cpu READ cpu NOTIFY sampled)
    Q_PROPERTY(int ram READ ram NOTIFY sampled)
    Q_PROPERTY(int gpu READ gpu NOTIFY sampled)  // -1 = not available on this machine
    Q_PROPERTY(bool lite READ lite NOTIFY liteChanged)
    Q_PROPERTY(bool autoTriggered READ autoTriggered NOTIFY liteChanged) // lite is on because of load, not the manual switch
    Q_PROPERTY(bool monitoring READ monitoring NOTIFY monitoringChanged)
public:
    explicit PerfMonitor(QObject *parent = nullptr);
    ~PerfMonitor() override;

    int cpu() const { return m_cpu; }
    int ram() const { return m_ram; }
    int gpu() const { return m_gpu; }
    bool lite() const { return m_manual || m_auto; }
    bool autoTriggered() const { return m_auto && !m_manual; }
    bool monitoring() const { return m_timer.isActive(); }

    void setManual(bool on);
    // auto: watch the load and switch by itself; one threshold per resource, in percent
    void configure(bool autoOn, int cpuAt, int gpuAt, int ramAt);

signals:
    void sampled();
    void liteChanged();
    void monitoringChanged();

private:
    void sample();
    int readCpu();
    int readRam() const;
    int readGpu();
    void applyState(bool wasLite);
    void updateTimer();

    QTimer m_timer;
    bool m_manual = false;
    bool m_autoOn = false;
    bool m_auto = false;
    int m_cpuAt = 50, m_gpuAt = 60, m_ramAt = 60;
    int m_cpu = 0, m_ram = 0, m_gpu = -1;
    int m_hot = 0, m_cool = 0;   // consecutive seconds above / well below the threshold

    quint64 m_lastIdle = 0, m_lastTotal = 0;
    struct Gpu;
    Gpu *m_g = nullptr;
};
