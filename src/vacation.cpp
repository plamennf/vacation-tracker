#include "pch.h"
#include "vacation.h"

Array <Employee *> all_employees;

Employee *add_employee(char *name) {
    Employee *result = new Employee();
    
    result->name  = copy_string(name);
    result->draw_all_vacations_on_hud = true;
    
    all_employees.add(result);
    
    return result;
}

Vacation_Info *Employee::add_vacation_info(int from_day, int from_month, int from_year, int to_day, int to_month, int to_year) {
    Vacation_Info *info = vacations.add();
    
    info->from_year  = from_year;
    info->from_month = from_month;
    info->from_day   = from_day;
    info->to_year    = to_year;
    info->to_month   = to_month;
    info->to_day     = to_day;

    return info;
}
