#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

std::wstring GetLastErrorAsString()
{
  //Get the error message, if any.
  DWORD const errorMessageID = ::GetLastError();
  if (errorMessageID == 0)
  {
    return std::wstring{}; //No error message has been recorded
  }

  LPWSTR messageBuffer = nullptr;
  size_t const size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

  std::wstring message{ messageBuffer, size };

  //Free the buffer.
  LocalFree(messageBuffer);

  return message;
}

std::wstring GetExecutableFilePath()
{
  WCHAR filename_buffer[MAX_PATH];
  DWORD const length = GetModuleFileNameW(
    NULL,
    filename_buffer,
    sizeof filename_buffer / sizeof filename_buffer[0]
  );
  return std::wstring{ filename_buffer, length };
}

int wmain(int argc, WCHAR** argv)
{
  if (argc < 2)
  {
    std::wcerr << L"No input file provided.\n";
    return EXIT_FAILURE;
  }

  fs::path exe_path = GetExecutableFilePath();

  std::wcout << L"This exe: " << exe_path.generic_wstring() << std::endl;

  std::vector<fs::path> infile_paths;
  std::wstring temp_obj_filename { fs::temp_directory_path().append("ngt_tmp.obj").generic_wstring() };
  std::wstring out_exe_name = L"output.exe";
  for (int i = 1; i < argc; ++i)
  {
    if (std::wstring{ argv[i] } == L"-o")
    {
      ++i;
      if (i == argc)
      {
        std::wcerr << L"Warning: -o must be followed by desired output file.\n";
        break;
      }
      out_exe_name = argv[i];
      continue;
    }
    fs::path infile{fs::current_path().append(argv[i])};
    if (fs::exists(infile))
    {
      infile_paths.emplace_back(infile.generic_wstring());
    }
    else
    {
      std::wcerr << L"Error: File does not exist: " << infile.generic_wstring() << std::endl;
      return EXIT_FAILURE;
    }
  }

  {
    std::wstringstream cli_nasm;
    cli_nasm << L"NASM -fwin64 -g ";
    cli_nasm << L"-o " << temp_obj_filename << L" ";
    for (fs::path const& p : infile_paths)
    {
      cli_nasm << p.native() << L" ";
    } 

    std::wcout << L"Going to run process:\n" << cli_nasm.str() << std::endl;

    STARTUPINFOW si_nasm = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi_nasm;
    if (!CreateProcessW(
      exe_path.parent_path().append("nasm.exe").generic_wstring().c_str(),
      cli_nasm.str().data(),
      NULL,
      NULL,
      FALSE,
      0,//CREATE_NO_WINDOW,
      NULL,
      NULL,
      &si_nasm,
      &pi_nasm
      ))
    {
      std::wcerr << L"Failed to start NASM.\n" << GetLastErrorAsString() << std::endl;
      return EXIT_FAILURE;
    }
    std::wcout << L"NASM PID: " << pi_nasm.dwProcessId << std::endl;
    if (WaitForSingleObject(pi_nasm.hProcess, INFINITE) == WAIT_OBJECT_0)
    {
      std::wcout << L"NASM finished.\n";
      CloseHandle(pi_nasm.hProcess);
      CloseHandle(pi_nasm.hThread);
    }
  }

  {
    std::wstringstream cli_golink;
    cli_golink << L"GoLink /console /entry main ";
    cli_golink << L"/fo " << std::filesystem::current_path().append(out_exe_name).generic_wstring() << L" ";
    cli_golink << L"kernel32.dll user32.dll gdi32.dll ";
    cli_golink << temp_obj_filename;

    std::wcout << L"Going to run process:\n" << cli_golink.str() << std::endl;

    STARTUPINFOW si_golink = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi_golink;
    if (!CreateProcessW(
      exe_path.parent_path().append("golink.exe").generic_wstring().c_str(),
      cli_golink.str().data(),
      NULL,
      NULL,
      FALSE,
      0,//CREATE_NO_WINDOW,
      NULL,
      NULL,
      &si_golink,
      &pi_golink
      ))
    {
      std::wcerr << L"Failed to start GoLink.\n" << GetLastErrorAsString() << std::endl;
      return EXIT_FAILURE;
    }
    std::wcout << L"GoLink PID: " << pi_golink.dwProcessId << std::endl;
    if (WaitForSingleObject(pi_golink.hProcess, INFINITE) == WAIT_OBJECT_0)
    {
      std::wcout << L"GoLink finished.\n";
      CloseHandle(pi_golink.hProcess);
      CloseHandle(pi_golink.hThread);
    }
  }

  return EXIT_SUCCESS;
}
