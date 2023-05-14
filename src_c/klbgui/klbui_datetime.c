#include "klbgui/klbui_datetime.h"
#include "klbgui/klb_gui_in.h"
#include <assert.h>


void klb_gui_set_datefmt(klb_gui_t* p_gui, int fmt)
{
    p_gui->datefmt = fmt;
}

int klb_gui_get_datefmt(klb_gui_t* p_gui)
{
    return p_gui->datefmt;
}

void klb_gui_set_timefmt(klb_gui_t* p_gui, int fmt)
{
    p_gui->timefmt = fmt;
}

int klb_gui_get_timefmt(klb_gui_t* p_gui)
{
    return p_gui->timefmt;
}

//////////////////////////////////////////////////////////////////////////
// 日历相关

int klbui_month_days(int year, int month)
{
    switch (month)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        {
            return 31;
        }
        break;
    case 4:
    case 6:
    case 9:
    case 11:
        {
            return 30;
        }
        break;
    case 2:
        {
            if ((0 == year % 4 && 0 != year % 100) || (0 == year % 400))
            {
                return 29;
            }
            else
            {
                return 28;
            }
        }
        break;
    default:
        break;
    }

    assert(false);
    return 30;
}

int klbui_weekday(int year, int month, int day)
{
    int w = 0;
    int day_sum = 0;

    for (int i = 1; i < month; i++)
    {
        day_sum += klbui_month_days(year, i);
    }

    day_sum += day;
    w = (year - 1) + ((year - 1) / 4) - ((year - 1) / 100) + ((year - 1) / 400) + day_sum;
    w = w % 7;

    return w;
}

void klbui_prev_month(int year, int month, int* p_front_year, int* p_front_month)
{
    if (1 == month)
    {
        *p_front_month = 12;
        *p_front_year = year - 1;
    }
    else
    {
        *p_front_month = month - 1;
        *p_front_year = year;
    }
}

void klbui_next_month(int year, int month, int* p_front_year, int* p_front_month)
{
    if (12 == month)
    {
        *p_front_month = 1;
        *p_front_year = year + 1;
    }
    else
    {
        *p_front_month = month + 1;
        *p_front_year = year;
    }
}

//////////////////////////////////////////////////////////////////////////
//

