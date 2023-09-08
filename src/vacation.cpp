#include "pch.h"
#include "vacation.h"

Array <Employee *> all_employees;

Employee *add_employee(char *name, Vector4 color) {
    Employee *result = new Employee();
    
    result->name  = copy_string(name);
    result->color = color;
    
    all_employees.add(result);
    
    return result;
}

Vacation_Info *Employee::add_vacation_info(int from_day, int to_day, int month, int year) {
    Vacation_Info *info = vacations.add();

    info->year     = year;
    info->month    = month;
    info->from_day = from_day;
    info->to_day   = to_day;

    return info;
}
