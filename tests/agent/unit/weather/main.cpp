/*
** NetXMS weather subagent unit tests
** Copyright (C) 2026 Raden Solutions
*/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <testtools.h>
#include <netxms-version.h>
#include <math.h>
#include "weather.h"

NETXMS_EXECUTABLE_HEADER(test-unit-weather)

/**
 * Open-Meteo /v1/forecast response with current conditions and a two-hour hourly
 * curve. Times are unix timestamps (timeformat=unixtime).
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
   "    \"precipitation\": 0.2,"
   "    \"shortwave_radiation\": 500.0,"
   "    \"direct_radiation\": 300.0"
   "  },"
   "  \"hourly\": {"
   "    \"time\": [1577836800, 1577840400],"
   "    \"temperature_2m\": [20.5, 21.0],"
   "    \"relative_humidity_2m\": [60, 58],"
   "    \"cloud_cover\": [30, 25],"
   "    \"wind_speed_10m\": [12.3, 11.8],"
   "    \"precipitation\": [0.2, 0.0],"
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

/**
 * MET Norway Locationforecast 2.0 (compact) response. The last entry carries no
 * next_1_hours block, as happens beyond the hourly part of the horizon.
 */
static const char *s_docMetNo =
   "{"
   "  \"type\": \"Feature\","
   "  \"properties\": {"
   "    \"meta\": { \"updated_at\": \"2020-01-01T00:00:00Z\" },"
   "    \"timeseries\": ["
   "      {"
   "        \"time\": \"2020-01-01T00:00:00Z\","
   "        \"data\": {"
   "          \"instant\": { \"details\": {"
   "            \"air_temperature\": 16.0,"
   "            \"cloud_area_fraction\": 100.0,"
   "            \"relative_humidity\": 92.2,"
   "            \"wind_speed\": 5.0"
   "          } },"
   "          \"next_1_hours\": { \"details\": { \"precipitation_amount\": 1.1 } }"
   "        }"
   "      },"
   "      {"
   "        \"time\": \"2020-01-01T01:00:00Z\","
   "        \"data\": {"
   "          \"instant\": { \"details\": {"
   "            \"air_temperature\": 17.1,"
   "            \"cloud_area_fraction\": 90.0,"
   "            \"relative_humidity\": 90.9,"
   "            \"wind_speed\": 10.0"
   "          } }"
   "        }"
   "      }"
   "    ]"
   "  }"
   "}";

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
 * Test canonical key formatting (named vs raw pair convergence, provider split).
 */
static void TestLocationKey()
{
   StartTest(_T("Canonical location key"));
   char k1[MAX_LOC_KEY], k2[MAX_LOC_KEY];
   FormatLocationKey("openmeteo", 50.1109, 8.6821, k1);
   FormatLocationKey("openmeteo", 50.11091, 8.68209, k2);   // within 4-decimal rounding
   AssertTrue(!strcmp(k1, k2));
   char k3[MAX_LOC_KEY];
   FormatLocationKey("openmeteo", 60.1699, 24.9384, k3);
   AssertTrue(strcmp(k1, k3) != 0);
   // Same coordinates on different providers must not share a cache entry
   char k4[MAX_LOC_KEY];
   FormatLocationKey("metno", 50.1109, 8.6821, k4);
   AssertTrue(strcmp(k1, k4) != 0);
   EndTest();
}

/**
 * Test ISO 8601 timestamp parsing.
 */
static void TestIsoTimestamp()
{
   StartTest(_T("ISO 8601 timestamp parsing"));
   AssertEquals(static_cast<int64_t>(ParseIsoTimestamp("2020-01-01T00:00:00Z")), INT64_C(1577836800));
   AssertEquals(static_cast<int64_t>(ParseIsoTimestamp("2020-01-01T01:00:00Z")), INT64_C(1577840400));
   AssertEquals(static_cast<int64_t>(ParseIsoTimestamp("not a timestamp")), INT64_C(0));
   AssertEquals(static_cast<int64_t>(ParseIsoTimestamp(nullptr)), INT64_C(0));
   EndTest();
}

/**
 * Test Open-Meteo request URL construction, free and commercial tiers.
 */
static void TestOpenMeteoUrls()
{
   StartTest(_T("Open-Meteo URL construction (free tier)"));
   OpenMeteoProvider freeTier("", "icon_seamless");
   char url[1024];
   freeTier.buildForecastUrl(50.1109, 8.6821, 2, url, sizeof(url));
   AssertNotNull(strstr(url, "https://api.open-meteo.com/v1/forecast?"));
   AssertNotNull(strstr(url, "latitude=50.1109&longitude=8.6821"));
   AssertNull(strstr(url, "apikey="));
   freeTier.buildEnsembleUrl(50.1109, 8.6821, 2, url, sizeof(url));
   AssertNotNull(strstr(url, "https://ensemble-api.open-meteo.com/v1/ensemble?"));
   AssertNotNull(strstr(url, "models=icon_seamless"));
   AssertNull(strstr(url, "apikey="));
   EndTest();

   StartTest(_T("Open-Meteo URL construction (commercial tier)"));
   OpenMeteoProvider commercial("SECRET-KEY", "icon_seamless");
   commercial.buildForecastUrl(50.1109, 8.6821, 2, url, sizeof(url));
   AssertNotNull(strstr(url, "https://customer-api.open-meteo.com/v1/forecast?"));
   AssertNotNull(strstr(url, "&apikey=SECRET-KEY"));
   commercial.buildEnsembleUrl(50.1109, 8.6821, 2, url, sizeof(url));
   AssertNotNull(strstr(url, "https://customer-ensemble-api.open-meteo.com/v1/ensemble?"));
   AssertNotNull(strstr(url, "&apikey=SECRET-KEY"));
   EndTest();
}

/**
 * Test MET Norway request URL construction.
 */
static void TestMetNoUrl()
{
   StartTest(_T("MET Norway URL construction"));
   MetNoProvider provider;
   char url[1024];
   // Coordinates must be truncated to four decimals per MET Norway terms of service
   provider.buildForecastUrl(59.913868, 10.752245, 2, url, sizeof(url));
   AssertEquals(url, "https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=59.9139&lon=10.7522");
   AssertFalse(provider.supportsEnsemble());
   EndTest();
}

/**
 * Test Open-Meteo forecast response parsing.
 */
static void TestForecastParsing()
{
   StartTest(_T("Open-Meteo forecast response parsing"));
   OpenMeteoProvider provider("", "icon_seamless");
   WeatherSnapshot *current = nullptr;
   ForecastCurve *forecast = nullptr;
   AssertTrue(provider.parseForecastResponse(s_docForecast, strlen(s_docForecast), 2, &current, &forecast));
   AssertNotNull(current);
   AssertNotNull(forecast);

   AssertTrue(current->observationTime == 1577836800);
   AssertTrue(FLOAT_EQ(current->temperature, 20.5));
   AssertTrue(FLOAT_EQ(current->cloudCover, 30.0));
   AssertTrue(FLOAT_EQ(current->shortwaveRadiation, 500.0));
   AssertTrue(FLOAT_EQ(current->directRadiation, 300.0));
   AssertTrue(FLOAT_EQ(current->windSpeed, 12.3));
   AssertTrue(FLOAT_EQ(current->relativeHumidity, 60.0));
   AssertTrue(FLOAT_EQ(current->precipitation, 0.2));

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
   OpenMeteoProvider provider("", "icon_seamless");
   WeatherSnapshot *current = nullptr;
   ForecastCurve *forecast = nullptr;
   AssertTrue(provider.parseForecastResponse(s_docPartial, strlen(s_docPartial), 2, &current, &forecast));
   AssertNotNull(current);
   AssertTrue(FLOAT_EQ(current->temperature, 15.0));
   AssertTrue(isnan(current->directRadiation));
   AssertTrue(isnan(current->cloudCover));
   AssertTrue(isnan(current->precipitation));
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
   OpenMeteoProvider provider("", "icon_seamless");
   AssertTrue(provider.supportsEnsemble());
   EnsembleCurve *curve = provider.parseEnsembleResponse(s_docEnsemble, strlen(s_docEnsemble));
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
   OpenMeteoProvider provider("", "icon_seamless");
   WeatherSnapshot *current = nullptr;
   ForecastCurve *forecast = nullptr;
   AssertFalse(provider.parseForecastResponse(s_docError, strlen(s_docError), 2, &current, &forecast));
   AssertNull(current);
   AssertNull(forecast);
   AssertNull(provider.parseEnsembleResponse(s_docError, strlen(s_docError)));
   EndTest();
}

/**
 * Test MET Norway response parsing and unit normalization.
 */
static void TestMetNoParsing()
{
   StartTest(_T("MET Norway response parsing"));
   MetNoProvider provider;
   WeatherSnapshot *current = nullptr;
   ForecastCurve *forecast = nullptr;
   // The document is dated in the past, so its horizon filter keeps every point
   // and the last entry at or before "now" becomes the current snapshot.
   AssertTrue(provider.parseForecastResponse(s_docMetNo, strlen(s_docMetNo), 2, &current, &forecast));
   AssertNotNull(current);
   AssertNotNull(forecast);
   AssertTrue(forecast->points.size() == 2);

   ForecastPoint *p0 = forecast->points.get(0);
   AssertTrue(p0->targetTime == 1577836800);
   AssertTrue(FLOAT_EQ(p0->temperature, 16.0));
   AssertTrue(FLOAT_EQ(p0->cloudCover, 100.0));
   AssertTrue(FLOAT_EQ(p0->relativeHumidity, 92.2));
   AssertTrue(FLOAT_EQ(p0->windSpeed, 18.0));            // 5 m/s -> 18 km/h
   AssertTrue(FLOAT_EQ(p0->precipitation, 1.1));
   AssertTrue(isnan(p0->shortwaveRadiation));            // not provided by Locationforecast
   AssertTrue(isnan(p0->directRadiation));

   ForecastPoint *p1 = forecast->points.get(1);
   AssertTrue(p1->targetTime == 1577840400);
   AssertTrue(FLOAT_EQ(p1->windSpeed, 36.0));            // 10 m/s -> 36 km/h
   AssertTrue(isnan(p1->precipitation));                 // no next_1_hours block

   // Current snapshot is the last entry at or before now
   AssertTrue(current->observationTime == 1577840400);
   AssertTrue(FLOAT_EQ(current->temperature, 17.1));

   MemFree(current);
   delete forecast;
   EndTest();
}

/**
 * Test that a malformed MET Norway document is rejected.
 */
static void TestMetNoBadDocument()
{
   StartTest(_T("MET Norway malformed document handling"));
   MetNoProvider provider;
   WeatherSnapshot *current = nullptr;
   ForecastCurve *forecast = nullptr;
   static const char *doc = "{ \"properties\": { \"meta\": {} } }";
   AssertFalse(provider.parseForecastResponse(doc, strlen(doc), 2, &current, &forecast));
   AssertNull(current);
   AssertNull(forecast);
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
   TestIsoTimestamp();
   TestOpenMeteoUrls();
   TestMetNoUrl();
   TestForecastParsing();
   TestPartialResponse();
   TestEnsembleParsing();
   TestErrorDocument();
   TestMetNoParsing();
   TestMetNoBadDocument();

   return 0;
}
