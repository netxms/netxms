#include <nms_common.h>
#include <nms_util.h>
#include <nxsnmp.h>
#include <testtools.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(test-libnxsnmp)

static uint32_t s_sysDescription[] = { 1, 3, 6, 1, 2, 1, 1, 1, 0 };
static SNMP_ObjectId s_oidSysDescription(s_sysDescription, sizeof(s_sysDescription) / sizeof(uint32_t));
static uint32_t s_unsignedTest[] = { 1, 3, 6, 1, 2, 1, 1, 2164260864, 0 };
static uint32_t s_sysLocation[] = { 1, 3, 6, 1, 2, 1, 1, 6, 0 };
static SNMP_ObjectId s_oidSysLocation(s_sysLocation, sizeof(s_sysLocation) / sizeof(uint32_t));
static uint32_t s_system[] = { 1, 3, 6, 1, 2, 1, 1 };
static SNMP_ObjectId s_oidSystem(s_system, sizeof(s_system) / sizeof(uint32_t));

/**
 * Test OID conversion
 */
static void TestOidConversion()
{
   TCHAR text[256];
   uint32_t bin[256];

   StartTest(_T("SNMPConvertOIDToText"));
   SnmpConvertOIDToText(9, s_sysDescription, text, 256);
   AssertTrue(!_tcscmp(text, _T("1.3.6.1.2.1.1.1.0")));
   EndTest();

   StartTest(_T("SNMPConvertOIDToText - handling high bit set"));
   SnmpConvertOIDToText(9, s_unsignedTest, text, 256);
   AssertTrue(!_tcscmp(text, _T("1.3.6.1.2.1.1.2164260864.0")));
   EndTest();

   StartTest(_T("SNMPParseOID"));
   AssertEquals(SnmpParseOID(_T("1.3.6.1.2.1.1.1.0"), bin, 256), 9);
   AssertTrue(!memcmp(bin, s_sysDescription, 9 * sizeof(uint32_t)));
   EndTest();

   StartTest(_T("SNMPParseOID - handling high bit set"));
   AssertEquals(SnmpParseOID(_T("1.3.6.1.2.1.1.2164260864.0"), bin, 256), 9);
   AssertTrue(!memcmp(bin, s_unsignedTest, 9 * sizeof(uint32_t)));
   EndTest();

   StartTest(_T("SNMPParseOID - leading dot"));
   AssertEquals(SnmpParseOID(_T(".1.3.6.1.2.1.1.1.0"), bin, 256), 9);
   AssertTrue(!memcmp(bin, s_sysDescription, 9 * sizeof(uint32_t)));
   EndTest();
}

/**
 * Test SNMP_ObjectId class
 */
static void TestOidClass()
{
   StartTest(_T("SNMP_ObjectId::toString"));
   AssertTrue(!_tcscmp(s_oidSysDescription.toString(), _T("1.3.6.1.2.1.1.1.0")));
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

   StartTest(_T("SNMP_ObjectId::equals"));
   AssertTrue(s_oidSysDescription.equals(s_sysDescription, 9));
   AssertTrue(s_oidSysDescription.equals(_T("1.3.6.1.2.1.1.1.0")));
   AssertTrue(s_oidSysDescription.equals({ 1, 3, 6, 1, 2, 1, 1, 1, 0 }));
   EndTest();

   StartTest(_T("SNMP_ObjectId::startsWith"));
   AssertTrue(s_oidSysDescription.startsWith(s_system, 7));
   AssertTrue(s_oidSysDescription.startsWith(s_sysDescription, 9));
   AssertTrue(s_oidSysDescription.startsWith(_T("1.3.6.1.2")));
   AssertTrue(s_oidSysDescription.startsWith(_T("1.3.6.1.2.1.1.1.0")));
   AssertTrue(s_oidSysDescription.startsWith({ 1, 3, 6, 1 }));
   AssertTrue(s_oidSysDescription.startsWith({ 1, 3, 6, 1, 2, 1, 1, 1, 0 }));
   EndTest();

   StartTest(_T("SNMP_ObjectId initializer list constructor"));
   SNMP_ObjectId initListOid({ 1, 3, 6, 1, 2, 1, 1, 6, 0 });
   AssertEquals(initListOid.compare(s_oidSysLocation), OID_EQUAL);
   AssertTrue(!memcmp(initListOid.value(), s_sysLocation, sizeof(s_sysLocation)));
   EndTest();

   StartTest(_T("SNMP_ObjectId copy constructor"));
   SNMP_ObjectId copy(s_oidSysLocation);
   AssertEquals(copy.compare(s_oidSysLocation), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_ObjectId move constructor"));
   SNMP_ObjectId temp(s_oidSysLocation);
   SNMP_ObjectId moved(std::move(temp));
   AssertEquals(moved.compare(s_oidSysLocation), OID_EQUAL);
   AssertEquals(temp.length(), 0);
   EndTest();

   StartTest(_T("SNMP_ObjectId::operator ="));
   copy = s_oidSysDescription;
   AssertEquals(copy.compare(s_oidSysDescription), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_ObjectId::operator = (move semantics)"));
   copy = std::move(moved);
   AssertEquals(copy.compare(s_oidSysLocation), OID_EQUAL);
   AssertEquals(moved.length(), 0);
   EndTest();

   StartTest(_T("SNMP_ObjectId::extend"));
   copy = s_oidSystem;
   copy.extend(1);
   copy.extend(0);
   AssertEquals(copy.compare(s_oidSysDescription), OID_EQUAL);
   copy = s_oidSystem;
   static uint32_t ext[] = { 6, 0 };
   copy.extend(ext, 2);
   AssertEquals(copy.compare(s_oidSysLocation), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_ObjectId extension constructors"));
   SNMP_ObjectId d1(s_oidSystem, ext, 2);
   AssertEquals(d1.compare(s_oidSysLocation), OID_EQUAL);
   SNMP_ObjectId d2(s_oidSystem, 1);
   AssertEquals(d2.length(), 8);
   AssertTrue(!memcmp(d2.value(), s_sysDescription, 8 * sizeof(uint32_t)));
   EndTest();

   StartTest(_T("SNMP_ObjectId::truncate"));
   copy.truncate(2);
   AssertEquals(copy.compare(s_oidSystem), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_ObjectId::getElement"));
   AssertEquals(copy.getElement(1), 3u);
   AssertEquals(copy.getElement(100), 0u);
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
   SNMP_Variable v4(_T("1.3.6.1.2.1.1.6.0"));
   AssertEquals(v4.getName().compare(s_oidSysLocation), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_Variable constructor from text OID (leading dot)"));
   SNMP_Variable v6(_T(".1.3.6.1.2.1.1.6.0"));
   AssertEquals(v6.getName().compare(s_oidSysLocation), OID_EQUAL);
   EndTest();

   StartTest(_T("SNMP_Variable::setValueFromString"));
   v4.setValueFromString(ASN_COUNTER32, _T("42"));
   AssertEquals(v4.getValueAsInt(), 42);
   EndTest();

   StartTest(_T("SNMP_Variable copy constructor"));
   SNMP_Variable v5(v4);
   AssertEquals(v5.getName().compare(s_oidSysLocation), OID_EQUAL);
   AssertEquals(v5.getValueAsInt(), 42);
   AssertEquals(v4.getName().compare(s_oidSysLocation), OID_EQUAL);
   AssertEquals(v4.getValueAsInt(), 42);

   SNMP_Variable v7(s_oidSysDescription);
   v7.setValueFromString(ASN_OCTET_STRING, _T("Some long string that exceeds internal value buffer in variable class"));
   SNMP_Variable v8(v7);
   AssertEquals(v8.getName().compare(s_oidSysDescription), OID_EQUAL);
   TCHAR buffer[256];
   v8.getValueAsString(buffer, 256);
   AssertEquals(buffer, _T("Some long string that exceeds internal value buffer in variable class"));
   AssertEquals(v7.getName().compare(s_oidSysDescription), OID_EQUAL);
   v7.getValueAsString(buffer, 256);
   AssertEquals(buffer, _T("Some long string that exceeds internal value buffer in variable class"));
   EndTest();

   StartTest(_T("SNMP_Variable move constructor"));
   SNMP_Variable v9(std::move(v4));
   AssertEquals(v9.getName().compare(s_oidSysLocation), OID_EQUAL);
   AssertEquals(v9.getValueAsInt(), 42);

   SNMP_Variable v10(std::move(v7));
   AssertEquals(v10.getName().compare(s_oidSysDescription), OID_EQUAL);
   v10.getValueAsString(buffer, 256);
   AssertEquals(buffer, _T("Some long string that exceeds internal value buffer in variable class"));

   v7.getValueAsString(buffer, 256);
   AssertEquals(static_cast<int32_t>(buffer[0]), 0);  // v7 should be in "empty" state after move
   EndTest();

   StartTest(_T("SNMP_Variable assignment operator"));
   v10 = v9;
   AssertEquals(v10.getName().compare(s_oidSysLocation), OID_EQUAL);
   AssertEquals(v10.getValueAsInt(), 42);

   SNMP_Variable v11(s_oidSysDescription);
   v11.setValueFromString(ASN_OCTET_STRING, _T("Some long string that exceeds internal value buffer in variable class"));
   v10 = v11;
   AssertEquals(v10.getName().compare(s_oidSysDescription), OID_EQUAL);
   v10.getValueAsString(buffer, 256);
   AssertEquals(buffer, _T("Some long string that exceeds internal value buffer in variable class"));
   AssertEquals(v11.getName().compare(s_oidSysDescription), OID_EQUAL);
   v11.getValueAsString(buffer, 256);
   AssertEquals(buffer, _T("Some long string that exceeds internal value buffer in variable class"));
   EndTest();

   StartTest(_T("SNMP_Variable assignment operator (move semantics)"));
   v11 = SNMP_Variable(s_oidSysLocation);
   v11.setValueFromString(ASN_COUNTER32, _T("84"));
   AssertEquals(v11.getName().compare(s_oidSysLocation), OID_EQUAL);
   AssertEquals(v11.getValueAsInt(), 84);

   v10 = std::move(v9);
   AssertEquals(v10.getName().compare(s_oidSysLocation), OID_EQUAL);
   AssertEquals(v10.getValueAsInt(), 42);

   SNMP_Variable v12(s_oidSysDescription);
   v12.setValueFromString(ASN_OCTET_STRING, _T("Some long string that exceeds internal value buffer in variable class"));
   v10 = std::move(v12);
   AssertEquals(v10.getName().compare(s_oidSysDescription), OID_EQUAL);
   v10.getValueAsString(buffer, 256);
   AssertEquals(buffer, _T("Some long string that exceeds internal value buffer in variable class"));
   v12.getValueAsString(buffer, 256);
   AssertEquals(static_cast<int32_t>(buffer[0]), 0);  // v12 should be in "empty" state after move
   EndTest();
}

/**
 * Test PDU encoding
 */
static void TestPDUEncoding()
{
   StartTest(_T("SNMP_PDU encoding/decoding"));

   SNMP_PDU pdu(SNMP_GET_REQUEST, SnmpNewRequestId(), SNMP_VERSION_2C);
   pdu.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 17713, 22, 1, 1, 1, 8, 0 }));
   pdu.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 17713, 22, 1, 1, 1, 4, 0 }));
   pdu.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 17713, 22, 1, 1, 1, 5, 0 }));

   SNMP_ObjectId oid { 1, 3, 6, 1, 4, 1, 17713, 22, 1, 2, 1, 4, 1, 1, 1 };
   oid.changeElement(11, 6);
   pdu.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 8);
   pdu.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 2);
   pdu.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 4);
   oid.changeElement(10, 1);
   oid.changeElement(11, 2);
   pdu.bindVariable(new SNMP_Variable(oid));

   SNMP_SecurityContext securityContext("public");
   SNMP_PDUBuffer encodedPDU;
   size_t size = pdu.encode(&encodedPDU, &securityContext);
   AssertTrue(size > 0);

   SNMP_PDU pdu2;
   AssertTrue(pdu2.parse(encodedPDU, size, &securityContext, false));

   AssertEquals(pdu.getCommand(), pdu2.getCommand());
   AssertEquals(pdu2.getCommunity(), "public");
   AssertEquals(pdu.getRequestId(), pdu2.getRequestId());
   AssertEquals(pdu.getNumVariables(), pdu2.getNumVariables());

   for(int i = 0; i < pdu.getNumVariables(); i++)
   {
      SNMP_Variable *v1 = pdu.getVariable(i);
      SNMP_Variable *v2 = pdu2.getVariable(i);
      AssertNotNull(v1);
      AssertNotNull(v2);
      AssertTrue(v1->getName().equals(v2->getName()));
   }

   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   TestOidConversion();
   TestOidClass();
   TestVariableClass();
   TestPDUEncoding();
   return 0;
}
