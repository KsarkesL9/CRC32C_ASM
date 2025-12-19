#include "CrcCalculator.h"
#include <QApplication>
#include <QElapsedTimer>

CrcCalculator::CrcCalculator() {
}

uint32_t CrcCalculator::calculateChunk(const uint8_t* data, size_t length, Algorithm algo) {
    uint32_t crc = 0;
    switch (algo) {
    case CppBitwise: crc = CppCrc32cUpdateBitwise(crc, data, length); break;
    case CppSlicing1: crc = CppCrc32cUpdateSlicing1(crc, data, length); break;
    case CppSlicing8: crc = CppCrc32cUpdateSlicing8(crc, data, length); break;
    case AsmHardware: crc = AsmCrc32cHardwareScalar(crc, data, length); break;
    case AsmHardwarePipelining: crc = AsmCrc32cHardwarePipelining(crc, data, length); break;
    }
    return crc;
}

uint32_t CrcCalculator::processBufferSequential(const uint8_t* buffer, size_t totalSize, Algorithm algo) {
    return calculateChunk(buffer, totalSize, algo);
}

uint32_t CrcCalculator::processBufferParallel(const uint8_t* buffer, size_t totalSize, const Settings& settings) {
    if (settings.threadCount <= 1) {
        return processBufferSequential(buffer, totalSize, settings.algorithm);
    }

    std::vector<std::future<uint32_t>> futures;
    size_t chunkSize = totalSize / settings.threadCount;
    size_t remainder = totalSize % settings.threadCount;
    size_t currentOffset = 0;

    for (int i = 0; i < settings.threadCount; ++i) {
        size_t size = chunkSize + (i < remainder ? 1 : 0);
        if (size == 0) continue;
        const uint8_t* chunkPtr = buffer + currentOffset;
        futures.push_back(std::async(std::launch::async, [=]() {
            return calculateChunk(chunkPtr, size, settings.algorithm);
            }));
        currentOffset += size;
    }

    uint32_t runningCrc = CppCrc32cInit();
    for (size_t i = 0; i < futures.size(); ++i) {
        size_t size = chunkSize + (i < remainder ? 1 : 0);
        runningCrc = CppCrc32cCombine(runningCrc, futures[i].get(), size);
    }
    return runningCrc;
}

CrcCalculator::Result CrcCalculator::calculateFromBuffer(const QByteArray& buffer, const Settings& settings, std::function<void(int)> progressCallback) {
    QElapsedTimer timer;
    timer.start();

    const uint8_t* rawData = reinterpret_cast<const uint8_t*>(buffer.constData());
    uint32_t crc = processBufferParallel(rawData, buffer.size(), settings);
    uint32_t finalCrc = CppCrc32cFinalize(crc);

    qint64 elapsed = timer.nsecsElapsed();
    if (progressCallback) progressCallback(100);

    return { finalCrc, elapsed };
}

CrcCalculator::Result CrcCalculator::calculateFromFile(const QString& filePath, const Settings& settings, qint64 chunkSize, std::function<void(int)> progressCallback) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return { 0, 0 };

    QElapsedTimer timer;
    timer.start();
    uint32_t runningCrc = CppCrc32cInit();

    if (chunkSize == -1) {
        QByteArray allData = file.readAll();
        runningCrc = processBufferParallel(reinterpret_cast<const uint8_t*>(allData.constData()), allData.size(), settings);
    }
    else {
        qint64 bytesReadTotal = 0;
        qint64 fileSize = file.size();
        while (!file.atEnd()) {
            QByteArray chunk = file.read(chunkSize);
            if (chunk.isEmpty()) break;

            const uint8_t* rawChunk = reinterpret_cast<const uint8_t*>(chunk.constData());
            if (settings.threadCount <= 1) {
                runningCrc = calculateChunk(rawChunk, chunk.size(), settings.algorithm);
            }
            else {
                runningCrc = CppCrc32cCombine(runningCrc, processBufferParallel(rawChunk, chunk.size(), settings), chunk.size());
            }

            bytesReadTotal += chunk.size();
            if (fileSize > 0 && progressCallback) progressCallback(static_cast<int>((bytesReadTotal * 100) / fileSize));
        }
    }

    uint32_t finalCrc = CppCrc32cFinalize(runningCrc);
    qint64 elapsed = timer.nsecsElapsed();
    file.close();

    return { finalCrc, elapsed };
}