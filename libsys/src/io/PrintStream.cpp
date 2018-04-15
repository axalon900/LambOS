#include <io/PrintStream.hpp>
#include <stdlib.h>

void PrintStream::print(const char *str)
{
    char currLetter;
    while((currLetter = *str++)) print(currLetter);
}

void PrintStream::print(int num)
{
    char intstr[33];
    itoa(num, intstr, 10);
    print(intstr);
}