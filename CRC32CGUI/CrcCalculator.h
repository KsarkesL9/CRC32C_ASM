#pragma once

#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <future>
#include <QByteArray>
#include <QFile>
#include <QString>
#include "../DllCpp/dll_cpp.h"

extern "C" uint32_t AsmCrc32cHardwareScalar(uint32_t currentCrc, const uint8_t* data, size_t length);
extern "C" uint32_t AsmCrc32cHardwarePipelining(uint32_t currentCrc, const uint8_t* data, size_t length);

class CrcCalculator {
public:
    enum Algorithm {
        CppBitwise = 0,
        CppSlicing1 = 1,
        CppSlicing8 = 2,
        AsmHardware = 3,
        AsmHardwarePipelining = 4
    };

    struct Settings {
        Algorithm algorithm;
        int threadCount;
    };

    struct Result {
        uint32_t crc;
        qint64 nanoseconds;
    };

    CrcCalculator();

    Result calculateFromFile(const QString& filePath, const Settings& settings, qint64 chunkSize, std::function<void(int)> progressCallback = nullptr);
    Result calculateFromBuffer(const QByteArray& buffer, const Settings& settings, std::function<void(int)> progressCallback = nullptr);

private:
    uint32_t calculateChunk(const uint8_t* data, size_t length, Algorithm algo);
    uint32_t processBufferParallel(const uint8_t* buffer, size_t totalSize, const Settings& settings);
    uint32_t processBufferSequential(const uint8_t* buffer, size_t totalSize, Algorithm algo);
};