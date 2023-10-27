#include "pch.h"

#ifdef _WIN32

#include "os_specific.h"

#include <windows.h>
#include <stdio.h>

static void to_windows_filepath(char *filepath, wchar_t *wide_filepath, int wide_filepath_size) {
    MultiByteToWideChar(CP_UTF8, 0, filepath, -1, wide_filepath, wide_filepath_size);

    for (wchar_t *at = wide_filepath; *at; at++) {
        if (*at == L'/') {
            *at = L'\\';
        }
    }
}

static void to_normal_filepath(wchar_t *wide_filepath, char *filepath, int filepath_size) {
    WideCharToMultiByte(CP_UTF8, 0, wide_filepath, -1, filepath, filepath_size, NULL, 0);

    for (char *at = filepath; *at; at++) {
        if (*at == '\\') {
            *at = '/';
        }
    }
}

bool os_file_exists(char *filepath) {
    wchar_t wide_filepath[4096];
    to_windows_filepath(filepath, wide_filepath, ArrayCount(wide_filepath));
    return GetFileAttributesW(wide_filepath) != INVALID_FILE_ATTRIBUTES;
}

bool os_get_file_last_write_time(char *filepath, u64 *modtime_pointer) {
    wchar_t wide_filepath[4096];
    to_windows_filepath(filepath, wide_filepath, ArrayCount(wide_filepath));
    
    HANDLE file = CreateFileW(wide_filepath, 0,//GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    defer { CloseHandle(file); };

    FILETIME ft_create, ft_access, ft_write;
    if (!GetFileTime(file, &ft_create, &ft_access, &ft_write)) return false;
    
    ULARGE_INTEGER uli;
    uli.LowPart = ft_write.dwLowDateTime;
    uli.HighPart = ft_write.dwHighDateTime;
    
    if (modtime_pointer) *modtime_pointer = uli.QuadPart;
    
    return true;
}

char *os_read_entire_file(char *filepath, s64 *length_pointer) {
    char *result = NULL;

    FILE *file = fopen(filepath, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        auto length = ftell(file);
        fseek(file, 0, SEEK_SET);

        result = new char[length + 1];
        memset(result, 0, length + 1);
        auto num_read = fread(result, 1, length, file);
        fclose(file);

        if (length_pointer) *length_pointer = num_read;
    }
    return result;
}

void os_init_colors_and_utf8() {
    SetConsoleOutputCP(CP_UTF8);
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD mode = 0;
    BOOL result = GetConsoleMode(stdout_handle, &mode);
    if (result) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(stdout_handle, mode);
    }
}

char *os_get_path_of_running_executable() {
    wchar_t wide_result[MAX_PATH];
    GetModuleFileNameW(nullptr, wide_result, MAX_PATH);

    char result[4096];
    to_normal_filepath(wide_result, result, ArrayCount(result));

    auto len = strlen(result);
    char *copied_result = new char[len + 1];
    memcpy(copied_result, result, len + 1);
    return copied_result;
}

void os_set_current_working_directory(char *path) {
    wchar_t wide_dir[4096];
    to_windows_filepath(path, wide_dir, ArrayCount(wide_dir));
    SetCurrentDirectoryW(wide_dir);
}

System_Time os_get_local_time() {
    SYSTEMTIME system_time = {};
    GetLocalTime(&system_time);

    System_Time result  = {};
    result.year         = system_time.wYear;
    result.month        = system_time.wMonth;
    result.day_of_week  = system_time.wDayOfWeek;
    result.day          = system_time.wDay;
    result.hour         = system_time.wHour;
    result.minute       = system_time.wMinute;
    result.second       = system_time.wSecond;
    result.milliseconds = system_time.wMilliseconds;
    
    return result;
}

double os_get_time() {
    LARGE_INTEGER perf_freq, perf_counter;
    QueryPerformanceFrequency(&perf_freq);
    QueryPerformanceCounter(&perf_counter);
    return (double)perf_counter.QuadPart / (double)perf_freq.QuadPart;
}

#endif
