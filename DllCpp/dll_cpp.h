#pragma once

#include <cstdint>
#include <cstddef>

#ifdef DLLCPP_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

extern "C" DLL_API uint32_t CppCrc32cInit();
extern "C" DLL_API uint32_t CppCrc32cFinalize(uint32_t currentCrc);

extern "C" DLL_API uint32_t CppCrc32cCombine(uint32_t crc1, uint32_t crc2, size_t len2);

extern "C" DLL_API uint32_t CppCrc32cUpdateBitwise(uint32_t currentCrc, const uint8_t* data, size_t length);
extern "C" DLL_API uint32_t CppCrc32cUpdateSlicing1(uint32_t currentCrc, const uint8_t* data, size_t length);
extern "C" DLL_API uint32_t CppCrc32cUpdateSlicing8(uint32_t currentCrc, const uint8_t* data, size_t length);

