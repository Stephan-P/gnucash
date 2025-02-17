/********************************************************************
 *            gnc-date.h (to be renamed qofdate.h)
 *
 *  Copyright (C) 1997 Robin D. Clark <rclark@cs.hmc.edu>
 *  Copyright (C) 1998-2000, 2003 Linas Vepstas <linas@linas.org>
 *  Copyright  2005  Neil Williams <linux@codehelp.co.uk>
 *  Copyright (C) 2005 David Hampton <hampton@employees.org>
 *  Copyright 2012 John Ralls <jralls@ceridwen.us>
 ********************************************************************/
/********************************************************************\
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
\********************************************************************/
/** @addtogroup Date
    Utility functions to handle date and time (adjusting, getting
    the current date, printing the date and time, etc.)

    Overall, this file is quite a mess.

    An important note about time-keeping:  The general goal of any
    program that works with numeric time values SHOULD BE to always
    stores and use UNIVERSAL TIME internally.  Universal time is the
    'one true time' that is independent of one's location on planet
    Earth.  It is measured in seconds from midnight January 1, 1970
    in localtime-Greenwich (GMT).  If one wants to display the local
    time, then the display-print routine should make all final
    tweaks to print the local time.   The local time *must not* be
    kept as a numeric value anywhere in the program.   If one wants
    to parse a user's input string as if it were local time, then
    the output of the parse routine MUST BE universal time.
    A sane program must never ever store (to file or db) a time
    that is not Universal Time.  Break these rules, and you will
    rue the day...

    \warning HACK ALERT -- the scan and print routines should probably be moved
    to somewhere else. The engine really isn't involved with things
    like printing formats. This is needed mostly by the GUI and so on.
    If a file-io backend needs date handling, it should do it itself,
    instead of depending on the routines here.

    (to be renamed qofdate.h in libqof2.)

    @author Copyright (C) 1997 Robin D. Clark <rclark@cs.hmc.edu>
    @author Copyright (C) 1998-2001,2003 Linas Vepstas <linas@linas.org>
*/

/** @{
    @file gnc-date.h
    @brief Date and Time handling routines
*/

#ifndef GNC_DATE_H
#define GNC_DATE_H

#include <glib-object.h>

#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Most systems that are currently maintained, including Microsoft Windows,
 * BSD-derived Unixes and Linux, support 64-bit time_t even on 32-bit
 * architectures. See https://en.wikipedia.org/wiki/Year_2038_problem
 *
 * For practical reasons -- as not all have made the transition to 64-bit --
 * we define our own 64-bit time type.
 */
typedef gint64 time64;
/* A bit of a hack to create a type separate from the alias of int64_t so that
 * compile-time dispatch can use the right KVP visitor.
 */
typedef struct
{
    time64 t;
} Time64;


/** @name GValue
  @{
*/
GType time64_get_type( void );
#define GNC_TYPE_TIME64 (time64_get_type ())

/** @} */
/** The default date format for use with strftime. */
extern const char *gnc_default_strftime_date_format;

/** The maximum length of a string created by the date printers */
#define MAX_DATE_LENGTH 34
/** Constants *******************************************************/
/** \brief UTC date format string.

Time zone independent, date and time inclusive, as used in the QSF backend.
The T and Z characters are from xsd:dateTime format in coordinated universal time, UTC.
You can reproduce the string from the GNU/Linux command line using the date utility:
date -u +%Y-%m-%dT%H:M:SZ = 2004-12-12T23:39:11Z The datestring must be time zone independent
and include all specified fields. Remember to use gmtime() NOT localtime()!
*/

#define QOF_UTC_DATE_FORMAT     "%Y-%m-%dT%H:%M:%SZ"

/** Enum for determining a date format */
typedef enum
{
    QOF_DATE_FORMAT_US,       /**< United states: mm/dd/yyyy */
    QOF_DATE_FORMAT_UK,       /**< Britain: dd/mm/yyyy */
    QOF_DATE_FORMAT_CE,       /**< Continental Europe: dd.mm.yyyy */
    QOF_DATE_FORMAT_ISO,      /**< ISO: yyyy-mm-dd */
    QOF_DATE_FORMAT_LOCALE,   /**< Take from locale information */
    QOF_DATE_FORMAT_UTC,      /**< UTC: 2004-12-12T23:39:11Z */
    QOF_DATE_FORMAT_CUSTOM,   /**< Used by the check printing code */
    QOF_DATE_FORMAT_UNSET     /**< No Fancy Date Format, use Global */
} QofDateFormat;

#define DATE_FORMAT_FIRST QOF_DATE_FORMAT_US
#define DATE_FORMAT_LAST  QOF_DATE_FORMAT_UTC

/** Enum for date completion modes (for dates entered without year) */
typedef enum
{
    QOF_DATE_COMPLETION_THISYEAR, /**< use current year */
    QOF_DATE_COMPLETION_SLIDING,  /**< use sliding 12-month window */
} QofDateCompletion;

/** \deprecated qof_date_format_get_format has been replaced
by qof_date_text_format_get_string */
#define qof_date_format_get_format qof_date_text_format_get_string

/**
 * This is how to format the month, as a number, an abbreviated string,
 * or the full name.
 */
typedef enum
{
    GNCDATE_MONTH_NUMBER,
    GNCDATE_MONTH_ABBREV,
    GNCDATE_MONTH_NAME
} GNCDateMonthFormat;

/* Replacements for POSIX functions which use time_t. Time_t is still
 * 32 bits in Microsoft Windows, Apple OSX, and some BSD versions even
 * when the rest of the system is 64-bits, as well as all 32-bit
 * versions of Unix. 32-bit time_t overflows at 03:14:07 UTC on
 * Tuesday, 19 January 2038 and so cannot represent dates after that.
 *
 * These functions use boost::date_time internally.
 */
/** \brief fill out a time struct from a 64-bit time value.
 *  \param secs: Seconds since 00:00:00 UTC 01 January 1970 (negative values
 * are seconds before that moment).
 *  \return A struct tm*, allocated on the heap. Must be freed with gnc_tm_free().
 *  The time is adjusted for the current local time zone.
 */
struct tm* gnc_localtime (const time64 *secs);

/** \brief fill out a time struct from a 64-bit time value adjusted for the current time zone.
 *  \param secs: Seconds since 00:00:00 UTC 01 January 1970 (negative values
 * are seconds before that moment)
 *  \param time: A struct tm* for the function to fill.
 *  The time is adjusted for the current local time zone.
 */
struct tm* gnc_localtime_r (const time64 *secs, struct tm* time);

/** \brief fill out a time struct from a 64-bit time value
 *  \param secs: Seconds since 00:00:00 UTC 01 January 1970 (negative values
 * are seconds before that moment)
 *  \return A struct tm*, allocated on the heap. Must be freed with gnc_tm_free()
 *  The time is UTC.
 */
struct tm* gnc_gmtime (const time64 *secs);

/** \brief returns an integer corresponding to locale start of week
 *  \return An integer 1=Sunday, 2=Monday etc. If error, return 0.
 */
gint gnc_start_of_week (void);

/** \brief calculate seconds from the epoch given a time struct
 *  \param time: A struct tm* containing the date-time information.
 *  The time is understood to be in the current local time zone.
 *  \return Seconds since 00:00:00 UTC 01 January 1970 (negative values
 * are seconds before that moment).
 */
time64 gnc_mktime (struct tm* time);

/** \brief calculate seconds from the epoch given a time struct
 *  \param time: A struct tm* containing the date-time information
 *  The time is understood to be utc.
 *  \return Seconds since 00:00:00 UTC 01 January 1970 (negative values
 * are seconds before that moment).
 */
time64 gnc_timegm (struct tm* time);

/** \brief Return a string representation of a date from a 64-bit time value
 *  \param secs: Seconds since 00:00:00 UTC 01 January 1970 (negative values
 * are seconds before that moment)
 * \return A string, which must be freed with g_free(), representing the date
 * in the following format:
 *       Thu Nov 24 18:22:48 1986\n\0
 * This is equivalent to the strftime format %a %b %H:%M:%S %Y.
 */
gchar* gnc_ctime (const time64 *secs);

/** \brief get the current time
 *  \param A time64* which, if not NULL, will be filled in with the same
 * value as is returned.
 * \return Seconds since 00:00:00 UTC 01 January 1970 (negative values
 * are seconds before that moment)
 */
time64 gnc_time (time64 *tbuf);

/** \brief Find the difference in seconds between two time values
 *  \param secs1: The first time value, in Seconds since
 * 00:00:00 UTC 01 January 1970 (negative values are seconds before that moment)
 *  \param secs2: The second time value, in Seconds since
 * 00:00:00 UTC 01 January 1970 (negative values are seconds before that moment)
 *  \return The difference in seconds between secs1 and secs2. If secs2 is
 * later than secs1 the value will be negative.
 */
gdouble gnc_difftime (const time64 secs1, const time64 secs2);

/** \brief free a struct tm* created with gnc_localtime() or gnc_gmtime()
 * \param time: The struct tm* to be freed.
 */
void gnc_tm_free (struct tm* time);

/** \name String / DateFormat conversion. */
//@{

/** \brief The string->value versions return FALSE on success and TRUE on failure */
const gchar* gnc_date_dateformat_to_string(QofDateFormat format);

/** \brief Converts the date format to a printable string.

Note the reversed return values!
@return FALSE on success, TRUE on failure.
*/
gboolean gnc_date_string_to_dateformat(const gchar* format_string,
                                       QofDateFormat *format);

const gchar* gnc_date_monthformat_to_string(GNCDateMonthFormat format);

/** \brief Converts the month format to a printable string.

Note the reversed return values!
@return FALSE on success, TRUE on failure.
*/
gboolean gnc_date_string_to_monthformat(const gchar *format_string,
                                        GNCDateMonthFormat *format);

/** \brief print a time64 as a date string per format
 * \param time The time64 to print
 * \param format A date format conforming to the strftime format rules.
 * \return a raw heap-allocated char* which must be freed.
 */
char* gnc_print_time64(time64 time, const char* format);

// @}

/** @name GDate time64 setters
 *    @{ */
/** Returns a newly allocated date of the current clock time, taken from
 * time(2). The caller must g_date_free() the object afterwards. */
GDate* gnc_g_date_new_today (void);

/** Set a GDate to the current day
 * @param gd The date to act on
 */
void gnc_gdate_set_today (GDate* gd);

/** Set a GDate to a time64
 * @param gd the date to act on
 * @param time the time to set it to.
 */
void gnc_gdate_set_time64 (GDate* gd, time64 time);

/** @} */
/** convert a time64 on a certain day (localtime) to
 * the time64 representing midday on that day. Watch out - this is *not* the
 * first second of the day, which is returned by various other functions
 * returning a time64. */
time64 time64CanonicalDayTime(time64 t);

/** Turns a GDate into a time64, returning the first second of the day */
time64 gdate_to_time64 (GDate d);

/** Convert a day, month, and year to a time64, returning the first second of the day */
time64 gnc_dmy2time64 (gint day, gint month, gint year);

/** Converts a day, month, and year to a time64 representing 11:00:00 UTC
 *  11:00:00 UTC falls on the same time in almost all timezones, the exceptions
 *  being the +13, +14, and -12 timezones used by countries along the
 *  International Date Line. Since users in those timezones would see dates
 *  immediately change by one day, the function checks the current timezone for
 *  those changes and adjusts the UTC time so that the date will be consistent.
 */
time64 gnc_dmy2time64_neutral (gint day, gint month, gint year);

/** Same as gnc_dmy2time64, but last second of the day */
time64 gnc_dmy2time64_end (gint day, gint month, gint year);

/** The gnc_iso8601_to_time64_gmt() routine converts an ISO-8601 style
 *    date/time string to time64.  Please note that ISO-8601 strings
 *    are a representation of Universal Time (UTC), and as such, they
 *    'store' UTC.  To make them human readable, they show time zone
 *    information along with a local-time string.  But fundamentally,
 *    they *are* UTC.  Thus, this routine takes a UTC input, and
 *    returns a UTC output.
 *
 *    For example: 1998-07-17 11:00:00.68-0500
 *    is 680 milliseconds after 11 o'clock, central daylight time
 *    It is also 680 milliseconds after 16:00:00 hours UTC.
 *    \return The universal time.
 *
 * XXX Caution: this routine does not handle strings that specify
 * times before January 1 1970.
 */
time64 gnc_iso8601_to_time64_gmt(const gchar *);

/** The gnc_time64_to_iso8601_buff() routine takes the input
 *    UTC time64 value and prints it as an ISO-8601 style string.
 *    The buffer must be long enough to contain the NULL-terminated
 *    string (32 characters + NUL).  This routine returns a pointer
 *    to the null terminator (and can thus be used in the 'stpcpy'
 *    metaphor of string concatenation).
 *
 *    Please note that ISO-8601 strings are a representation of
 *    Universal Time (UTC), and as such, they 'store' UTC.  To make them
 *    human readable, they show time zone information along with a
 *    local-time string.  But fundamentally, they *are* UTC.  Thus,
 *    this routine takes a UTC input, and returns a UTC output.
 *
 *    The string generated by this routine uses the local time zone
 *    on the machine on which it is executing to create the time string.
 */
gchar * gnc_time64_to_iso8601_buff (time64, char * buff);
// @}

/* ======================================================== */

/** \name QofDateFormat functions */
// @{
/** The qof_date_format_get routine returns the date format that
 *  the date printing will use when printing a date, and the scanning
 *  routines will assume when parsing a date.
 * @returns: the one of the enumerated date formats.
 */
QofDateFormat qof_date_format_get(void);

/**
 * The qof_date_format_set() routine sets date format to one of
 *    US, UK, CE, OR ISO.  Checks to make sure it's a legal value.
 *    Args: QofDateFormat: enumeration indicating preferred format
 */
void qof_date_format_set(QofDateFormat df);

/** This function returns a strftime formatting string for printing an
 *  all numeric date (e.g. 2005-09-14).  The string returned is based
 *  upon the location specified.
 *
 *  @param df The date style (us, uk, iso, etc) that should be provided.
 *
 *  @return A formatting string that will print a date in the
 *  requested style  */
const gchar *qof_date_format_get_string(QofDateFormat df);

/** This function returns a strftime formatting string for printing a
 *  date using words and numbers (e.g. 2005-September-14).  The string
 *  returned is based upon the location specified.
 *
 *  @param df The date style (us, uk, iso, etc) that should be provided.
 *
 *  @return A formatting string that will print a date in the
 *  requested style  */
const gchar *qof_date_text_format_get_string(QofDateFormat df);
// @}

/* ======================================================== */

/**
 * The qof_date_completion_set() routing sets the date completion method to
 *    one of QOF_DATE_COMPLETION_THISYEAR (for completing the year to
 *    the current calendar year) or QOF_DATE_COMPLETION_SLIDING (for
 *    using a sliding 12-month window). The sliding window starts
 *    'backmonth' months before the current month (0-11) */
void qof_date_completion_set(QofDateCompletion dc, int backmonths);

/** dateSeparator
 *    Return the field separator for the current date format
 *
 * Args:   none
 *
 * Return: date character
 *
 * Globals: global dateFormat value
 */
gchar dateSeparator(void);

/* ======================================================== */

/** \name Date Printing/Scanning functions
 */
// @{
/**
 * \warning HACK ALERT -- the scan and print routines should probably
 * be moved to somewhere else. The engine really isn't involved with
 * things like printing formats. This is needed mostly by the GUI and
 * so on.  If a file-io thing needs date handling, it should do it
 * itself, instead of depending on the routines here.
 */

/* qof_format_time takes a format specification in UTF-8 and a broken-down time,
 *  tries to call strftime with a sufficiently large buffer and, if successful,
 *  return a newly allocated string in UTF-8 for the printing result.
 *
 *  @param format A format specification in UTF-8.
 *
 *  @param tm A broken-down time.
 *
 *  @return A newly allocated string on success, or NULL otherwise.
 */
/* gchar *qof_format_time(const gchar *format, const struct tm *tm); */

/** qof_strftime calls qof_format_time to print a given time and afterwards tries
 *  to put the result into a buffer of fixed size.
 *
 *  @param buf A buffer.
 *
 *  @param max The size of buf in bytes.
 *
 *  @param format A format specification in UTF-8.
 *
 *  @param tm A broken-down time.
 *
 *  @return The number of characters written, not include the null byte, if the
 *  complete string, including the null byte, fits into the buffer.  Otherwise 0.
 */
gsize qof_strftime(gchar *buf, gsize max, const gchar *format,
                   const struct tm *tm);

/** qof_print_date_dmy_buff
 *    Convert a date as day / month / year integers into a localized string
 *    representation
 *
 * Args:   buff - pointer to previously allocated character array; its size
 *                must be at lease MAX_DATE_LENTH bytes.
 *         len - length of the buffer, in bytes.
 *         day - day of the month as 1 ... 31
 *         month - month of the year as 1 ... 12
 *         year - year (4-digit)
 *
 * Returns: number of characters printed
 *
 * Globals: global dateFormat value
 **/
size_t qof_print_date_dmy_buff (gchar * buff, size_t buflen, int day, int month, int year);

/** Convenience: calls through to qof_print_date_dmy_buff(). **/
size_t qof_print_date_buff (char * buff, size_t buflen, time64 secs);

/** Convenience; calls through to qof_print_date_dmy_buff(). **/
size_t qof_print_gdate(char *buf, size_t bufflen, const GDate *gd);

/** Convenience; calls through to qof_print_date_dmy_buff().
 *  Return: string, which should be freed when no longer needed.
 * **/
char * qof_print_date (time64 secs);


/* ------------------------------------------------------------------ */
/* time printing utilities */

/**
 *    Returns the number of bytes printed.
 */

size_t qof_print_date_time_buff (char * buff, size_t len, time64 secs);

/** qof_scan_date
 *    Convert a string into  day / month / year integers according to
 *    the current dateFormat value.
 *
 * Args:   buff - pointer to date string
 *         day -  will store day of the month as 1 ... 31
 *         month - will store month of the year as 1 ... 12
 *         year - will store the year (4-digit)
 *
 * Return: TRUE if the string seemed to be a valid date; else FALSE.
 *
 * Globals: uses global dateFormat value to assist in parsing.
 */
gboolean qof_scan_date (const char *buff, int *day, int *month, int *year);

// @}

/* ======================================================== */

/** \name Date Start/End Adjustment routines
 * Given a time value, adjust it to be the beginning or end of that day.
 */
// @{

/** The gnc_tm_set_day_start() inline routine will set the appropriate
 *  fields in the struct tm to indicate the first second of that day.
 *  This routine assumes that the contents of the data structure is
 *  already in normalized form. */
static inline
void gnc_tm_set_day_start (struct tm *tm)
{
    /* First second of the day */
    g_return_if_fail (tm != NULL);
    tm->tm_hour = 0;
    tm->tm_min = 0;
    tm->tm_sec = 0;
}

/** The gnc_tm_set_day_neutral() inline routine will set the appropriate
 *  fields in the struct tm to indicate 10:59am of that day.
 */
void gnc_tm_set_day_neutral (struct tm *tm);

/** The gnc_tm_set_day_middle() inline routine will set the appropriate
 *  fields in the struct tm to indicate noon of that day.  This
 *  routine assumes that the contents of the data structure is already
 *  in normalized form.*/
static inline
void gnc_tm_set_day_middle (struct tm *tm)
{
    /* First second of the day */
    g_return_if_fail (tm != NULL);
    tm->tm_hour = 12;
    tm->tm_min = 0;
    tm->tm_sec = 0;
}

/** The gnc_tm_set_day_end() inline routine will set the appropriate
 *  fields in the struct tm to indicate the last second of that day.
 *  This routine assumes that the contents of the data structure is
 *  already in normalized form.*/
static inline
void gnc_tm_set_day_end (struct tm *tm)
{
    /* Last second of the day */
    g_return_if_fail (tm != NULL);
    tm->tm_hour = 23;
    tm->tm_min = 59;
    tm->tm_sec = 59;
}

/** The gnc_time64_get_day_start() routine will take the given time in
 *  seconds and adjust it to the first second of that day. */
time64 gnc_time64_get_day_start(time64 time_val);

/** The gnc_time64_get_day_neutral() routine will take the given time in
 *  seconds and adjust it to 10:59:00Z of that day. */
time64 gnc_time64_get_day_neutral(time64 time_val);

/** The gnc_time64_get_day_end() routine will take the given time in
 *  seconds and adjust it to the last second of that day. */
time64 gnc_time64_get_day_end(time64 time_val);

/** Get the numerical last date of the month. (28, 29, 30, 31) */
int gnc_date_get_last_mday (int month, int year);

// @}

/* ======================================================== */

/** \name Today's Date */
// @{
/** The gnc_tm_get_today_start() routine takes a pointer to a struct
 *  tm and fills it in with the first second of the today. */
void   gnc_tm_get_today_start(struct tm *tm);

/** The gnc_tm_get_today_start() routine takes a pointer to a struct
 *  tm and fills it in with the timezone neutral time (10:59:00Z). */
void   gnc_tm_get_today_neutral(struct tm *tm);

/** The gnc_tm_get_today_end() routine takes a pointer to a struct
 *  tm and fills it in with the last second of the today. */
void   gnc_tm_get_today_end(struct tm *tm);

/** The gnc_time64_get_today_start() routine returns a time64 value
 *  corresponding to the first second of today. */
time64 gnc_time64_get_today_start(void);

/** The gnc_time64_get_today_end() routine returns a time64 value
 *  corresponding to the last second of today. */
time64 gnc_time64_get_today_end(void);

/** \brief Make a timestamp in YYYYMMDDHHMMSS format.
 *  @return A pointer to the generated string.
 *  @note The caller owns this buffer and must g_free it when done. */
char * gnc_date_timestamp (void);

#define MIN_BUF_LEN 10
/**
 * Localized DOW abbreviation.
 * @param buf_len at least MIN_BUF_LEN
 * @param dow struct tm semantics: 0=sunday .. 6=saturday
 **/
void gnc_dow_abbrev(gchar *buf, int buf_len, int dow);

//@}

/* ======================================================== */

/** \name GDate hash table support */
// @{

/** Compares two GDate*'s for equality; useful for using GDate*'s as
 *  GHashTable keys. */
gint gnc_gdate_equal(gconstpointer gda, gconstpointer gdb);


/** Provides a "hash" of a GDate* value; useful for using GDate*'s as
 *  GHashTable keys. */
guint gnc_gdate_hash( gconstpointer gd );

//@}

/* ======================================================== */

/** \name GDate to time64 conversions */
// @{
/** Returns the GDate in which the time64 occurs.
 * @param t The time64
 * @returns A GDate for the day in which the time64 occurs.
 */
GDate time64_to_gdate (time64 t);

/** The gnc_time64_get_day_start() routine will take the given time in
 *  GLib GDate format and adjust it to the first second of that day.
 */
time64 gnc_time64_get_day_start_gdate (const GDate *date);

/** The gnc_time64_get_day_end() routine will take the given time in
 *  GLib GDate format and adjust it to the last second of that day.
 */
time64 gnc_time64_get_day_end_gdate (const GDate *date);

//@}

/* ======================================================== */

/** \name Date Manipulation */
// @{

/** This function modifies a GDate to set it to the first day of the
 *  month in which it falls.  For example, if this function is called
 *  with a date of 2003-09-24 the date will be modified to 2003-09-01.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_month_start (GDate *date);


/** This function modifies a GDate to set it to the last day of the
 *  month in which it falls.  For example, if this function is called
 *  with a date of 2003-09-24 the date will be modified to 2003-09-30.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_month_end (GDate *date);


/** This function modifies a GDate to set it to the first day of the
 *  month prior to the one in which it falls.  For example, if this
 *  function is called with a date of 2003-09-24 the date will be
 *  modified to 2003-08-01.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_prev_month_start (GDate *date);


/** This function modifies a GDate to set it to the last day of the
 *  month prior to the one in which it falls.  For example, if this
 *  function is called with a date of 2003-09-24 the date will be
 *  modified to 2003-08-31.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_prev_month_end (GDate *date);


/** This function modifies a GDate to set it to the first day of the
 *  quarter in which it falls.  For example, if this function is called
 *  with a date of 2003-09-24 the date will be modified to 2003-07-01.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_quarter_start (GDate *date);


/** This function modifies a GDate to set it to the last day of the
 *  quarter in which it falls.  For example, if this function is called
 *  with a date of 2003-09-24 the date will be modified to 2003-09-30.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_quarter_end (GDate *date);


/** This function modifies a GDate to set it to the first day of the
 *  quarter prior to the one in which it falls.  For example, if this
 *  function is called with a date of 2003-09-24 the date will be
 *  modified to 2003-04-01.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_prev_quarter_start (GDate *date);


/** This function modifies a GDate to set it to the last day of the
 *  quarter prior to the one in which it falls.  For example, if this
 *  function is called with a date of 2003-09-24 the date will be
 *  modified to 2003-06-30.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_prev_quarter_end (GDate *date);


/** This function modifies a GDate to set it to the first day of the
 *  year in which it falls.  For example, if this function is called
 *  with a date of 2003-09-24 the date will be modified to 2003-01-01.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_year_start (GDate *date);


/** This function modifies a GDate to set it to the last day of the
 *  year in which it falls.  For example, if this function is called
 *  with a date of 2003-09-24 the date will be modified to 2003-12-31.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_year_end (GDate *date);


/** This function modifies a GDate to set it to the first day of the
 *  year prior to the one in which it falls.  For example, if this
 *  function is called with a date of 2003-09-24 the date will be
 *  modified to 2002-01-01.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_prev_year_start (GDate *date);


/** This function modifies a GDate to set it to the last day of the
 *  year prior to the one in which it falls.  For example, if this
 *  function is called with a date of 2003-09-24 the date will be
 *  modified to 2002-12-31.
 *
 *  @param date The GDate to modify. */
void gnc_gdate_set_prev_year_end (GDate *date);


/** This function modifies a GDate to set it to the first day of the
 *  fiscal year in which it falls.  For example, if this function is
 *  called with a date of 2003-09-24 and a fiscal year ending July
 *  31st, the date will be modified to 2003-08-01.
 *
 *  @param date The GDate to modify.
 *
 *  @param year_end A GDate containing the last month and day of the
 *  fiscal year.  The year field of this argument is ignored. */
void gnc_gdate_set_fiscal_year_start (GDate *date, const GDate *year_end);


/** This function modifies a GDate to set it to the last day of the
 *  fiscal year in which it falls.  For example, if this function is
 *  called with a date of 2003-09-24 and a fiscal year ending July
 *  31st, the date will be modified to 2004-07-31.
 *
 *  @param date The GDate to modify.
 *
 *  @param year_end A GDate containing the last month and day of the
 *  fiscal year.  The year field of this argument is ignored. */
void gnc_gdate_set_fiscal_year_end (GDate *date, const GDate *year_end);


/** This function modifies a GDate to set it to the first day of the
 *  fiscal year prior to the one in which it falls.  For example, if
 *  this function is called with a date of 2003-09-24 and a fiscal
 *  year ending July 31st, the date will be modified to 2002-08-01.
 *
 *  @param date The GDate to modify.
 *
 *  @param year_end A GDate containing the last month and day of the
 *  fiscal year.  The year field of this argument is ignored. */
void gnc_gdate_set_prev_fiscal_year_start (GDate *date, const GDate *year_end);


/** This function modifies a GDate to set it to the last day of the
 *  fiscal year prior to the one in which it falls.  For example, if
 *  this function is called with a date of 2003-09-24 and a fiscal
 *  year ending July 31st, the date will be modified to 2003-07-31.
 *
 *  @param date The GDate to modify.
 *
 *  @param year_end A GDate containing the last month and day of the
 *  fiscal year.  The year field of this argument is ignored. */
void gnc_gdate_set_prev_fiscal_year_end (GDate *date, const GDate *year_end);



/** This function takes a GList of char*, and uses locale-sensitive
 *  list formatter.
 *
 *  @param strings The GList* of char*.
 *
 * @returns a newly allocated char*
 */
gchar* gnc_list_formatter (GList* strings);

//@}

//@}
#ifdef __cplusplus
}
#endif

#endif /* GNC_DATE_H */
