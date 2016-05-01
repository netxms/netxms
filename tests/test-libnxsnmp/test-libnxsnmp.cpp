#include <nms_common.h>
#include <nms_util.h>
#include <nxsnmp.h>
#include <testtools.h>

static UINT32 s_sysDescription[] = { 1, 3, 6, 1, 2, 1, 1, 1, 0 };
static SNMP_ObjectId s_oidSysDescription(s_sysDescription, sizeof(s_sysDescription) / sizeof(UINT32));
static UINT32 s_unsignedTest[] = { 1, 3, 6, 1, 2, 1, 1, 2164260864, 0 };
static UINT32 s_sysLocation[] = { 1, 3, 6, 1, 2, 1, 1, 6, 0 };
static SNMP_ObjectId s_oidSysLocation(s_sysLocation, sizeof(s_sysLocation) / sizeof(UINT32));
static UINT32 s_system[] = { 1, 3, 6, 1, 2, 1, 1 };
static SNMP_ObjectId s_oidSystem(s_system, sizeof(s_system) / sizeof(UINT32));

/**
 * Test OID conversion
 */
static void TestOidConversion()
{
   TCHAR text[256];
   UINT32 bin[256];

   StartTest(_T("SNMPConvertOIDToText"));
   SNMPConvertOIDToText(9, s_sysDescription, text, 256);
   AssertTrue(!_tcscmp(text, _T(".1.3.6.1.2.1.1.1.0")));
   EndTest();

   StartTest(_T("SNMPConvertOIDToText - handling high bit set"));
   SNMPConvertOIDToText(9, s_unsignedTest, text, 256);
   AssertTrue(!_tcscmp(text, _T(".1.3.6.1.2.1.1.2164260864.0")));
   EndTest();

   StartTest(_T("SNMPParseOID"));
   AssertEquals(SNMPParseOID(_T(".1.3.6.1.2.1.1.1.0"), bin, 256), 9);
   AssertTrue(!memcmp(bin, s_sysDescription, 9 * sizeof(UINT32)));
   EndTest();

   StartTest(_T("SNMPParseOID - handling high bit set"));
   AssertEquals(SNMPParseOID(_T(".1.3.6.1.2.1.1.2164260864.0"), bin, 256), 9);
   AssertTrue(!memcmp(bin, s_unsignedTest, 9 * sizeof(UINT32)));
   EndTest();
}

/**
 * Test SNMP_ObjectId class
 */
static void TestOidClass()
{
   StartTest(_T("SNMP_ObjectId::toString"));
   AssertTrue(!_tcscmp(s_oidSysDescription.toString(), _T(".1.3.6.1.2.1.1.1.0")));
   EndTest();

   StartTest(_T("SNMP_ObjectId::length"));
   AssertEquals(s_oidSysDescription.length(), 9);
   EndTest();

   StartTest(_T("SNMP_ObjectId::value"));
   AssertTrue(!memcmp(s_oidSysDescription.value(), s_sysDescription, 9 * sizeof(UINT32)));
   EndTest();

   StartTest(_T("SNMP_ObjectId::compare"));
   AssertEquals(s_oidSysDescription.compare(s_sysDescription, 9), OID_EQUAL);
   AssertEquals(s_oidSysDescription.compare(s_unsignedTest, 9), OID_PRECEDING);
   AssertEquals(s_oidSysLocation.compare(s_oidSysDescription), OID_FOLLOWING);
   AssertEquals(s_oidSysLocation.compare(s_system, 7), OID_LONGER);
   AssertEquals(s_oidSystem.compare(s_oidSysLocation), OID_SHORTER);
   EndTest();

   StartTest(_T("SNMP_ObjectId copy constructor"));
   SNMP_ObjectId copy(s_oidSysLocation);
   AssertEquals(copy.compare(s_oidSysLocation), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_ObjectId::operator ="));
   copy = s_oidSysDescription;
   AssertEquals(copy.compare(s_oidSysDescription), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_ObjectId::extend"));
   copy = s_oidSystem;
   copy.extend(1);
   copy.extend(0);
   AssertEquals(copy.compare(s_oidSysDescription), OID_EQUAL);
   copy = s_oidSystem;
   static UINT32 ext[] = { 6, 0 };
   copy.extend(ext, 2);
   AssertEquals(copy.compare(s_oidSysLocation), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_ObjectId::truncate"));
   copy.truncate(2);
   AssertEquals(copy.compare(s_oidSystem), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_ObjectId::getElement"));
   AssertEquals(copy.getElement(1), 3);
   AssertEquals(copy.getElement(100), 0);
   EndTest();

   StartTest(_T("SNMP_ObjectId::changeElement"));
   copy = s_oidSysDescription;
   copy.changeElement(7, 6);
   AssertEquals(copy.compare(s_oidSysLocation), OID_EQUAL);
   EndTest();
}

/**
 * Test SNMP_Variable class
 */
static void TestVariableClass()
{
   StartTest(_T("SNMP_Variable default constructor"));
   SNMP_Variable v1;
   AssertFalse(v1.getName().isValid());
   EndTest();

   StartTest(_T("SNMP_Variable constructor from binary OID"));
   SNMP_Variable v2(s_sysDescription, 9);
   AssertEquals(v2.getName().compare(s_oidSysDescription), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_Variable constructor from OID object"));
   SNMP_Variable v3(s_oidSysLocation);
   AssertEquals(v3.getName().compare(s_oidSysLocation), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_Variable constructor from text OID"));
   SNMP_Variable v4(_T(".1.3.6.1.2.1.1.6.0"));
   AssertEquals(v4.getName().compare(s_oidSysLocation), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_Variable::setValueFromString"));
   v4.setValueFromString(ASN_COUNTER32, _T("42"));
   AssertEquals(v4.getValueAsInt(), 42);
   EndTest();

   StartTest(_T("SNMP_Variable copy constructor"));
   SNMP_Variable v5(&v4);
   AssertEquals(v5.getName().compare(s_oidSysLocation), OID_EQUAL);
   AssertEquals(v5.getValueAsInt(), 42);
   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   TestOidConversion();
   TestOidClass();
   TestVariableClass();
   return 0;
}
