// Boundary tests for the Rust DLL, called from C++ the way Dockly calls it. Touches no user data.
#include "nativebridge.h"

#include <atomic>
#include <cstring>
#include <thread>
#include <vector>

#include <QDir>
#include <QJsonArray>
#include <QStorageInfo>
#include <QtTest>

#include <windows.h>
#include <psapi.h>

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

    void storageAgreesWithQt()
    {
        const NativeBridge b;
        std::vector<dn_device> found;
        QCOMPARE(b.devices(DN_DEVICE_STORAGE, &found), DN_OK);
        const QStorageInfo system(qEnvironmentVariable("SystemDrive") + "/");
        bool seen = false;
        for (const dn_device &d : found) {
            if (QDir::fromNativeSeparators(d.id).compare(system.rootPath(), Qt::CaseInsensitive) != 0)
                continue;
            seen = true;
            QVERIFY(d.flags & DN_FLAG_HAS_SIZE);
            QCOMPARE(d.total_bytes, uint64_t(system.bytesTotal()));
            QCOMPARE(QString::fromUtf8(d.detail), QString::fromLatin1(system.fileSystemType()));
        }
        QVERIFY2(seen, "the system drive is missing");
    }

    void everyDeviceKindIsWellFormed()
    {
        const NativeBridge b;
        for (uint32_t kind = DN_DEVICE_STORAGE; kind <= DN_DEVICE_SENSOR; ++kind) {
            std::vector<dn_device> found;
            QCOMPARE(b.devices(kind, &found), DN_OK);
            for (const dn_device &d : found) {
                QCOMPARE(d.kind, kind);
                QVERIFY(qstrnlen(d.id, DN_ID_LEN) > 0 && qstrnlen(d.id, DN_ID_LEN) < DN_ID_LEN);
                QVERIFY(qstrnlen(d.name, DN_NAME_LEN) < DN_NAME_LEN);
                QVERIFY(qstrnlen(d.detail, DN_DETAIL_LEN) < DN_DETAIL_LEN);
            }
            qInfo() << "kind" << kind << ":" << found.size() << "devices";
        }
    }

    void deviceListRejectsBadArguments()
    {
        const NativeBridge b;
        const uint32_t size = sizeof(dn_device);
        dn_device one;
        std::memset(&one, 0xAB, sizeof(one));
        const dn_device before = one;
        uint32_t count = 1234;
        QCOMPARE(b.deviceList(0, nullptr, 0, size, &count), DN_ERR_ARG);
        QCOMPARE(b.deviceList(99, nullptr, 0, size, &count), DN_ERR_ARG);
        QCOMPARE(b.deviceList(DN_DEVICE_STORAGE, nullptr, 1, size, &count), DN_ERR_ARG);
        QCOMPARE(b.deviceList(DN_DEVICE_STORAGE, &one, 1, size, nullptr), DN_ERR_ARG);
        QCOMPARE(b.deviceList(DN_DEVICE_STORAGE, &one, 1, size - 1, &count), DN_ERR_SIZE);
        QCOMPARE(b.deviceList(DN_DEVICE_STORAGE, &one, 1, size + 8, &count), DN_ERR_SIZE);
        QCOMPARE(count, 1234u);
        QCOMPARE(b.devices(DN_DEVICE_STORAGE, nullptr), DN_ERR_ARG);
        // too small: the count is reported and the array is not touched
        QCOMPARE(b.deviceList(DN_DEVICE_STORAGE, &one, 0, size, &count), DN_ERR_MORE);
        QVERIFY(count >= 1);
        QCOMPARE(std::memcmp(&one, &before, sizeof(one)), 0);
    }

    void deviceListStaysInsideTheArray()
    {
        const NativeBridge b;
        uint32_t count = 0;
        QCOMPARE(b.deviceList(DN_DEVICE_STORAGE, nullptr, 0, sizeof(dn_device), &count), DN_ERR_MORE);
        std::vector<dn_device> items(count + 1);
        std::memset(items.data(), 0xAB, items.size() * sizeof(dn_device));
        const dn_device guard = items.back();
        QCOMPARE(b.deviceList(DN_DEVICE_STORAGE, items.data(), count, sizeof(dn_device), &count), DN_OK);
        QCOMPARE(std::memcmp(&items.back(), &guard, sizeof(guard)), 0);
    }

    void memoryAgreesWithWindows()
    {
        const NativeBridge b;
        dn_memory_info m{};
        m.struct_size = sizeof(m);
        QCOMPARE(b.memoryInfo(&m), DN_OK);
        PROCESS_MEMORY_COUNTERS_EX c{};
        QVERIFY(K32GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&c), sizeof(c)));
        // two moments, so allow what a test process can move by in between
        const qint64 slack = 8 * 1024 * 1024;
        QVERIFY(qAbs(qint64(m.private_bytes) - qint64(c.PrivateUsage)) < slack);
        QVERIFY(m.peak_working_set_bytes >= m.working_set_bytes);
        QVERIFY(m.committed_image_bytes > 0);
        QVERIFY(m.largest_free_block_bytes <= m.free_bytes);
        QCOMPARE(b.memoryInfo(nullptr), DN_ERR_ARG);
        m.struct_size = sizeof(m) - 1;
        QCOMPARE(b.memoryInfo(&m), DN_ERR_SIZE);
    }

    void regionTotalsFollowARealAllocation()
    {
        const NativeBridge b;
        const auto read = [&] {
            dn_memory_info m{};
            m.struct_size = sizeof(m);
            b.memoryInfo(&m);
            return m;
        };
        const uint64_t size = 64 * 1024 * 1024, slack = 4 * 1024 * 1024;
        const dn_memory_info before = read();
        void *block = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        QVERIFY(block);
        const dn_memory_info held = read();
        QVERIFY(VirtualFree(block, 0, MEM_RELEASE));
        const dn_memory_info released = read();
        QVERIFY(held.committed_private_bytes >= before.committed_private_bytes + size - slack);
        QVERIFY(held.private_bytes >= before.private_bytes + size - slack);
        QVERIFY(released.committed_private_bytes + size <= held.committed_private_bytes + slack);
    }

    // leak check for the DLL's own heap: nothing it allocates outlives a call
    void repeatedCallsLeaveNothingAllocated()
    {
        const NativeBridge b;
        std::vector<dn_device> found;
        const auto round = [&] {
            for (uint32_t kind = DN_DEVICE_STORAGE; kind <= DN_DEVICE_SENSOR; ++kind)
                QCOMPARE(b.devices(kind, &found), DN_OK);
            DeviceWatch watch(b);
            QVERIFY(watch.isOpen());
        };
        const auto read = [&] {
            dn_memory_info m{};
            m.struct_size = sizeof(m);
            b.memoryInfo(&m);
            return m;
        };
        round(); // anything created once is created here
        const dn_memory_info before = read();
        for (int n = 0; n < 50; ++n)
            round();
        const dn_memory_info after = read();
        QVERIFY(after.native_allocations > before.native_allocations);
        QCOMPARE(after.native_live_bytes, before.native_live_bytes);
        QCOMPARE(after.native_allocations - after.native_frees, before.native_allocations - before.native_frees);
        QVERIFY2(after.handle_count <= before.handle_count + 8,
                 qPrintable(QString("handles %1 -> %2").arg(before.handle_count).arg(after.handle_count)));
    }

    void watchHandlesAreChecked()
    {
        const NativeBridge b;
        uint64_t handle = 0, arrivals = 99, removals = 99;
        QCOMPARE(b.watchOpen(nullptr), DN_ERR_ARG);
        QCOMPARE(b.watchOpen(&handle), DN_OK);
        QVERIFY(handle != 0);
        dn_watch_counts c{};
        c.struct_size = sizeof(c);
        QCOMPARE(b.watchRead(handle, &c), DN_OK);
        QCOMPARE(b.watchRead(handle, nullptr), DN_ERR_ARG);
        QCOMPARE(b.watchClose(handle), DN_OK);
        QCOMPARE(b.watchClose(handle), DN_ERR_ARG); // double close
        QCOMPARE(b.watchRead(handle, &c), DN_ERR_ARG); // use after close
        QCOMPARE(b.watchClose(0), DN_ERR_ARG);
        QCOMPARE(b.watchClose(~0ULL), DN_ERR_ARG);

        const DeviceWatch watch(b);
        QVERIFY(watch.read(&arrivals, &removals));
    }

    void watchLimitAndConcurrentUse()
    {
        const NativeBridge b;
        std::vector<uint64_t> open;
        uint64_t handle = 0;
        int status = DN_OK;
        while (open.size() < 200 && (status = b.watchOpen(&handle)) == DN_OK)
            open.push_back(handle);
        QCOMPARE(status, DN_ERR_LIMIT);
        QCOMPARE(open.size(), size_t(64));
        for (const uint64_t h : open)
            QCOMPARE(b.watchClose(h), DN_OK);

        std::atomic<int> failures{0};
        std::vector<std::thread> threads;
        for (int t = 0; t < 8; ++t)
            threads.emplace_back([&] {
                std::vector<dn_device> found;
                for (int n = 0; n < 20; ++n) {
                    DeviceWatch watch(b);
                    if (!watch.read(nullptr, nullptr) || b.devices(DN_DEVICE_HID, &found) != DN_OK)
                        ++failures;
                }
            });
        for (auto &t : threads)
            t.join();
        QCOMPARE(failures.load(), 0);
    }

    void reportCoversEverySection()
    {
        const QJsonObject r = NativeBridge().report();
        QVERIFY(r["available"].toBool());
        QVERIFY(r["system"].toObject().contains("physicalCores"));
        QVERIFY(r["processMemory"].toObject().contains("privateBytes"));
        QCOMPARE(r["devices"].toObject().size(), 8);
        QVERIFY(!r["devices"].toObject()["storage"].toArray().isEmpty());
    }

    void unavailableLibraryDisablesEverything()
    {
        const NativeBridge b("Z:/nowhere/dockly_native.dll");
        std::vector<dn_device> found;
        QCOMPARE(b.devices(DN_DEVICE_USB, &found), DN_ERR_OS);
        dn_memory_info m{};
        m.struct_size = sizeof(m);
        QCOMPARE(b.memoryInfo(&m), DN_ERR_OS);
        const DeviceWatch watch(b);
        QVERIFY(!watch.isOpen() && !watch.read(nullptr, nullptr));
        QVERIFY(!b.report()["available"].toBool());
    }

    // timings, each beside the nearest thing Qt or Win32 already offers (not identical work)
    void benchmarkStorageNative()
    {
        const NativeBridge b;
        std::vector<dn_device> found;
        QBENCHMARK { b.devices(DN_DEVICE_STORAGE, &found); }
    }

    void benchmarkStorageQt()
    {
        QBENCHMARK {
            const auto volumes = QStorageInfo::mountedVolumes();
            for (const QStorageInfo &v : volumes)
                (void)(v.bytesTotal() + v.name().size() + v.fileSystemType().size());
        }
    }

    void benchmarkUsbList()
    {
        const NativeBridge b;
        std::vector<dn_device> found;
        QBENCHMARK { b.devices(DN_DEVICE_USB, &found); }
    }

    void benchmarkHidList()
    {
        const NativeBridge b;
        std::vector<dn_device> found;
        QBENCHMARK { b.devices(DN_DEVICE_HID, &found); }
    }

    void benchmarkMemoryInfo()
    {
        const NativeBridge b;
        dn_memory_info m{};
        m.struct_size = sizeof(m);
        QBENCHMARK { b.memoryInfo(&m); }
    }

    void benchmarkWatchRead()
    {
        const NativeBridge b;
        const DeviceWatch watch(b);
        uint64_t arrivals = 0;
        QBENCHMARK { watch.read(&arrivals, nullptr); }
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
