#include "dll_cpp.h"

extern "C" DLL_API int CppAdd(int a, int b)
{
    return a + b;
}