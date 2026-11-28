// Boundary tests for the Rust DLL, called from C++ the way Dockly calls it. Touches no user data.
#include "nativebridge.h"

#include <atomic>
#include <cstring>
#include <thread>
#include <vector>

#include <QtTest>

#include <windows.h>

class NativeTest : public QObject
{
    Q_OBJECT

    static dn_system_info blank(uint32_t size = sizeof(dn_system_info))
    {
        dn_system_info i;
        std::memset(&i, 0xAB, sizeof(i));
        i.struct_size = size;
        return i;
    }

private slots:
    void loadsFromTheDefaultPath()
    {
        const NativeBridge b;
        QVERIFY2(b.isAvailable(), qPrintable(b.error()));
        QVERIFY(b.summary().startsWith("native layer ABI 2:"));
    }

    void valuesAgreeWithWindows()
    {
        const NativeBridge b;
        dn_system_info i = blank();
        QCOMPARE(b.systemInfo(&i), DN_OK);
        MEMORYSTATUSEX ms{};
        ms.dwLength = sizeof(ms);
        QVERIFY(GlobalMemoryStatusEx(&ms));
        QCOMPARE(i.struct_size, uint32_t(sizeof(i)));
        QCOMPARE(i.total_physical_bytes, uint64_t(ms.ullTotalPhys));
        QCOMPARE(i.logical_processors, uint32_t(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS)));
        QVERIFY(i.physical_cores >= 1 && i.physical_cores <= i.logical_processors);
        QVERIFY(i.processor_packages >= 1);
        QVERIFY(i.available_physical_bytes <= i.total_physical_bytes);
        QVERIFY(i.memory_load_percent <= 100);
        QVERIFY(i.os_major >= 10);
        QVERIFY(i.os_build > 0);
        qInfo().noquote() << b.summary();
    }

    void rejectsBadPointers()
    {
        const NativeBridge b;
        QCOMPARE(b.systemInfo(nullptr), DN_ERR_ARG);
        alignas(dn_system_info) unsigned char raw[sizeof(dn_system_info) + 1] = {};
        QCOMPARE(b.systemInfo(reinterpret_cast<dn_system_info *>(raw + 1)), DN_ERR_ARG);
    }

    void shortStructIsLeftUnchanged()
    {
        const NativeBridge b;
        for (const uint32_t size : {0u, 4u, uint32_t(sizeof(dn_system_info) - 1)}) {
            dn_system_info i = blank(size);
            const dn_system_info before = i;
            QCOMPARE(b.systemInfo(&i), DN_ERR_SIZE);
            QCOMPARE(std::memcmp(&i, &before, sizeof(i)), 0);
        }
    }

    void largerCallerStructKeepsItsTail()
    {
        const NativeBridge b;
        struct { dn_system_info info; uint64_t tail; } bigger{blank(sizeof(dn_system_info) + 8), 0x1122334455667788ULL};
        QCOMPARE(b.systemInfo(&bigger.info), DN_OK);
        QCOMPARE(bigger.info.struct_size, uint32_t(sizeof(dn_system_info) + 8));
        QCOMPARE(bigger.tail, uint64_t(0x1122334455667788ULL));
    }

    void callableFromManyThreads()
    {
        const NativeBridge b;
        std::atomic<int> failures{0};
        std::vector<std::thread> threads;
        for (int t = 0; t < 8; ++t)
            threads.emplace_back([&] {
                for (int n = 0; n < 500; ++n) {
                    dn_system_info i = blank();
                    if (b.systemInfo(&i) != DN_OK || i.physical_cores == 0)
                        ++failures;
                }
            });
        for (auto &t : threads)
            t.join();
        QCOMPARE(failures.load(), 0);
    }

    void repeatedCallsDoNotLeakHandles()
    {
        const NativeBridge b;
        dn_system_info i = blank();
        QCOMPARE(b.systemInfo(&i), DN_OK); // anything created once is created here
        DWORD before = 0, after = 0;
        QVERIFY(GetProcessHandleCount(GetCurrentProcess(), &before));
        for (int n = 0; n < 10000; ++n) {
            i = blank();
            QCOMPARE(b.systemInfo(&i), DN_OK);
        }
        QVERIFY(GetProcessHandleCount(GetCurrentProcess(), &after));
        QVERIFY2(after <= before + 2, qPrintable(QString("handles %1 -> %2").arg(before).arg(after)));
    }

    void missingLibraryIsReportedNotFatal()
    {
        const NativeBridge b("Z:/nowhere/dockly_native.dll");
        QVERIFY(!b.isAvailable());
        QVERIFY(!b.error().isEmpty());
        dn_system_info i = blank();
        QCOMPARE(b.systemInfo(&i), DN_ERR_OS);
        QVERIFY(b.summary().startsWith("native layer unavailable"));
    }

    void foreignLibraryIsRefused()
    {
        const NativeBridge b(qEnvironmentVariable("windir") + "/System32/version.dll");
        QVERIFY(!b.isAvailable());
        QCOMPARE(b.error(), QString("not a Dockly native library"));
    }

    void otherAbiVersionIsRefused()
    {
        const NativeBridge b(QStringLiteral(DN_BADABI_PATH));
        QVERIFY(!b.isAvailable());
        QVERIFY2(b.error().startsWith("ABI version"), qPrintable(b.error()));
    }

    // timing only: what one call across the boundary costs, beside the plain Win32 memory query
    void benchmarkSystemInfo()
    {
        const NativeBridge b;
        dn_system_info i = blank();
        QBENCHMARK { b.systemInfo(&i); }
    }

    void benchmarkWin32MemoryStatus()
    {
        MEMORYSTATUSEX ms{};
        ms.dwLength = sizeof(ms);
        QBENCHMARK { GlobalMemoryStatusEx(&ms); }
    }

    void benchmarkBoundaryOnly()
    {
        const NativeBridge b;
        QBENCHMARK { b.systemInfo(nullptr); }
    }
};

QTEST_GUILESS_MAIN(NativeTest)
#include "native_test.moc"
