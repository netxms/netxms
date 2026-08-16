/*
** NetXMS external check (Nagios-compatible plugin) unit tests
** Copyright (C) 2026 Raden Solutions
*/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <testtools.h>
#include <netxms-version.h>
#include "nxagentd.h"

NETXMS_EXECUTABLE_HEADER(test-unit-extcheck)

/**
 * Assert single perfdata entry content
 */
static void AssertPerfDataValue(const PerfDataValue *v, const TCHAR *label, const TCHAR *value, const TCHAR *uom,
         const TCHAR *warning, const TCHAR *critical, const TCHAR *min, const TCHAR *max)
{
   AssertNotNull(v);
   AssertEquals(v->label, label);
   AssertEquals(v->value, value);
   AssertEquals(v->uom, uom);
   AssertEquals(v->warning, warning);
   AssertEquals(v->critical, critical);
   AssertEquals(v->min, min);
   AssertEquals(v->max, max);
}

/**
 * Test basic perfdata parsing (check_ping style output)
 */
static void TestBasicPerfData()
{
   StartTest(_T("Basic perfdata parsing"));

   ObjectArray<PerfDataValue> values(0, 16, Ownership::True);
   AssertEquals(ParseNagiosPerfData(_T("rta=0.110000ms;100.000000;500.000000;0.000000 pl=0%;20;60;0"), &values), 2);
   AssertEquals(values.size(), 2);
   AssertPerfDataValue(values.get(0), _T("rta"), _T("0.110000"), _T("ms"), _T("100.000000"), _T("500.000000"), _T("0.000000"), _T(""));
   AssertPerfDataValue(values.get(1), _T("pl"), _T("0"), _T("%"), _T("20"), _T("60"), _T("0"), _T(""));

   EndTest();
}

/**
 * Test quoted labels, including escaped quote
 */
static void TestQuotedLabels()
{
   StartTest(_T("Quoted labels"));

   ObjectArray<PerfDataValue> values(0, 16, Ownership::True);
   AssertEquals(ParseNagiosPerfData(_T("'used space'=5610MB;4000;5000;0;6968 'it''s'=1"), &values), 2);
   AssertPerfDataValue(values.get(0), _T("used space"), _T("5610"), _T("MB"), _T("4000"), _T("5000"), _T("0"), _T("6968"));
   AssertPerfDataValue(values.get(1), _T("it's"), _T("1"), _T(""), _T(""), _T(""), _T(""), _T(""));

   EndTest();
}

/**
 * Test empty fields and Nagios range syntax in warn/crit
 */
static void TestEmptyFieldsAndRanges()
{
   StartTest(_T("Empty fields and range syntax"));

   ObjectArray<PerfDataValue> values(0, 16, Ownership::True);
   AssertEquals(ParseNagiosPerfData(_T("Connections=209c;;; load=3.15;@10:20;~:30;0; time=0.002"), &values), 3);
   AssertPerfDataValue(values.get(0), _T("Connections"), _T("209"), _T("c"), _T(""), _T(""), _T(""), _T(""));
   AssertPerfDataValue(values.get(1), _T("load"), _T("3.15"), _T(""), _T("@10:20"), _T("~:30"), _T("0"), _T(""));
   AssertPerfDataValue(values.get(2), _T("time"), _T("0.002"), _T(""), _T(""), _T(""), _T(""), _T(""));

   EndTest();
}

/**
 * Test numeric value forms - sign, missing integer part, exponent
 */
static void TestNumericForms()
{
   StartTest(_T("Numeric value forms"));

   ObjectArray<PerfDataValue> values(0, 16, Ownership::True);
   AssertEquals(ParseNagiosPerfData(_T("neg=-5.5C power=1.2e3W offset=+.5s weird=12e"), &values), 4);
   AssertPerfDataValue(values.get(0), _T("neg"), _T("-5.5"), _T("C"), _T(""), _T(""), _T(""), _T(""));
   AssertPerfDataValue(values.get(1), _T("power"), _T("1.2e3"), _T("W"), _T(""), _T(""), _T(""), _T(""));
   AssertPerfDataValue(values.get(2), _T("offset"), _T("+.5"), _T("s"), _T(""), _T(""), _T(""), _T(""));
   AssertPerfDataValue(values.get(3), _T("weird"), _T("12"), _T("e"), _T(""), _T(""), _T(""), _T(""));

   EndTest();
}

/**
 * Test that malformed entries are skipped without affecting valid ones
 */
static void TestMalformedEntries()
{
   StartTest(_T("Malformed entries"));

   ObjectArray<PerfDataValue> values(0, 16, Ownership::True);
   AssertEquals(ParseNagiosPerfData(_T("foo bar=abc temp=U =5 ok=1;2;3;4;5"), &values), 1);
   AssertEquals(values.size(), 1);
   AssertPerfDataValue(values.get(0), _T("ok"), _T("1"), _T(""), _T("2"), _T("3"), _T("4"), _T("5"));

   values.clear();
   AssertEquals(ParseNagiosPerfData(_T(""), &values), 0);
   AssertEquals(ParseNagiosPerfData(_T("   "), &values), 0);
   AssertEquals(values.size(), 0);

   EndTest();
}

/**
 * Test single line plugin output parsing
 */
static void TestSingleLineOutput()
{
   StartTest(_T("Single line plugin output"));

   StringList output;
   output.add(_T("DISK OK - free space: / 3326 MB (56%); | /=2643MB;5948;5958;0;6968"));

   StringBuffer statusText;
   ObjectArray<PerfDataValue> perfData(0, 16, Ownership::True);
   ParseNagiosPluginOutput(output, &statusText, &perfData);
   AssertEquals(statusText.cstr(), _T("DISK OK - free space: / 3326 MB (56%);"));
   AssertEquals(perfData.size(), 1);
   AssertPerfDataValue(perfData.get(0), _T("/"), _T("2643"), _T("MB"), _T("5948"), _T("5958"), _T("0"), _T("6968"));

   EndTest();
}

/**
 * Test multi-line plugin output parsing (example from Nagios plugin API specification):
 * long text lines are ignored, perfdata continues after "|" on a subsequent line
 */
static void TestMultilineOutput()
{
   StartTest(_T("Multi-line plugin output"));

   StringList output;
   output.add(_T("DISK OK - free space: / 3326 MB (56%);|/=2643MB;5948;5958;0;6968"));
   output.add(_T("/ 15272 MB (77%);"));
   output.add(_T("/boot 68 MB (69%);"));
   output.add(_T("/home 69357 MB (27%);"));
   output.add(_T("/var/log 819 MB (84%); | /boot=68MB;88;93;0;98"));
   output.add(_T("/home=69357MB;253404;253409;0;253414"));
   output.add(_T("/var/log=818MB;970;975;0;980"));

   StringBuffer statusText;
   ObjectArray<PerfDataValue> perfData(0, 16, Ownership::True);
   ParseNagiosPluginOutput(output, &statusText, &perfData);
   AssertEquals(statusText.cstr(), _T("DISK OK - free space: / 3326 MB (56%);"));
   AssertEquals(perfData.size(), 4);
   AssertPerfDataValue(perfData.get(0), _T("/"), _T("2643"), _T("MB"), _T("5948"), _T("5958"), _T("0"), _T("6968"));
   AssertPerfDataValue(perfData.get(1), _T("/boot"), _T("68"), _T("MB"), _T("88"), _T("93"), _T("0"), _T("98"));
   AssertPerfDataValue(perfData.get(2), _T("/home"), _T("69357"), _T("MB"), _T("253404"), _T("253409"), _T("0"), _T("253414"));
   AssertPerfDataValue(perfData.get(3), _T("/var/log"), _T("818"), _T("MB"), _T("970"), _T("975"), _T("0"), _T("980"));

   EndTest();
}

/**
 * Test plugin output without perfdata and empty output
 */
static void TestOutputWithoutPerfData()
{
   StartTest(_T("Output without perfdata"));

   StringList output;
   output.add(_T("PING OK - Packet loss = 0%"));

   StringBuffer statusText;
   ObjectArray<PerfDataValue> perfData(0, 16, Ownership::True);
   ParseNagiosPluginOutput(output, &statusText, &perfData);
   AssertEquals(statusText.cstr(), _T("PING OK - Packet loss = 0%"));
   AssertEquals(perfData.size(), 0);

   StringList emptyOutput;
   StringBuffer emptyText;
   ParseNagiosPluginOutput(emptyOutput, &emptyText, &perfData);
   AssertTrue(emptyText.isEmpty());
   AssertEquals(perfData.size(), 0);

   EndTest();
}

/**
 * Test parsing of ExternalCheck configuration entries
 */
static void TestAddExternalCheck()
{
   StartTest(_T("External check configuration"));

   TCHAR config1[] = _T("disk:/usr/lib/nagios/plugins/check_disk -w 20% -c 10% -p /");
   AssertTrue(AddExternalCheck(config1));

   TCHAR config2[] = _T(" mysql : /usr/lib/nagios/plugins/check_mysql -H localhost ");
   AssertTrue(AddExternalCheck(config2));

   TCHAR config3[] = _T("winsvc:C:\\Plugins\\check_service.exe");
   AssertTrue(AddExternalCheck(config3));

   TCHAR invalid1[] = _T("nocommand");
   AssertFalse(AddExternalCheck(invalid1));

   TCHAR invalid2[] = _T(":check_disk");
   AssertFalse(AddExternalCheck(invalid2));

   TCHAR invalid3[] = _T("disk:");
   AssertFalse(AddExternalCheck(invalid3));

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

   TestBasicPerfData();
   TestQuotedLabels();
   TestEmptyFieldsAndRanges();
   TestNumericForms();
   TestMalformedEntries();
   TestSingleLineOutput();
   TestMultilineOutput();
   TestOutputWithoutPerfData();
   TestAddExternalCheck();

   return 0;
}
