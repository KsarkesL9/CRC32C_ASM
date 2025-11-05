#include <iostream>
#include "../DllCpp/dll_cpp.h" 
extern "C" int AsmAdd(int a, int b);

int main()
{
    int liczba1 = 10;
    int liczba2 = 25;

    int wynikCpp = CppAdd(liczba1, liczba2);
    std::cout << "Wynik z DllCpp (C++): " << wynikCpp << std::endl;

    int wynikAsm = AsmAdd(liczba1, liczba2);
    std::cout << "Wynik z DllAsm (MASM x64): " << wynikAsm << std::endl;


    return 0;
}