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
static uint32_t s_longOid[] = { 1, 3, 6, 1, 4, 1, 7562, 84, 748, 256, 12345, 11, 0 };

/**
 * Test OID conversion
 */
static void TestOidConversion()
{
   wchar_t textW[256], smallTextW[16];
   char textA[256], smallTextA[16];
   uint32_t bin[256];

   StartTest(_T("SNMPConvertOIDToTextW"));
   SnmpConvertOIDToTextW(9, s_sysDescription, textW, 256);
   AssertTrue(!wcscmp(textW, L"1.3.6.1.2.1.1.1.0"));
   EndTest();

   StartTest(_T("SNMPConvertOIDToTextW - handling high bit set"));
   SnmpConvertOIDToTextW(9, s_unsignedTest, textW, 256);
   AssertTrue(!wcscmp(textW, L"1.3.6.1.2.1.1.2164260864.0"));
   EndTest();

   StartTest(_T("SNMPConvertOIDToTextW - truncation"));
   SnmpConvertOIDToTextW(14, s_longOid, smallTextW, 16);
   AssertTrue(!wcscmp(smallTextW, L"1.3.6.1.4.1.756"));
   SecureZeroMemory(smallTextW, sizeof(smallTextW));
   SnmpConvertOIDToTextW(14, s_longOid, smallTextW, 13);
   AssertTrue(!wcscmp(smallTextW, L"1.3.6.1.4.1."));
   EndTest();

   StartTest(_T("SNMPConvertOIDToTextA"));
   SnmpConvertOIDToTextA(9, s_sysDescription, textA, 256);
   AssertTrue(!strcmp(textA, "1.3.6.1.2.1.1.1.0"));
   EndTest();

   StartTest(_T("SNMPConvertOIDToTextA - handling high bit set"));
   SnmpConvertOIDToTextA(9, s_unsignedTest, textA, 256);
   AssertTrue(!strcmp(textA, "1.3.6.1.2.1.1.2164260864.0"));
   EndTest();

   StartTest(_T("SNMPConvertOIDToTextA - truncation"));
   SnmpConvertOIDToTextA(14, s_longOid, smallTextA, 16);
   AssertTrue(!strcmp(smallTextA, "1.3.6.1.4.1.756"));
   SecureZeroMemory(smallTextW, sizeof(smallTextA));
   SnmpConvertOIDToTextA(14, s_longOid, smallTextA, 13);
   AssertTrue(!strcmp(smallTextA, "1.3.6.1.4.1."));
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
 * Find byte sequence in buffer
 */
static bool ContainsBytes(const BYTE *buffer, size_t size, const BYTE *pattern, size_t patternSize)
{
   for(size_t i = 0; i + patternSize <= size; i++)
      if (!memcmp(&buffer[i], pattern, patternSize))
         return true;
   return false;
}

/**
 * Test SNMPv1 trap encoding (enterprise field and time-stamp type as defined in RFC 1157 / RFC 2576)
 */
static void TestV1TrapEncoding()
{
   StartTest(_T("SNMPv1 trap encoding/decoding"));

   SNMP_SecurityContext securityContext("public");

   // Enterprise specific trap E.0.N: enterprise = E, specific trap = N, time-stamp as TimeTicks
   SNMP_ObjectId trapId({ 1, 3, 6, 1, 4, 1, 57163, 1, 0, 2 });
   SNMP_PDU pdu(SNMP_TRAP, SNMP_VERSION_1, trapId, 12345, 1);
   pdu.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 57163, 1, 1, 3, 0 }));

   SNMP_PDUBuffer encodedPDU;
   size_t size = pdu.encode(&encodedPDU, &securityContext);
   AssertTrue(size > 0);

   static const BYTE enterprise[] = { 0x06, 0x09, 0x2B, 0x06, 0x01, 0x04, 0x01, 0x83, 0xBE, 0x4B, 0x01, 0x40 };  // OID .1.3.6.1.4.1.57163.1 followed by IpAddress tag
   AssertTrue(ContainsBytes(encodedPDU, size, enterprise, sizeof(enterprise)));
   static const BYTE trapTypes[] = { 0x02, 0x01, 0x06, 0x02, 0x01, 0x02, 0x43, 0x02, 0x30, 0x39 };  // generic 6, specific 2, TimeTicks 12345
   AssertTrue(ContainsBytes(encodedPDU, size, trapTypes, sizeof(trapTypes)));

   SNMP_PDU pdu2;
   AssertTrue(pdu2.parse(encodedPDU, size, &securityContext, false));
   AssertEquals(pdu2.getCommand(), SNMP_TRAP);
   AssertEquals(pdu2.getVersion(), SNMP_VERSION_1);
   AssertEquals(pdu2.getTrapType(), 6);
   AssertEquals(pdu2.getSpecificTrapType(), 2);
   AssertTrue(pdu2.getTrapId().equals(trapId));
   AssertEquals(pdu2.getNumVariables(), 1);

   // Enterprise specific trap without zero before last sub-identifier: enterprise = OID without last sub-identifier
   SNMP_ObjectId trapId2({ 1, 3, 6, 1, 4, 1, 57163, 7 });
   SNMP_PDU pdu3(SNMP_TRAP, SNMP_VERSION_1, trapId2, 1, 2);
   size = pdu3.encode(&encodedPDU, &securityContext);
   AssertTrue(size > 0);
   static const BYTE enterprise2[] = { 0x06, 0x08, 0x2B, 0x06, 0x01, 0x04, 0x01, 0x83, 0xBE, 0x4B, 0x40 };  // OID .1.3.6.1.4.1.57163 followed by IpAddress tag
   AssertTrue(ContainsBytes(encodedPDU, size, enterprise2, sizeof(enterprise2)));
   static const BYTE trapTypes2[] = { 0x02, 0x01, 0x06, 0x02, 0x01, 0x07, 0x43 };  // generic 6, specific 7, TimeTicks tag
   AssertTrue(ContainsBytes(encodedPDU, size, trapTypes2, sizeof(trapTypes2)));

   // Standard trap (linkDown, snmpTraps.3): enterprise = snmpTraps, generic trap = 2, specific trap = 0
   SNMP_ObjectId linkDown({ 1, 3, 6, 1, 6, 3, 1, 1, 5, 3 });
   SNMP_PDU pdu4(SNMP_TRAP, SNMP_VERSION_1, linkDown, 1, 3);
   size = pdu4.encode(&encodedPDU, &securityContext);
   AssertTrue(size > 0);
   static const BYTE snmpTraps[] = { 0x06, 0x08, 0x2B, 0x06, 0x01, 0x06, 0x03, 0x01, 0x01, 0x05, 0x40 };  // .1.3.6.1.6.3.1.1.5 followed by IpAddress tag
   AssertTrue(ContainsBytes(encodedPDU, size, snmpTraps, sizeof(snmpTraps)));
   static const BYTE trapTypes3[] = { 0x02, 0x01, 0x02, 0x02, 0x01, 0x00, 0x43 };  // generic 2, specific 0, TimeTicks tag
   AssertTrue(ContainsBytes(encodedPDU, size, trapTypes3, sizeof(trapTypes3)));

   SNMP_PDU pdu5;
   AssertTrue(pdu5.parse(encodedPDU, size, &securityContext, false));
   AssertEquals(pdu5.getTrapType(), 2);
   AssertEquals(pdu5.getSpecificTrapType(), 0);
   AssertTrue(pdu5.getTrapId().equals(linkDown));

   // Copy of trap PDU must preserve V1 header fields
   SNMP_PDU copy(pdu);
   size = copy.encode(&encodedPDU, &securityContext);
   AssertTrue(size > 0);
   AssertTrue(ContainsBytes(encodedPDU, size, trapTypes, sizeof(trapTypes)));

   EndTest();
}

/**
 * Test SNMPv3 privacy. Number of bound variables is varied so that encoded PDU
 * size hits both exact block multiples and sizes requiring padding.
 */
static void TestPDUPrivacy(SNMP_EncryptionMethod method, const TCHAR *name)
{
   StartTest(name);

   static const BYTE engineId[] = { 0x80, 0x00, 0x1F, 0x88, 0x80, 0x12, 0x34, 0x56, 0x78 };
   SNMP_Engine engine(engineId, sizeof(engineId), 1, 42);

   for(int count = 1; count <= 16; count++)
   {
      SNMP_SecurityContext securityContext("testuser", "authPassword123", "privPassword456", SNMP_AUTH_SHA1, method);
      securityContext.setAuthoritativeEngine(engine);
      securityContext.recalculateKeys();

      SNMP_PDU pdu(SNMP_GET_REQUEST, SnmpNewRequestId(), SNMP_VERSION_3);
      pdu.setContextEngineId(engineId, sizeof(engineId));
      for(int i = 0; i < count; i++)
         pdu.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 1, static_cast<uint32_t>(i + 1), 0 }));

      SNMP_PDUBuffer encodedPDU;
      size_t size = pdu.encode(&encodedPDU, &securityContext);
      AssertTrue(size > 0);

      SNMP_PDU pdu2;
      AssertTrue(pdu2.parse(encodedPDU, size, &securityContext, true));
      AssertEquals(pdu.getNumVariables(), pdu2.getNumVariables());
      for(int i = 0; i < pdu.getNumVariables(); i++)
         AssertTrue(pdu.getVariable(i)->getName().equals(pdu2.getVariable(i)->getName()));
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
   TestV1TrapEncoding();
   TestPDUPrivacy(SNMP_ENCRYPT_DES, _T("SNMPv3 privacy (DES)"));
   TestPDUPrivacy(SNMP_ENCRYPT_AES_128, _T("SNMPv3 privacy (AES-128)"));
   return 0;
}
