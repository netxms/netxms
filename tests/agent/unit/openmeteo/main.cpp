/*
** NetXMS Open-Meteo subagent unit tests
** Copyright (C) 2026 Raden Solutions
*/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <testtools.h>
#include <netxms-version.h>
#include <math.h>
#include "openmeteo.h"

NETXMS_EXECUTABLE_HEADER(test-unit-openmeteo)

/**
 * /v1/forecast response with current conditions and a two-hour hourly curve.
 * Times are unix timestamps (timeformat=unixtime).
 */
static const char *s_docForecast =
   "{"
   "  \"latitude\": 50.11, \"longitude\": 8.68,"
   "  \"current\": {"
   "    \"time\": 1577836800,"
   "    \"temperature_2m\": 20.5,"
   "    \"relative_humidity_2m\": 60,"
   "    \"cloud_cover\": 30,"
   "    \"wind_speed_10m\": 12.3,"
   "    \"shortwave_radiation\": 500.0,"
   "    \"direct_radiation\": 300.0"
   "  },"
   "  \"hourly\": {"
   "    \"time\": [1577836800, 1577840400],"
   "    \"temperature_2m\": [20.5, 21.0],"
   "    \"relative_humidity_2m\": [60, 58],"
   "    \"cloud_cover\": [30, 25],"
   "    \"wind_speed_10m\": [12.3, 11.8],"
   "    \"shortwave_radiation\": [500.0, 520.0],"
   "    \"direct_radiation\": [300.0, 310.0]"
   "  }"
   "}";

/**
 * Forecast response missing the direct_radiation variable entirely - the field
 * must come back as NaN, not zero.
 */
static const char *s_docPartial =
   "{"
   "  \"current\": { \"time\": 1577836800, \"temperature_2m\": 15.0 },"
   "  \"hourly\": {"
   "    \"time\": [1577836800],"
   "    \"temperature_2m\": [15.0]"
   "  }"
   "}";

/**
 * /v1/ensemble response: control run + two members for solar and temperature.
 */
static const char *s_docEnsemble =
   "{"
   "  \"hourly\": {"
   "    \"time\": [1577836800, 1577840400],"
   "    \"shortwave_radiation\": [500.0, 400.0],"
   "    \"shortwave_radiation_member01\": [520.0, 380.0],"
   "    \"shortwave_radiation_member02\": [480.0, 420.0],"
   "    \"temperature_2m\": [20.0, 18.0],"
   "    \"temperature_2m_member01\": [22.0, 19.0],"
   "    \"temperature_2m_member02\": [18.0, 17.0]"
   "  }"
   "}";

/**
 * Open-Meteo error document.
 */
static const char *s_docError =
   "{ \"error\": true, \"reason\": \"Latitude must be in range of -90 to 90\" }";

#define FLOAT_EQ(a, b)  (fabs((a) - (b)) < 0.01)

/**
 * Test "lat,lon" detection and validation.
 */
static void TestLatLonParsing()
{
   double lat, lon;

   StartTest(_T("Coordinate pair parsing"));
   AssertTrue(ParseLatLon(_T("50.1109,8.6821"), &lat, &lon));
   AssertTrue(FLOAT_EQ(lat, 50.1109) && FLOAT_EQ(lon, 8.6821));
   AssertTrue(ParseLatLon(_T("60.1699, 24.9384"), &lat, &lon));
   AssertTrue(FLOAT_EQ(lat, 60.1699) && FLOAT_EQ(lon, 24.9384));
   AssertTrue(ParseLatLon(_T("-33.87, 151.21"), &lat, &lon));
   AssertTrue(FLOAT_EQ(lat, -33.87) && FLOAT_EQ(lon, 151.21));
   EndTest();

   StartTest(_T("Non-coordinate instances rejected"));
   AssertFalse(ParseLatLon(_T("datacenter-fra"), &lat, &lon));
   AssertFalse(ParseLatLon(_T("50.1109"), &lat, &lon));            // missing longitude
   AssertFalse(ParseLatLon(_T("50.1,8.6,extra"), &lat, &lon));      // trailing garbage
   AssertFalse(ParseLatLon(_T("91.0,8.0"), &lat, &lon));            // latitude out of range
   AssertFalse(ParseLatLon(_T("50.0,181.0"), &lat, &lon));          // longitude out of range
   AssertFalse(ParseLatLon(_T(""), &lat, &lon));
   EndTest();
}

/**
 * Test canonical key formatting (named vs raw pair convergence).
 */
static void TestLocationKey()
{
   StartTest(_T("Canonical location key"));
   char k1[MAX_LOC_KEY], k2[MAX_LOC_KEY];
   FormatLocationKey(50.1109, 8.6821, k1);
   FormatLocationKey(50.11091, 8.68209, k2);   // within 4-decimal rounding
   AssertTrue(!strcmp(k1, k2));
   char k3[MAX_LOC_KEY];
   FormatLocationKey(60.1699, 24.9384, k3);
   AssertTrue(strcmp(k1, k3) != 0);
   EndTest();
}

/**
 * Test forecast response parsing.
 */
static void TestForecastParsing()
{
   StartTest(_T("Forecast response parsing"));
   WeatherSnapshot *current = nullptr;
   ForecastCurve *forecast = nullptr;
   AssertTrue(ParseForecastResponse(s_docForecast, strlen(s_docForecast), &current, &forecast));
   AssertNotNull(current);
   AssertNotNull(forecast);

   AssertTrue(current->observationTime == 1577836800);
   AssertTrue(FLOAT_EQ(current->temperature, 20.5));
   AssertTrue(FLOAT_EQ(current->cloudCover, 30.0));
   AssertTrue(FLOAT_EQ(current->shortwaveRadiation, 500.0));
   AssertTrue(FLOAT_EQ(current->directRadiation, 300.0));
   AssertTrue(FLOAT_EQ(current->windSpeed, 12.3));
   AssertTrue(FLOAT_EQ(current->relativeHumidity, 60.0));

   AssertTrue(forecast->points.size() == 2);
   ForecastPoint *p0 = forecast->points.get(0);
   AssertTrue(p0->targetTime == 1577836800);
   AssertTrue(FLOAT_EQ(p0->temperature, 20.5));
   AssertTrue(FLOAT_EQ(p0->shortwaveRadiation, 500.0));
   ForecastPoint *p1 = forecast->points.get(1);
   AssertTrue(p1->targetTime == 1577840400);
   AssertTrue(FLOAT_EQ(p1->temperature, 21.0));
   AssertTrue(FLOAT_EQ(p1->shortwaveRadiation, 520.0));

   MemFree(current);
   delete forecast;
   EndTest();
}

/**
 * Test that missing variables become NaN rather than zero.
 */
static void TestPartialResponse()
{
   StartTest(_T("Missing variables reported as NaN"));
   WeatherSnapshot *current = nullptr;
   ForecastCurve *forecast = nullptr;
   AssertTrue(ParseForecastResponse(s_docPartial, strlen(s_docPartial), &current, &forecast));
   AssertNotNull(current);
   AssertTrue(FLOAT_EQ(current->temperature, 15.0));
   AssertTrue(isnan(current->directRadiation));
   AssertTrue(isnan(current->cloudCover));
   AssertNotNull(forecast);
   AssertTrue(isnan(forecast->points.get(0)->directRadiation));
   MemFree(current);
   delete forecast;
   EndTest();
}

/**
 * Test ensemble spread computation.
 */
static void TestEnsembleParsing()
{
   StartTest(_T("Ensemble spread computation"));
   EnsembleCurve *curve = ParseEnsembleResponse(s_docEnsemble, strlen(s_docEnsemble));
   AssertNotNull(curve);
   AssertTrue(curve->points.size() == 2);

   // Hour 0: solar {500, 520, 480} -> min 480, mean 500, max 520
   EnsemblePoint *p0 = curve->points.get(0);
   AssertTrue(p0->targetTime == 1577836800);
   AssertTrue(FLOAT_EQ(p0->solarMin, 480.0));
   AssertTrue(FLOAT_EQ(p0->solarMean, 500.0));
   AssertTrue(FLOAT_EQ(p0->solarMax, 520.0));
   // Hour 0: temperature {20, 22, 18} -> min 18, mean 20, max 22
   AssertTrue(FLOAT_EQ(p0->tempMin, 18.0));
   AssertTrue(FLOAT_EQ(p0->tempMean, 20.0));
   AssertTrue(FLOAT_EQ(p0->tempMax, 22.0));

   // Hour 1: solar {400, 380, 420} -> min 380, mean 400, max 420
   EnsemblePoint *p1 = curve->points.get(1);
   AssertTrue(FLOAT_EQ(p1->solarMin, 380.0));
   AssertTrue(FLOAT_EQ(p1->solarMean, 400.0));
   AssertTrue(FLOAT_EQ(p1->solarMax, 420.0));

   delete curve;
   EndTest();
}

/**
 * Test error document handling.
 */
static void TestErrorDocument()
{
   StartTest(_T("Error document handling"));
   WeatherSnapshot *current = nullptr;
   ForecastCurve *forecast = nullptr;
   AssertFalse(ParseForecastResponse(s_docError, strlen(s_docError), &current, &forecast));
   AssertNull(current);
   AssertNull(forecast);
   AssertNull(ParseEnsembleResponse(s_docError, strlen(s_docError)));
   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);
   if ((argc > 1) && !strcmp(argv[1], "-debug"))
      nxlog_set_debug_level(9);

   TestLatLonParsing();
   TestLocationKey();
   TestForecastParsing();
   TestPartialResponse();
   TestEnsembleParsing();
   TestErrorDocument();

   return 0;
}
