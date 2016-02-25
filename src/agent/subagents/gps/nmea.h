/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: config.h 17 2008-03-11 11:56:11Z xtimor $
 *
 */

#ifndef __NMEA_CONFIG_H__
#define __NMEA_CONFIG_H__

#define NMEA_VERSION        ("0.5.3")
#define NMEA_VERSION_MAJOR  (0)
#define NMEA_VERSION_MINOR  (5)
#define NMEA_VERSION_PATCH  (3)

#define NMEA_CONVSTR_BUF    (256)
#define NMEA_TIMEPARSE_BUF  (256)

#if defined(WINCE) || defined(UNDER_CE)
#   define  NMEA_CE
#endif

#if defined(WIN32) || defined(NMEA_CE)
#   define  NMEA_WIN
#else
#   define  NMEA_UNI
#endif

#if defined(NMEA_WIN) && (_MSC_VER >= 1400)
# pragma warning(disable: 4996) /* declared deprecated */
#endif

#if defined(_MSC_VER)
# define NMEA_POSIX(x)  _##x
# define NMEA_INLINE    __inline
#else
# define NMEA_POSIX(x)  x
# define NMEA_INLINE    inline
#endif

#if !defined(NDEBUG) && !defined(NMEA_CE)
#   include <assert.h>
#   define NMEA_ASSERT(x)   assert(x)
#else
#   define NMEA_ASSERT(x)
#endif

#endif /* __NMEA_CONFIG_H__ */
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: units.h 4 2007-08-27 13:11:03Z xtimor $
 *
 */

#ifndef __NMEA_UNITS_H__
#define __NMEA_UNITS_H__

/*
 * Distance units
 */

#define NMEA_TUD_YARDS      (1.0936)        /**< Yeards, meter * NMEA_TUD_YARDS = yard */
#define NMEA_TUD_KNOTS      (1.852)         /**< Knots, kilometer / NMEA_TUD_KNOTS = knot */
#define NMEA_TUD_MILES      (1.609)         /**< Miles, kilometer / NMEA_TUD_MILES = mile */

/*
 * Speed units
 */

#define NMEA_TUS_MS         (3.6)           /**< Meters per seconds, (k/h) / NMEA_TUS_MS= (m/s) */

#endif /* __NMEA_UNITS_H__ */
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: time.h 4 2007-08-27 13:11:03Z xtimor $
 *
 */

/*! \file */

#ifndef __NMEA_TIME_H__
#define __NMEA_TIME_H__

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * Date and time data
 * @see nmea_time_now
 */
typedef struct _nmeaTIME
{
    int     year;       /**< Years since 1900 */
    int     mon;        /**< Months since January - [0,11] */
    int     day;        /**< Day of the month - [1,31] */
    int     hour;       /**< Hours since midnight - [0,23] */
    int     min;        /**< Minutes after the hour - [0,59] */
    int     sec;        /**< Seconds after the minute - [0,59] */
    int     hsec;       /**< Hundredth part of second - [0,99] */

} nmeaTIME;

/**
 * \brief Get time now to nmeaTIME structure
 */
void nmea_time_now(nmeaTIME *t);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_TIME_H__ */
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: info.h 10 2007-11-15 14:50:15Z xtimor $
 *
 */

/*! \file */

#ifndef __NMEA_INFO_H__
#define __NMEA_INFO_H__

#define NMEA_SIG_BAD        (0)
#define NMEA_SIG_LOW        (1)
#define NMEA_SIG_MID        (2)
#define NMEA_SIG_HIGH       (3)

#define NMEA_FIX_BAD        (1)
#define NMEA_FIX_2D         (2)
#define NMEA_FIX_3D         (3)

#define NMEA_MAXSAT         (12)
#define NMEA_SATINPACK      (4)
#define NMEA_NSATPACKS      (NMEA_MAXSAT / NMEA_SATINPACK)

#define NMEA_DEF_LAT        (5001.2621)
#define NMEA_DEF_LON        (3613.0595)

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * Position data in fractional degrees or radians
 */
typedef struct _nmeaPOS
{
    double lat;         /**< Latitude */
    double lon;         /**< Longitude */

} nmeaPOS;

/**
 * Information about satellite
 * @see nmeaSATINFO
 * @see nmeaGPGSV
 */
typedef struct _nmeaSATELLITE
{
    int     id;         /**< Satellite PRN number */
    int     in_use;     /**< Used in position fix */
    int     elv;        /**< Elevation in degrees, 90 maximum */
    int     azimuth;    /**< Azimuth, degrees from true north, 000 to 359 */
    int     sig;        /**< Signal, 00-99 dB */

} nmeaSATELLITE;

/**
 * Information about all satellites in view
 * @see nmeaINFO
 * @see nmeaGPGSV
 */
typedef struct _nmeaSATINFO
{
    int     inuse;      /**< Number of satellites in use (not those in view) */
    int     inview;     /**< Total number of satellites in view */
    nmeaSATELLITE sat[NMEA_MAXSAT]; /**< Satellites information */

} nmeaSATINFO;

/**
 * Summary GPS information from all parsed packets,
 * used also for generating NMEA stream
 * @see nmea_parse
 * @see nmea_GPGGA2info,  nmea_...2info
 */
typedef struct _nmeaINFO
{
    int     smask;      /**< Mask specifying types of packages from which data have been obtained */

    nmeaTIME utc;       /**< UTC of position */

    int     sig;        /**< GPS quality indicator (0 = Invalid; 1 = Fix; 2 = Differential, 3 = Sensitive) */
    int     fix;        /**< Operating mode, used for navigation (1 = Fix not available; 2 = 2D; 3 = 3D) */

    double  PDOP;       /**< Position Dilution Of Precision */
    double  HDOP;       /**< Horizontal Dilution Of Precision */
    double  VDOP;       /**< Vertical Dilution Of Precision */

    double  lat;        /**< Latitude in NDEG - +/-[degree][min].[sec/60] */
    double  lon;        /**< Longitude in NDEG - +/-[degree][min].[sec/60] */
    double  elv;        /**< Antenna altitude above/below mean sea level (geoid) in meters */
    double  speed;      /**< Speed over the ground in kilometers/hour */
    double  direction;  /**< Track angle in degrees True */
    double  declination; /**< Magnetic variation degrees (Easterly var. subtracts from true course) */

    nmeaSATINFO satinfo; /**< Satellites information */

} nmeaINFO;

void nmea_zero_INFO(nmeaINFO *info);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_INFO_H__ */
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: tok.h 4 2007-08-27 13:11:03Z xtimor $
 *
 */

#ifndef __NMEA_TOK_H__
#define __NMEA_TOK_H__

#ifdef  __cplusplus
extern "C" {
#endif

int     nmea_calc_crc(const char *buff, int buff_sz);
int     nmea_atoi(const char *str, int str_sz, int radix);
double  nmea_atof(const char *str, int str_sz);
int     nmea_printf(char *buff, int buff_sz, const char *format, ...);
int     nmea_scanf(const char *buff, int buff_sz, const char *format, ...);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_TOK_H__ */
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: sentence.h 17 2008-03-11 11:56:11Z xtimor $
 *
 */

/*! \file */

#ifndef __NMEA_SENTENCE_H__
#define __NMEA_SENTENCE_H__

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * NMEA packets type which parsed and generated by library
 */
enum nmeaPACKTYPE
{
    GPNON   = 0x0000,   /**< Unknown packet type. */
    GPGGA   = 0x0001,   /**< GGA - Essential fix data which provide 3D location and accuracy data. */
    GPGSA   = 0x0002,   /**< GSA - GPS receiver operating mode, SVs used for navigation, and DOP values. */
    GPGSV   = 0x0004,   /**< GSV - Number of SVs in view, PRN numbers, elevation, azimuth & SNR values. */
    GPRMC   = 0x0008,   /**< RMC - Recommended Minimum Specific GPS/TRANSIT Data. */
    GPVTG   = 0x0010    /**< VTG - Actual track made good and speed over ground. */
};

/**
 * GGA packet information structure (Global Positioning System Fix Data)
 */
typedef struct _nmeaGPGGA
{
    nmeaTIME utc;       /**< UTC of position (just time) */
	double  lat;        /**< Latitude in NDEG - [degree][min].[sec/60] */
    char    ns;         /**< [N]orth or [S]outh */
	double  lon;        /**< Longitude in NDEG - [degree][min].[sec/60] */
    char    ew;         /**< [E]ast or [W]est */
    int     sig;        /**< GPS quality indicator (0 = Invalid; 1 = Fix; 2 = Differential, 3 = Sensitive) */
	int     satinuse;   /**< Number of satellites in use (not those in view) */
    double  HDOP;       /**< Horizontal dilution of precision */
    double  elv;        /**< Antenna altitude above/below mean sea level (geoid) */
    char    elv_units;  /**< [M]eters (Antenna height unit) */
    double  diff;       /**< Geoidal separation (Diff. between WGS-84 earth ellipsoid and mean sea level. '-' = geoid is below WGS-84 ellipsoid) */
    char    diff_units; /**< [M]eters (Units of geoidal separation) */
    double  dgps_age;   /**< Time in seconds since last DGPS update */
    int     dgps_sid;   /**< DGPS station ID number */

} nmeaGPGGA;

/**
 * GSA packet information structure (Satellite status)
 */
typedef struct _nmeaGPGSA
{
    char    fix_mode;   /**< Mode (M = Manual, forced to operate in 2D or 3D; A = Automatic, 3D/2D) */
    int     fix_type;   /**< Type, used for navigation (1 = Fix not available; 2 = 2D; 3 = 3D) */
    int     sat_prn[NMEA_MAXSAT]; /**< PRNs of satellites used in position fix (null for unused fields) */
    double  PDOP;       /**< Dilution of precision */
    double  HDOP;       /**< Horizontal dilution of precision */
    double  VDOP;       /**< Vertical dilution of precision */

} nmeaGPGSA;

/**
 * GSV packet information structure (Satellites in view)
 */
typedef struct _nmeaGPGSV
{
    int     pack_count; /**< Total number of messages of this type in this cycle */
    int     pack_index; /**< Message number */
    int     sat_count;  /**< Total number of satellites in view */
    nmeaSATELLITE sat_data[NMEA_SATINPACK];

} nmeaGPGSV;

/**
 * RMC packet information structure (Recommended Minimum sentence C)
 */
typedef struct _nmeaGPRMC
{
    nmeaTIME utc;       /**< UTC of position */
    char    status;     /**< Status (A = active or V = void) */
	double  lat;        /**< Latitude in NDEG - [degree][min].[sec/60] */
    char    ns;         /**< [N]orth or [S]outh */
	double  lon;        /**< Longitude in NDEG - [degree][min].[sec/60] */
    char    ew;         /**< [E]ast or [W]est */
    double  speed;      /**< Speed over the ground in knots */
    double  direction;  /**< Track angle in degrees True */
    double  declination; /**< Magnetic variation degrees (Easterly var. subtracts from true course) */
    char    declin_ew;  /**< [E]ast or [W]est */
    char    mode;       /**< Mode indicator of fix type (A = autonomous, D = differential, E = estimated, N = not valid, S = simulator) */

} nmeaGPRMC;

/**
 * VTG packet information structure (Track made good and ground speed)
 */
typedef struct _nmeaGPVTG
{
    double  dir;        /**< True track made good (degrees) */
    char    dir_t;      /**< Fixed text 'T' indicates that track made good is relative to true north */
    double  dec;        /**< Magnetic track made good */
    char    dec_m;      /**< Fixed text 'M' */
    double  spn;        /**< Ground speed, knots */
    char    spn_n;      /**< Fixed text 'N' indicates that speed over ground is in knots */
    double  spk;        /**< Ground speed, kilometers per hour */
    char    spk_k;      /**< Fixed text 'K' indicates that speed over ground is in kilometers/hour */

} nmeaGPVTG;

void nmea_zero_GPGGA(nmeaGPGGA *pack);
void nmea_zero_GPGSA(nmeaGPGSA *pack);
void nmea_zero_GPGSV(nmeaGPGSV *pack);
void nmea_zero_GPRMC(nmeaGPRMC *pack);
void nmea_zero_GPVTG(nmeaGPVTG *pack);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_SENTENCE_H__ */
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: context.h 4 2007-08-27 13:11:03Z xtimor $
 *
 */

#ifndef __NMEA_CONTEXT_H__
#define __NMEA_CONTEXT_H__

#define NMEA_DEF_PARSEBUFF  (1024)
#define NMEA_MIN_PARSEBUFF  (256)

#ifdef  __cplusplus
extern "C" {
#endif

typedef void (*nmeaTraceFunc)(const char *str, int str_size);
typedef void (*nmeaErrorFunc)(const char *str, int str_size);

typedef struct _nmeaPROPERTY
{
    nmeaTraceFunc   trace_func;
    nmeaErrorFunc   error_func;
    int             parse_buff_size;

} nmeaPROPERTY;

nmeaPROPERTY * nmea_property();

void nmea_trace(const char *str, ...);
void nmea_trace_buff(const char *buff, int buff_size);
void nmea_error(const char *str, ...);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_CONTEXT_H__ */
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: parse.h 4 2007-08-27 13:11:03Z xtimor $
 *
 */

#ifndef __NMEA_PARSE_H__
#define __NMEA_PARSE_H__

#ifdef  __cplusplus
extern "C" {
#endif

int nmea_pack_type(const char *buff, int buff_sz);
int nmea_find_tail(const char *buff, int buff_sz, int *res_crc);

int nmea_parse_GPGGA(const char *buff, int buff_sz, nmeaGPGGA *pack);
int nmea_parse_GPGSA(const char *buff, int buff_sz, nmeaGPGSA *pack);
int nmea_parse_GPGSV(const char *buff, int buff_sz, nmeaGPGSV *pack);
int nmea_parse_GPRMC(const char *buff, int buff_sz, nmeaGPRMC *pack);
int nmea_parse_GPVTG(const char *buff, int buff_sz, nmeaGPVTG *pack);

void nmea_GPGGA2info(nmeaGPGGA *pack, nmeaINFO *info);
void nmea_GPGSA2info(nmeaGPGSA *pack, nmeaINFO *info);
void nmea_GPGSV2info(nmeaGPGSV *pack, nmeaINFO *info);
void nmea_GPRMC2info(nmeaGPRMC *pack, nmeaINFO *info);
void nmea_GPVTG2info(nmeaGPVTG *pack, nmeaINFO *info);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_PARSE_H__ */
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: parser.h 4 2007-08-27 13:11:03Z xtimor $
 *
 */

#ifndef __NMEA_PARSER_H__
#define __NMEA_PARSER_H__

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * high level
 */

typedef struct _nmeaPARSER
{
    void *top_node;
    void *end_node;
    unsigned char *buffer;
    int buff_size;
    int buff_use;

} nmeaPARSER;

int     nmea_parser_init(nmeaPARSER *parser);
void    nmea_parser_destroy(nmeaPARSER *parser);

int     nmea_parse(
        nmeaPARSER *parser,
        const char *buff, int buff_sz,
        nmeaINFO *info
        );

/*
 * low level
 */

int     nmea_parser_push(nmeaPARSER *parser, const char *buff, int buff_sz);
int     nmea_parser_top(nmeaPARSER *parser);
int     nmea_parser_pop(nmeaPARSER *parser, void **pack_ptr);
int     nmea_parser_peek(nmeaPARSER *parser, void **pack_ptr);
int     nmea_parser_drop(nmeaPARSER *parser);
int     nmea_parser_buff_clear(nmeaPARSER *parser);
int     nmea_parser_queue_clear(nmeaPARSER *parser);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_PARSER_H__ */
