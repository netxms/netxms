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
   StartTest(_T("Test location occurrence in triangle"));
   ObjectArray<GeoLocation> rectangle(3, 3, Ownership::True);
   rectangle.add(new GeoLocation(GL_MANUAL, 48.729058, 21.256807));
   rectangle.add(new GeoLocation(GL_MANUAL, 48.730155, 21.257869));
   rectangle.add(new GeoLocation(GL_MANUAL, 48.728393, 21.259468));

   GeoLocation point(GL_MANUAL, 48.729015, 21.257829);
   AssertTrue(point.isInPolygon(rectangle));

   GeoLocation point2(GL_MANUAL, 48.727841, 21.251716);
   AssertTrue(!point2.isInPolygon(rectangle));
   EndTest();
}

/**
 * Test geo location
 */
void TestGeoLocationPolygon()
{
   StartTest(_T("Test location occurrence in polygon"));
   ObjectArray<GeoLocation> rectangle(3, 3, Ownership::True);
   rectangle.add(new GeoLocation(GL_MANUAL, 57.22338, 24.54132));
   rectangle.add(new GeoLocation(GL_MANUAL, 57.24271, 24.78301));
   rectangle.add(new GeoLocation(GL_MANUAL, 57.11841, 24.71984));
   rectangle.add(new GeoLocation(GL_MANUAL, 57.10722, 24.9121));
   rectangle.add(new GeoLocation(GL_MANUAL, 57.23082, 24.93682));
   rectangle.add(new GeoLocation(GL_MANUAL, 57.22338, 25.11947));
   rectangle.add(new GeoLocation(GL_MANUAL, 57.05498, 25.06454));
   rectangle.add(new GeoLocation(GL_MANUAL, 57.06095, 24.81735));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.98321, 24.77889));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.96375, 25.07003));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.84602, 25.02883));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.89556, 24.6704));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.98995, 24.6965));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.9877, 24.58663));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.90081, 24.57565));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.91431, 24.40536));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.94803, 24.40811));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.94728, 24.53033));
   rectangle.add(new GeoLocation(GL_MANUAL, 57.17503, 24.68826));
   rectangle.add(new GeoLocation(GL_MANUAL, 57.178, 24.57015));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.98471, 24.4699));
   rectangle.add(new GeoLocation(GL_MANUAL, 56.99668, 24.40948));

   GeoLocation point(GL_MANUAL, 57.09996, 24.59066);
   AssertTrue(!point.isInPolygon(rectangle));

   GeoLocation point2(GL_MANUAL, 57.0081, 24.45195);
   AssertTrue(point2.isInPolygon(rectangle));

   GeoLocation point3(GL_MANUAL, 57.05219, 24.74858);
   AssertTrue(point3.isInPolygon(rectangle));

   GeoLocation point4(GL_MANUAL, 56.96545, 24.66344);
   AssertTrue(!point4.isInPolygon(rectangle));
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
