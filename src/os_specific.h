#pragma once

bool os_file_exists(char *filepath);
bool os_get_file_last_write_time(char *filepath, u64 *modtime_pointer);
char *os_read_entire_file(char *filepath, s64 *length_pointer = NULL);

void os_init_colors_and_utf8();
char *os_get_path_of_running_executable();
void os_set_current_working_directory(char *path);

struct System_Time {
    int year;
    int month;
    int day_of_week;
    int day;
    int hour;
    int minute;
    int second;
    int milliseconds;
};

System_Time os_get_local_time();

double os_get_time();

void os_show_message_box(char *caption, char *text, bool error);
