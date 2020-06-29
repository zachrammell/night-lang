#include <Windows.h>

int main()
{
  HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  WriteConsoleA(hOutput, "Hello World!\n", 13, NULL, NULL);
  return 0;
}
