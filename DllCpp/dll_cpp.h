#pragma once

#ifdef DLLCPP_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

extern "C" DLL_API int CppAdd(int a, int b);
