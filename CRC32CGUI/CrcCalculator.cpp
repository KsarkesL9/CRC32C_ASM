#include "CrcCalculator.h"
#include <QApplication>


CrcCalculator::CrcCalculator() {
}

uint32_t CrcCalculator::calculateChunk(const uint8_t* data, size_t length, Algorithm algo) {
    uint32_t crc = 0;
    switch (algo) {
    case CppBitwise:
        crc = CppCrc32cUpdateBitwise(crc, data, length);
        break;
    case CppSlicing1:
        crc = CppCrc32cUpdateSlicing1(crc, data, length);
        break;
    case CppSlicing8:
        crc = CppCrc32cUpdateSlicing8(crc, data, length);
        break;
    case AsmHardware:
        crc = AsmCrc32cHardwareScalar(crc, data, length);
        break;
    case AsmHardwarePipelining:
        crc = AsmCrc32cHardwarePipelining(crc, data, length);
        break;
    }
    return crc;
}

uint32_t CrcCalculator::processBufferSequential(const uint8_t* buffer, size_t totalSize, Algorithm algo) {
    uint32_t crc = CppCrc32cInit();

    switch (algo) {
        case CppBitwise: crc = CppCrc32cUpdateBitwise(crc, buffer, totalSize); break;
        case CppSlicing1: crc = CppCrc32cUpdateSlicing1(crc, buffer, totalSize); break;
        case CppSlicing8: crc = CppCrc32cUpdateSlicing8(crc, buffer, totalSize); break;
        case AsmHardware: crc = AsmCrc32cHardwareScalar(crc, buffer, totalSize); break;
        case AsmHardwarePipelining: crc = AsmCrc32cHardwarePipelining(crc, buffer, totalSize); break;
    }
    return CppCrc32cFinalize(crc);
}

uint32_t CrcCalculator::processBufferParallel(const uint8_t* buffer, size_t totalSize, const Settings& settings) {
    if (settings.threadCount == 1) {
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

    currentOffset = 0;
    chunkSize = totalSize / settings.threadCount;
    remainder = totalSize % settings.threadCount;

    for (int i = 0; i < settings.threadCount; ++i) {
        size_t size = chunkSize + (i < remainder ? 1 : 0);
        if (size == 0) continue;

        uint32_t chunkCrc = futures[i].get();
        runningCrc = CppCrc32cCombine(runningCrc, chunkCrc, size);

        currentOffset += size;
    }

    return CppCrc32cFinalize(runningCrc);
}

uint32_t CrcCalculator::calculateFromBuffer(const QByteArray& buffer, const Settings& settings, std::function<void(int)> progressCallback) {

    
    const uint8_t* rawData = reinterpret_cast<const uint8_t*>(buffer.constData());
    uint32_t result = processBufferParallel(rawData, buffer.size(), settings);
    
    if (progressCallback) progressCallback(100);
    return result;
}

uint32_t CrcCalculator::calculateFromFile(const QString& filePath, const Settings& settings, qint64 chunkSize, std::function<void(int)> progressCallback) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {

        return 0; 
    }

    uint32_t finalCrc = 0;

    if (chunkSize == -1) {
        QByteArray allData = file.readAll();

        finalCrc = processBufferParallel(reinterpret_cast<const uint8_t*>(allData.constData()), allData.size(), settings);
        if (progressCallback) progressCallback(100);
    }
    else {
        uint32_t runningCrc = CppCrc32cInit();
        qint64 bytesReadTotal = 0;
        qint64 fileSize = file.size();

        while (!file.atEnd()) {
            QByteArray chunk = file.read(chunkSize);
            if (chunk.isEmpty()) break;

            const uint8_t* rawChunk = reinterpret_cast<const uint8_t*>(chunk.constData());

            if (settings.threadCount == 1) {
                 switch (settings.algorithm) {
                    case CppBitwise: runningCrc = CppCrc32cUpdateBitwise(runningCrc, rawChunk, chunk.size()); break;
                    case CppSlicing1: runningCrc = CppCrc32cUpdateSlicing1(runningCrc, rawChunk, chunk.size()); break;
                    case CppSlicing8: runningCrc = CppCrc32cUpdateSlicing8(runningCrc, rawChunk, chunk.size()); break;
                    case AsmHardware: runningCrc = AsmCrc32cHardwareScalar(runningCrc, rawChunk, chunk.size()); break;
                    case AsmHardwarePipelining: runningCrc = AsmCrc32cHardwarePipelining(runningCrc, rawChunk, chunk.size()); break;
                }
            }
            else {
                 std::vector<std::future<uint32_t>> futures;
                 size_t currentChunkSize = chunk.size();
                 size_t subChunkSize = currentChunkSize / settings.threadCount;
                 size_t rem = currentChunkSize % settings.threadCount;
                 size_t offset = 0;

                 for (int i = 0; i < settings.threadCount; ++i) {
                     size_t sz = subChunkSize + (i < rem ? 1 : 0);
                     if (sz == 0) continue;
                     const uint8_t* ptr = rawChunk + offset;
                     
                     futures.push_back(std::async(std::launch::async, [=]() {
                         return calculateChunk(ptr, sz, settings.algorithm);
                     }));
                     offset += sz;
                 }

                 offset = 0;

                 for (int i = 0; i < settings.threadCount; ++i) {
                    size_t sz = subChunkSize + (i < rem ? 1 : 0);
                    if (sz == 0) continue;
                    uint32_t subRes = futures[i].get();
                    runningCrc = CppCrc32cCombine(runningCrc, subRes, sz);
                    offset += sz;
                 }
            }

            bytesReadTotal += chunk.size();
            if (fileSize > 0 && progressCallback) {
                progressCallback(static_cast<int>((bytesReadTotal * 100) / fileSize));
            }
            
            QApplication::processEvents();
        }
        
        finalCrc = CppCrc32cFinalize(runningCrc);
    }
    
    file.close();
    return finalCrc;
}