#include "dll_cpp.h"
#include <array>
#include <cstring>

extern "C" DLL_API uint32_t CppCrc32cInit() {
    return 0xFFFFFFFF;
}

extern "C" DLL_API uint32_t CppCrc32cFinalize(uint32_t currentCrc) {
    return currentCrc ^ 0xFFFFFFFF;
}

namespace {
    const uint32_t POLY = 0x82F63B78;

    uint32_t gf2_matrix_times(const uint32_t* mat, uint32_t vec) {
        uint32_t sum = 0;
        int idx = 0;
        while (vec) {
            if (vec & 1)
                sum ^= mat[idx];
            vec >>= 1;
            idx++;
        }
        return sum;
    }

    void gf2_matrix_multiply(uint32_t* dest, const uint32_t* mat1, const uint32_t* mat2) {
        for (int i = 0; i < 32; i++) {
            dest[i] = gf2_matrix_times(mat1, mat2[i]);
        }
    }

    uint32_t crc32c_combine_impl(uint32_t crc1, uint32_t crc2, size_t len2) {
        if (len2 == 0) return crc1 ^ crc2;

        uint32_t odd[32];
        uint32_t even[32];

        for (int n = 0; n < 32; n++) {
            uint32_t c = (1u << n);
            for (int k = 0; k < 8; k++) {
                if (c & 1)
                    c = (c >> 1) ^ POLY;
                else
                    c >>= 1;
            }
            odd[n] = c;
        }

        for (int n = 0; n < 32; n++) {
            even[n] = (1u << n);
        }

        uint32_t tmp[32];
        uint32_t* result = even;
        uint32_t* base = odd;

        while (len2 > 0) {
            if (len2 & 1) {
                gf2_matrix_multiply(tmp, result, base);
                std::memcpy(result, tmp, 32 * sizeof(uint32_t));
            }

            len2 >>= 1;
            if (len2 == 0) break;

            gf2_matrix_multiply(tmp, base, base);
            std::memcpy(base, tmp, 32 * sizeof(uint32_t));
        }

        uint32_t shiftedCrc1 = gf2_matrix_times(result, crc1);

        return shiftedCrc1 ^ crc2;
    }
}

extern "C" DLL_API uint32_t CppCrc32cCombine(uint32_t crc1, uint32_t crc2, size_t len2) {
    return crc32c_combine_impl(crc1, crc2, len2);
}

extern "C" DLL_API uint32_t CppCrc32cUpdateBitwise(uint32_t currentCrc, const uint8_t* data, size_t length)
{
    uint32_t crc = currentCrc;
    const uint32_t polynomial = 0x82F63B78;

    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1)
                crc = (crc >> 1) ^ polynomial;
            else
                crc >>= 1;
        }
    }
    return crc;
}

namespace {
    std::array<std::array<uint32_t, 256>, 8> GenerateCrc32cTable8() {
        std::array<std::array<uint32_t, 256>, 8> tables;
        const uint32_t polynomial = 0x82F63B78;

        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (int j = 0; j < 8; ++j) {
                crc = (crc & 1) ? (crc >> 1) ^ polynomial : (crc >> 1);
            }
            tables[0][i] = crc;
        }

        for (int i = 0; i < 256; i++) {
            for (int k = 1; k < 8; k++) {
                uint32_t prev = tables[k - 1][i];
                tables[k][i] = (prev >> 8) ^ tables[0][prev & 0xFF];
            }
        }

        return tables;
    }

    const std::array<std::array<uint32_t, 256>, 8>& GetCrc32cTables() {
        static const auto tables = GenerateCrc32cTable8();
        return tables;
    }
}

extern "C" DLL_API uint32_t CppCrc32cUpdateSlicing1(uint32_t currentCrc, const uint8_t* data, size_t length)
{
    const auto& tables = GetCrc32cTables();
    const auto& table = tables[0];
    uint32_t crc = currentCrc;

    for (size_t i = 0; i < length; ++i) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ table[index];
    }

    return crc;
}

extern "C" DLL_API uint32_t CppCrc32cUpdateSlicing8(uint32_t currentCrc, const uint8_t* data, size_t length)
{
    const auto& tables = GetCrc32cTables();
    uint32_t crc = currentCrc;
    size_t i = 0;

    for (; i + 8 <= length; i += 8) {
        uint32_t term1;
        uint32_t term2;
        std::memcpy(&term1, data + i, 4);
        std::memcpy(&term2, data + i + 4, 4);

        term1 ^= crc;

        crc = tables[7][(term1) & 0xFF] ^
            tables[6][(term1 >> 8) & 0xFF] ^
            tables[5][(term1 >> 16) & 0xFF] ^
            tables[4][(term1 >> 24)] ^
            tables[3][(term2) & 0xFF] ^
            tables[2][(term2 >> 8) & 0xFF] ^
            tables[1][(term2 >> 16) & 0xFF] ^
            tables[0][(term2 >> 24)];
    }

    for (; i < length; ++i) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ tables[0][index];
    }

    return crc;
}

