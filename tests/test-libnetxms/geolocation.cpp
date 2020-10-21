#include <geolocation.h>
#include <testtools.h>

#ifndef _WIN32
#include <signal.h>
#endif

/**
 * Test  
 */
void TestGeoLocationTriangle()
{
   StartTest(_T("Test location within triangle"));
   ObjectArray<GeoLocation> triangle(3, 3, Ownership::True);
   triangle.add(new GeoLocation(GL_MANUAL, 48.729058, 21.256807));
   triangle.add(new GeoLocation(GL_MANUAL, 48.730155, 21.257869));
   triangle.add(new GeoLocation(GL_MANUAL, 48.728393, 21.259468));

   GeoLocation point(GL_MANUAL, 48.729015, 21.257829);
   AssertTrue(point.isWithinArea(triangle));

   GeoLocation point2(GL_MANUAL, 48.727841, 21.251716);
   AssertFalse(point2.isWithinArea(triangle));
   EndTest();
}

/**
 * Test geo location
 */
void TestGeoLocationPolygon()
{
   StartTest(_T("Test location within polygon"));
   ObjectArray<GeoLocation> polygon(3, 3, Ownership::True);
   polygon.add(new GeoLocation(GL_MANUAL, 57.22338, 24.54132));
   polygon.add(new GeoLocation(GL_MANUAL, 57.24271, 24.78301));
   polygon.add(new GeoLocation(GL_MANUAL, 57.11841, 24.71984));
   polygon.add(new GeoLocation(GL_MANUAL, 57.10722, 24.9121));
   polygon.add(new GeoLocation(GL_MANUAL, 57.23082, 24.93682));
   polygon.add(new GeoLocation(GL_MANUAL, 57.22338, 25.11947));
   polygon.add(new GeoLocation(GL_MANUAL, 57.05498, 25.06454));
   polygon.add(new GeoLocation(GL_MANUAL, 57.06095, 24.81735));
   polygon.add(new GeoLocation(GL_MANUAL, 56.98321, 24.77889));
   polygon.add(new GeoLocation(GL_MANUAL, 56.96375, 25.07003));
   polygon.add(new GeoLocation(GL_MANUAL, 56.84602, 25.02883));
   polygon.add(new GeoLocation(GL_MANUAL, 56.89556, 24.6704));
   polygon.add(new GeoLocation(GL_MANUAL, 56.98995, 24.6965));
   polygon.add(new GeoLocation(GL_MANUAL, 56.9877, 24.58663));
   polygon.add(new GeoLocation(GL_MANUAL, 56.90081, 24.57565));
   polygon.add(new GeoLocation(GL_MANUAL, 56.91431, 24.40536));
   polygon.add(new GeoLocation(GL_MANUAL, 56.94803, 24.40811));
   polygon.add(new GeoLocation(GL_MANUAL, 56.94728, 24.53033));
   polygon.add(new GeoLocation(GL_MANUAL, 57.17503, 24.68826));
   polygon.add(new GeoLocation(GL_MANUAL, 57.178, 24.57015));
   polygon.add(new GeoLocation(GL_MANUAL, 56.98471, 24.4699));
   polygon.add(new GeoLocation(GL_MANUAL, 56.99668, 24.40948));

   GeoLocation point(GL_MANUAL, 57.09996, 24.59066);
   AssertFalse(point.isWithinArea(polygon));

   GeoLocation point2(GL_MANUAL, 57.0081, 24.45195);
   AssertTrue(point2.isWithinArea(polygon));

   GeoLocation point3(GL_MANUAL, 57.05219, 24.74858);
   AssertTrue(point3.isWithinArea(polygon));

   GeoLocation point4(GL_MANUAL, 56.96545, 24.66344);
   AssertFalse(point4.isWithinArea(polygon));
   EndTest();
}

/**
 * Test for function that checks if point on map in within of 
 */
void TestGeoLocation()
{
   TestGeoLocationTriangle();
   TestGeoLocationPolygon();
}
