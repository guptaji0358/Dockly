#include "perfmonitor.h"

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <QByteArray>
#endif

struct PerfMonitor::Gpu
{
#ifdef Q_OS_WIN
    PDH_HQUERY query = nullptr;
    PDH_HCOUNTER counter = nullptr;
    bool primed = false;
#endif
};

namespace {
constexpr int kHotSeconds = 5;   // load must stay high this long before Lite switches on
constexpr int kCoolSeconds = 8;  // ...and stay calm this long before it switches off
constexpr int kCoolMargin = 15;  // "calm" = this many points under the threshold, so it doesn't flap
}

PerfMonitor::PerfMonitor(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &PerfMonitor::sample);
}

PerfMonitor::~PerfMonitor()
{
#ifdef Q_OS_WIN
    if (m_g && m_g->query)
        PdhCloseQuery(m_g->query);
#endif
    delete m_g;
}

void PerfMonitor::setManual(bool on)
{
    if (m_manual == on)
        return;
    const bool was = lite();
    m_manual = on;
    applyState(was);
}

void PerfMonitor::configure(bool autoOn, int threshold)
{
    m_threshold = std::clamp(threshold, 40, 100);
    if (m_autoOn != autoOn) {
        m_autoOn = autoOn;
        if (!autoOn && m_auto) {
            const bool was = lite();
            m_auto = false;
            applyState(was);
        }
        m_hot = m_cool = 0;
    }
    updateTimer();
}

void PerfMonitor::updateTimer()
{
    // the numbers are only read while someone needs them (auto mode); a manual switch costs nothing
    const bool want = m_autoOn;
    if (want == m_timer.isActive())
        return;
    if (want) {
        m_lastIdle = m_lastTotal = 0;
        m_timer.start();
        sample();
    } else {
        m_timer.stop();
    }
    emit monitoringChanged();
}

void PerfMonitor::applyState(bool wasLite)
{
    if (wasLite != lite())
        emit liteChanged();
}

void PerfMonitor::sample()
{
    m_cpu = readCpu();
    m_ram = readRam();
    m_gpu = readGpu();
    emit sampled();

    const int worst = std::max({m_cpu, m_ram, m_gpu});
    const bool wasLite = lite();
    if (worst >= m_threshold) {
        ++m_hot;
        m_cool = 0;
        if (!m_auto && m_hot >= kHotSeconds)
            m_auto = true;
    } else if (worst < m_threshold - kCoolMargin) {
        ++m_cool;
        m_hot = 0;
        if (m_auto && m_cool >= kCoolSeconds)
            m_auto = false;
    } else {
        m_hot = m_cool = 0;
    }
    applyState(wasLite);
}

int PerfMonitor::readCpu()
{
#ifdef Q_OS_WIN
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user))
        return m_cpu;
    auto q = [](const FILETIME &f) { return (quint64(f.dwHighDateTime) << 32) | f.dwLowDateTime; };
    const quint64 i = q(idle), t = q(kernel) + q(user); // kernel time already includes idle
    int out = m_cpu;
    if (m_lastTotal && t > m_lastTotal)
        out = int(100.0 * double((t - m_lastTotal) - (i - m_lastIdle)) / double(t - m_lastTotal) + 0.5);
    m_lastIdle = i;
    m_lastTotal = t;
    return std::clamp(out, 0, 100);
#else
    return 0;
#endif
}

int PerfMonitor::readRam() const
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms))
        return int(ms.dwMemoryLoad);
#endif
    return 0;
}

int PerfMonitor::readGpu()
{
#ifdef Q_OS_WIN
    // Windows' own GPU counters (the ones Task Manager shows). Not every machine has them: then -1 and GPU is ignored.
    if (!m_g)
        m_g = new Gpu;
    if (!m_g->query) {
        if (PdhOpenQueryW(nullptr, 0, &m_g->query) != ERROR_SUCCESS
            || PdhAddEnglishCounterW(m_g->query, LR"(\GPU Engine(*)\Utilization Percentage)", 0, &m_g->counter) != ERROR_SUCCESS) {
            if (m_g->query)
                PdhCloseQuery(m_g->query);
            m_g->query = nullptr;
            m_g->primed = true; // don't retry every second
            return -1;
        }
    }
    if (!m_g->counter)
        return -1;
    if (PdhCollectQueryData(m_g->query) != ERROR_SUCCESS)
        return m_gpu;
    if (!m_g->primed) { // a rate counter needs two samples
        m_g->primed = true;
        return m_gpu;
    }
    DWORD bytes = 0, count = 0;
    PDH_STATUS st = PdhGetFormattedCounterArrayW(m_g->counter, PDH_FMT_DOUBLE, &bytes, &count, nullptr);
    if (st != PDH_MORE_DATA || bytes == 0)
        return m_gpu;
    QByteArray buf(int(bytes), 0);
    auto *items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W *>(buf.data());
    if (PdhGetFormattedCounterArrayW(m_g->counter, PDH_FMT_DOUBLE, &bytes, &count, items) != ERROR_SUCCESS)
        return m_gpu;
    // the busiest engine type (3D, copy, video ...) is the GPU's load; each type is the sum over processes
    double sum3d = 0, sumAny = 0;
    for (DWORD k = 0; k < count; ++k) {
        const QString name = QString::fromWCharArray(items[k].szName);
        const double v = items[k].FmtValue.doubleValue;
        sumAny = std::max(sumAny, v);
        if (name.contains(QLatin1String("engtype_3D")))
            sum3d += v;
    }
    return std::clamp(int(std::max(sum3d, sumAny) + 0.5), 0, 100);
#else
    return -1;
#endif
}
