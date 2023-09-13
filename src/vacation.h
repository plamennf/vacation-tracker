#pragma once

struct Vacation_Info {
    int year;
    int from_month;
    int from_day;
    int to_month;
    int to_day;
};

struct Employee {
    char *name = NULL;
    Array <Vacation_Info> vacations;

    bool draw_all_vacations_on_hud;
    
    Vacation_Info *add_vacation_info(int from_day, int from_month, int to_day, int to_month, int year);
};

extern Array <Employee *> all_employees;

Employee *add_employee(char *name);
