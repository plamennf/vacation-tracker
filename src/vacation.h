#pragma once

struct Vacation_Info {
    int year;
    int month;
    int from_day;
    int to_day;
};

struct Employee {
    char *name = NULL;
    Array <Vacation_Info> vacations;
    Vector4 color;
    
    Vacation_Info *add_vacation_info(int from_day, int to_day, int month, int year);
};

extern Array <Employee *> all_employees;

Employee *add_employee(char *name, Vector4 color);
