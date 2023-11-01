#include "pch.h"
#include "vacation.h"

Array <Employee *> all_employees;

Employee *add_employee(char *name) {
    Employee *result = new Employee();
    
    result->name  = copy_string(name);
    result->draw_all_vacations_on_hud = true;
    result->has_vacation_that_overlaps = false;
    
    all_employees.add(result);
    
    return result;
}

Vacation_Info *Employee::add_vacation_info(int from_day, int from_month, int from_year, int to_day, int to_month, int to_year) {
    Vacation_Info *info = vacations.add();
    
    info->from_year    = from_year;
    info->from_month   = from_month;
    info->from_day     = from_day;
    info->to_year      = to_year;
    info->to_month     = to_month;
    info->to_day       = to_day;

    info->is_colliding = false;

    return info;
}

struct Date {
    int year;
    int month;
    int day;

    Date() = default;
    Date(Vacation_Info info, bool from) {
        if (from) {
            year  = info.from_year;
            month = info.from_month;
            day   = info.from_day;
        } else {
            year  = info.to_year;
            month = info.to_month;
            day   = info.to_day;
        }
    }
};

inline bool operator<(Date a, Date b) {
    if (a.day == b.day && a.month == b.month && a.year == b.year)
        return false;

    if (a.year > b.year || a.year == b.year && a.month > b.month ||
        a.year == b.year && a.month == b.month && a.day > b.day)
        return false;

    return true;
}

inline bool operator>(Date a, Date b) {
    if (a.day == b.day && a.month == b.month && a.year == b.year)
        return false;

    if (a.year > b.year || a.year == b.year && a.month > b.month ||
        a.year == b.year && a.month == b.month && a.day > b.day)
        return true;

    return false;
}

inline bool operator==(Date a, Date b) {
    if (a.day == b.day && a.month == b.month && a.year == b.year)
        return true;

    return false;
}

inline bool operator!=(Date a, Date b) {
    if (a.day == b.day && a.month == b.month && a.year == b.year)
        return false;

    return true;
}

inline bool operator<=(Date a, Date b) {
    if (a < b || a == b)
        return true;
    
    return false;
}

inline bool operator>=(Date a, Date b) {
    if (a > b || a == b)
        return true;
    
    return false;
}

static int compare_dates(Date a, Date b) {
    if (a.day == b.day && a.month == b.month && a.year == b.year)
        return 0;

    if (a.year > b.year || a.year == b.year && a.month > b.month ||
        a.year == b.year && a.month == b.month && a.day > b.day)
        return 1;

    return -1;
}

bool are_vacations_colliding() {
    bool are_colliding = false;
    
    for (auto employee : all_employees) {
        //for (auto info : employee->vacations) {
        for (int i = 0; i < employee->vacations.count; i++) {
            auto info = &employee->vacations[i];
            /*
            s64 employee_from_time, employee_to_time;
            get_vacation_time(*info, &employee_from_time, &employee_to_time);
            */

            Date employee_start(*info, true);
            Date employee_end(*info, false);
            
            for (auto other : all_employees) {
                if (employee == other) continue;

                //for (auto other_info : other->vacations) {
                for (int j = 0; j < other->vacations.count; j++) {
                    Vacation_Info *other_info = &other->vacations[j];
                    Date other_start(*other_info, true);
                    Date other_end(*other_info, false);

                    /*
                    if (Max(employee_to_time, other_to_time) - Min(employee_from_time, other_from_time) < (employee_to_time - employee_from_time) + (other_to_time - other_from_time)) {
                    */
                    
                    if (Max(employee_start, other_start) < Min(employee_end, other_end)) {
                        employee->has_vacation_that_overlaps = true;
                        other->has_vacation_that_overlaps = true;

                        info->is_colliding = true;
                        other_info->is_colliding = true;
                        
                        are_colliding = true;
                    }
                }
            }
        }
    }

    return are_colliding;
}
