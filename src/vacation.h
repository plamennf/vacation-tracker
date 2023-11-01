#pragma once

struct Vacation_Info {
    int from_year;
    int from_month;
    int from_day;
    int to_year;
    int to_month;
    int to_day;

    bool is_colliding;
};

struct Employee {
    char *name = NULL;
    Array <Vacation_Info> vacations;

    bool has_vacation_that_overlaps = false;
    bool draw_all_vacations_on_hud = true;
    
    Vacation_Info *add_vacation_info(int from_day, int from_month, int from_year, int to_day, int to_month, int to_year);
};

extern Array <Employee *> all_employees;

Employee *add_employee(char *name);

bool are_vacations_colliding();
