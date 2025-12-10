#pragma once

#include <cstdint>
#include <cstddef>

#ifdef DLLCPP_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

// Funkcje pomocnicze
extern "C" DLL_API int CppAdd(int a, int b);

// Inicjalizacja i Finalizacja (wspólne dla wszystkich metod)
extern "C" DLL_API uint32_t CppCrc32cInit();
extern "C" DLL_API uint32_t CppCrc32cFinalize(uint32_t currentCrc);

// Ró¿ne implementacje funkcji Update
extern "C" DLL_API uint32_t CppCrc32cUpdateBitwise(uint32_t currentCrc, const uint8_t* data, size_t length);
extern "C" DLL_API uint32_t CppCrc32cUpdateSlicing1(uint32_t currentCrc, const uint8_t* data, size_t length);
extern "C" DLL_API uint32_t CppCrc32cUpdateSlicing8(uint32_t currentCrc, const uint8_t* data, size_t length);

// Wrapper domyœlny (mo¿e u¿ywaæ Slicing-by-1 lub 8 zale¿nie od implementacji, tu zostawimy dla kompatybilnoœci)
extern "C" DLL_API uint32_t CppCrc32c(const uint8_t* data, size_t length);