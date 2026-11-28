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
        QVERIFY(b.summary().startsWith("native layer ABI 1:"));
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

};

QTEST_GUILESS_MAIN(NativeTest)
#include "native_test.moc"
