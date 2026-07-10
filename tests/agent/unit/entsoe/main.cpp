/*
** NetXMS ENTSO-E subagent unit tests
** Copyright (C) 2026 Raden Solutions
*/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <testtools.h>
#include <netxms-version.h>
#include <math.h>
#include "entsoe.h"

NETXMS_EXECUTABLE_HEADER(test-unit-entsoe)

/**
 * A75 actual-generation document: solar (B16) 300 MW, gas (B04) 700 MW.
 * Start is far in the past so every point is a fully-elapsed interval.
 */
static const char *s_docBasic =
   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
   "<GL_MarketDocument xmlns=\"urn:iec62325.351:tc57wg16:451-6:generationloaddocument:3:0\">\n"
   "  <TimeSeries>\n"
   "    <inBiddingZone_Domain.mRID codingScheme=\"A01\">10YLV-1001A00074</inBiddingZone_Domain.mRID>\n"
   "    <MktPSRType><psrType>B16</psrType></MktPSRType>\n"
   "    <Period>\n"
   "      <timeInterval><start>2020-01-01T00:00Z</start><end>2020-01-01T01:00Z</end></timeInterval>\n"
   "      <resolution>PT60M</resolution>\n"
   "      <Point><position>1</position><quantity>300</quantity></Point>\n"
   "    </Period>\n"
   "  </TimeSeries>\n"
   "  <TimeSeries>\n"
   "    <inBiddingZone_Domain.mRID codingScheme=\"A01\">10YLV-1001A00074</inBiddingZone_Domain.mRID>\n"
   "    <MktPSRType><psrType>B04</psrType></MktPSRType>\n"
   "    <Period>\n"
   "      <timeInterval><start>2020-01-01T00:00Z</start><end>2020-01-01T01:00Z</end></timeInterval>\n"
   "      <resolution>PT60M</resolution>\n"
   "      <Point><position>1</position><quantity>700</quantity></Point>\n"
   "    </Period>\n"
   "  </TimeSeries>\n"
   "</GL_MarketDocument>\n";

/**
 * Document with pumped-storage generation (B10, included in list but excluded
 * from CI/total) and a separate pumped-storage consumption series (out only,
 * must be skipped entirely). Plus a sparse series exercising forward-fill.
 */
static const char *s_docStorage =
   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
   "<GL_MarketDocument xmlns=\"urn:iec62325.351:tc57wg16:451-6:generationloaddocument:3:0\">\n"
   "  <TimeSeries>\n"
   "    <inBiddingZone_Domain.mRID>10YLV-1001A00074</inBiddingZone_Domain.mRID>\n"
   "    <MktPSRType><psrType>B16</psrType></MktPSRType>\n"
   "    <Period>\n"
   "      <timeInterval><start>2020-01-01T00:00Z</start><end>2020-01-01T01:00Z</end></timeInterval>\n"
   "      <resolution>PT15M</resolution>\n"
   "      <Point><position>1</position><quantity>300</quantity></Point>\n"
   "    </Period>\n"
   "  </TimeSeries>\n"
   "  <TimeSeries>\n"
   "    <inBiddingZone_Domain.mRID>10YLV-1001A00074</inBiddingZone_Domain.mRID>\n"
   "    <MktPSRType><psrType>B04</psrType></MktPSRType>\n"
   "    <Period>\n"
   "      <timeInterval><start>2020-01-01T00:00Z</start><end>2020-01-01T01:00Z</end></timeInterval>\n"
   "      <resolution>PT15M</resolution>\n"
   "      <Point><position>1</position><quantity>500</quantity></Point>\n"
   "      <Point><position>3</position><quantity>700</quantity></Point>\n"
   "    </Period>\n"
   "  </TimeSeries>\n"
   "  <TimeSeries>\n"
   "    <inBiddingZone_Domain.mRID>10YLV-1001A00074</inBiddingZone_Domain.mRID>\n"
   "    <MktPSRType><psrType>B10</psrType></MktPSRType>\n"
   "    <Period>\n"
   "      <timeInterval><start>2020-01-01T00:00Z</start><end>2020-01-01T01:00Z</end></timeInterval>\n"
   "      <resolution>PT15M</resolution>\n"
   "      <Point><position>1</position><quantity>100</quantity></Point>\n"
   "    </Period>\n"
   "  </TimeSeries>\n"
   "  <TimeSeries>\n"
   "    <outBiddingZone_Domain.mRID>10YLV-1001A00074</outBiddingZone_Domain.mRID>\n"
   "    <MktPSRType><psrType>B10</psrType></MktPSRType>\n"
   "    <Period>\n"
   "      <timeInterval><start>2020-01-01T00:00Z</start><end>2020-01-01T01:00Z</end></timeInterval>\n"
   "      <resolution>PT15M</resolution>\n"
   "      <Point><position>1</position><quantity>50</quantity></Point>\n"
   "    </Period>\n"
   "  </TimeSeries>\n"
   "</GL_MarketDocument>\n";

/**
 * ENTSO-E acknowledgement (error) document.
 */
static const char *s_docAck =
   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
   "<Acknowledgement_MarketDocument xmlns=\"urn:iec62325.351:tc57wg16:451-3:acknowledgementdocument:8:0\">\n"
   "  <Reason><code>999</code><text>No matching data found</text></Reason>\n"
   "</Acknowledgement_MarketDocument>\n";

#define FLOAT_EQ(a, b)  (fabs((a) - (b)) < 0.01)

static double FindType(GenerationSnapshot *s, const char *code)
{
   for(int i = 0; i < s->byType.size(); i++)
      if (!strcmp(s->byType.get(i)->psrType, code))
         return s->byType.get(i)->mw;
   return -1;
}

/**
 * Test ISO 8601 helpers.
 */
static void TestTimeHelpers()
{
   StartTest(_T("ISO 8601 duration parsing"));
   AssertTrue(ParseIso8601Duration("PT60M") == 3600);
   AssertTrue(ParseIso8601Duration("PT15M") == 900);
   AssertTrue(ParseIso8601Duration("PT30M") == 1800);
   AssertTrue(ParseIso8601Duration("PT1H") == 3600);
   AssertTrue(ParseIso8601Duration("P1D") == 86400);
   AssertTrue(ParseIso8601Duration("garbage") == 0);
   EndTest();

   StartTest(_T("ISO 8601 timestamp parsing"));
   AssertTrue(ParseIso8601Utc("2020-01-01T00:00Z") == 1577836800);
   AssertTrue(ParseIso8601Utc("2020-01-01T00:00:00Z") == 1577836800);
   AssertTrue(ParseIso8601Utc("bad") == 0);
   EndTest();
}

/**
 * Test carbon-intensity and share computation.
 */
static void TestCarbonMetrics()
{
   EmissionFactorSet factors;

   StartTest(_T("Generation parsing and carbon intensity"));
   GenerationSnapshot *s = ParseGenerationDocument(s_docBasic, strlen(s_docBasic), factors);
   AssertNotNull(s);
   AssertTrue(s->byType.size() == 2);
   AssertTrue(FLOAT_EQ(FindType(s, "B16"), 300.0));
   AssertTrue(FLOAT_EQ(FindType(s, "B04"), 700.0));
   AssertTrue(FLOAT_EQ(s->totalMW, 1000.0));
   // CI = (300*45 + 700*490) / 1000 = 356.5
   AssertTrue(FLOAT_EQ(s->carbonIntensity, 356.5));
   // renewable share = 300 / 1000 = 30%
   AssertTrue(FLOAT_EQ(s->renewableShare, 30.0));
   AssertTrue(FLOAT_EQ(s->lowCarbonShare, 30.0));
   delete s;
   EndTest();
}

/**
 * Test storage exclusion, consumption-series skip and forward-fill.
 */
static void TestStorageAndForwardFill()
{
   EmissionFactorSet factors;

   StartTest(_T("Storage exclusion and forward-fill"));
   GenerationSnapshot *s = ParseGenerationDocument(s_docStorage, strlen(s_docStorage), factors);
   AssertNotNull(s);
   // B16, B04, B10 discharge present; consumption B10 series skipped -> 3 rows
   AssertTrue(s->byType.size() == 3);
   AssertTrue(FLOAT_EQ(FindType(s, "B16"), 300.0));
   // B04 has points at position 1 and 3 (gap at 2); latest elapsed is well past
   // position 3, so forward-fill yields the position-3 value (700).
   AssertTrue(FLOAT_EQ(FindType(s, "B04"), 700.0));
   // B10 discharge present in list but not counted (consumption 50 must not add)
   AssertTrue(FLOAT_EQ(FindType(s, "B10"), 100.0));
   // total excludes storage: 300 + 700 = 1000
   AssertTrue(FLOAT_EQ(s->totalMW, 1000.0));
   // CI over non-storage: (300*45 + 700*490) / 1000 = 356.5
   AssertTrue(FLOAT_EQ(s->carbonIntensity, 356.5));
   delete s;
   EndTest();
}

/**
 * Test acknowledgement handling.
 */
static void TestAcknowledgement()
{
   EmissionFactorSet factors;

   StartTest(_T("Acknowledgement document handling"));
   GenerationSnapshot *s = ParseGenerationDocument(s_docAck, strlen(s_docAck), factors);
   AssertNull(s);
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

   TestTimeHelpers();
   TestCarbonMetrics();
   TestStorageAndForwardFill();
   TestAcknowledgement();

   return 0;
}
