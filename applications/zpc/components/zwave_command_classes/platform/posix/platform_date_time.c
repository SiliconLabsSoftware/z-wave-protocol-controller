#include "platform_date_time.h"
#include <time.h>
#include <sys/time.h>

// Static variable to store time offset
static time_t time_offset = 0;

date_time_t platform_get_date_time()
{
  date_time_t return_time = {0};
  time_t current_time;
  current_time = time(NULL);

  current_time += time_offset;
  struct tm *local_time = localtime(&current_time);

  if (local_time == NULL) {
    return return_time;
  }

  // Populate return_time with the adjusted values
  return_time.sec  = local_time->tm_sec;
  return_time.min  = local_time->tm_min;
  return_time.hour = local_time->tm_hour;
  return_time.day  = local_time->tm_mday;
  return_time.mon  = local_time->tm_mon;
  return_time.year = local_time->tm_year;

  return return_time;
}

sl_status_t platform_set_date_time(const date_time_t *new_time)
{
  struct tm tm = {0};

  // Populate the tm structure with the values from new_time
  tm.tm_year = new_time->year;  // tm_year is years since 1900
  tm.tm_mon  = new_time->mon;   // tm_mon is 0-11
  tm.tm_mday = new_time->day;
  tm.tm_hour = new_time->hour;
  tm.tm_min  = new_time->min;
  tm.tm_sec  = new_time->sec;

  // Convert tm structure to time_t
  time_t time_in_secs = mktime(&tm);
  if (time_in_secs == (time_t)(-1)) {
    return SL_STATUS_FAIL;
  }

  // Get the current system time
  time_t real_time_in_secs = time(NULL);

  // Calculate the time offset
  time_offset = time_in_secs - real_time_in_secs;

  return SL_STATUS_OK;  // Return success status
}
