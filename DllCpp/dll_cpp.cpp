#include "dll_cpp.h"
#include <array>
#include <cstring> // dla memcpy (bezpieczne rzutowanie typów)

extern "C" DLL_API int CppAdd(int a, int b) {
    return a + b;
}

// --- Wspólne ---

extern "C" DLL_API uint32_t CppCrc32cInit() {
    return 0xFFFFFFFF;
}

extern "C" DLL_API uint32_t CppCrc32cFinalize(uint32_t currentCrc) {
    return currentCrc ^ 0xFFFFFFFF;
}

// --- Metoda 1: Bit-wise (Bardzo wolna) ---
// Przetwarza dane bit po bicie, bez u¿ycia tablicy pamiêci.
// Dobra do zrozumienia matematyki, tragiczna wydajnoœciowo.

extern "C" DLL_API uint32_t CppCrc32cUpdateBitwise(uint32_t currentCrc, const uint8_t* data, size_t length)
{
    uint32_t crc = currentCrc;
    const uint32_t polynomial = 0x82F63B78;

    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i]; // XOR wejœcia z CRC
        for (int j = 0; j < 8; ++j) {
            if (crc & 1)
                crc = (crc >> 1) ^ polynomial;
            else
                crc >>= 1;
        }
    }
    return crc;
}

// --- Metoda 2 & 3: Slicing (Tablice) ---

namespace {
    // Generuje 8 tablic po 256 wpisów.
    // table[0] to standardowa tablica (Slicing-by-1).
    // table[1..7] to tablice przesuniête, pozwalaj¹ce przetwarzaæ kolejne bajty w jednym kroku.
    std::array<std::array<uint32_t, 256>, 8> GenerateCrc32cTable8() {
        std::array<std::array<uint32_t, 256>, 8> tables;
        const uint32_t polynomial = 0x82F63B78;

        // Generowanie pierwszej tablicy (standardowej)
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (int j = 0; j < 8; ++j) {
                crc = (crc & 1) ? (crc >> 1) ^ polynomial : (crc >> 1);
            }
            tables[0][i] = crc;
        }

        // Generowanie kolejnych 7 tablic na podstawie pierwszej
        // table[k][i] = (table[k-1][i] >> 8) ^ table[0][table[k-1][i] & 0xFF]
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

// --- Metoda 2: Slicing-by-1 (Standardowa) ---

extern "C" DLL_API uint32_t CppCrc32cUpdateSlicing1(uint32_t currentCrc, const uint8_t* data, size_t length)
{
    const auto& tables = GetCrc32cTables();
    const auto& table = tables[0]; // U¿ywamy tylko pierwszej tablicy
    uint32_t crc = currentCrc;

    for (size_t i = 0; i < length; ++i) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ table[index];
    }

    return crc;
}

// --- Metoda 3: Slicing-by-8 (Szybka) ---

extern "C" DLL_API uint32_t CppCrc32cUpdateSlicing8(uint32_t currentCrc, const uint8_t* data, size_t length)
{
    const auto& tables = GetCrc32cTables();
    uint32_t crc = currentCrc;
    size_t i = 0;

    // G³ówna pêtla przetwarzaj¹ca po 8 bajtów
    // Warunek: musimy mieæ co najmniej 8 bajtów do przetworzenia
    for (; i + 8 <= length; i += 8) {
        // Pobieramy 8 bajtów danych (dwa s³owa 32-bitowe dla uproszczenia kodu na x86/x64)
        uint32_t term1;
        uint32_t term2;
        // U¿ywamy memcpy aby unikn¹æ problemów z wyrównaniem pamiêci (alignment)
        // Kompilator i tak zoptymalizuje to do prostego ³adowania rejestrów
        std::memcpy(&term1, data + i, 4);
        std::memcpy(&term2, data + i + 4, 4);

        // XORujemy pierwsze 4 bajty danych z obecnym CRC
        term1 ^= crc;

        // Magia Slicing-by-8:
        // Wykonujemy lookupy dla 8 bajtów równolegle (w sensie zale¿noœci danych)
        // Indeksy s¹ pobierane z poszczególnych bajtów term1 i term2
        crc = tables[7][(term1) & 0xFF] ^
            tables[6][(term1 >> 8) & 0xFF] ^
            tables[5][(term1 >> 16) & 0xFF] ^
            tables[4][(term1 >> 24)] ^
            tables[3][(term2) & 0xFF] ^
            tables[2][(term2 >> 8) & 0xFF] ^
            tables[1][(term2 >> 16) & 0xFF] ^
            tables[0][(term2 >> 24)];
    }

    // Obs³uga "ogona" (pozosta³e 0-7 bajtów) metod¹ standardow¹ Slicing-by-1
    for (; i < length; ++i) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ tables[0][index];
    }

    return crc;
}

// Kompatybilnoœæ wsteczna
extern "C" DLL_API uint32_t CppCrc32c(const uint8_t* data, size_t length)
{
    return CppCrc32cFinalize(CppCrc32cUpdateSlicing1(CppCrc32cInit(), data, length));
}