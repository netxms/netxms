#include <nms_common.h>
#include <nms_util.h>
#include <nxsrvapi.h>
#include <testtools.h>

/**
 * Buffer size for versions in tests
 */
#define VERSION_BUFFER_SIZE 64

/**
 * Get wide string property of test vector
 */
static String GetVectorString(json_t *vector, const char *tag)
{
   json_t *value = json_object_get(vector, tag);
   AssertTrueEx(json_is_string(value), L"Test vector property is missing or is not a string");
   return json_object_get_string(vector, tag, L"");
}

/**
 * Get version scheme of test vector
 */
static FirmwareVersionScheme GetVectorScheme(json_t *vector)
{
   FirmwareVersionScheme scheme;
   AssertTrueEx(ParseFirmwareVersionSchemeName(GetVectorString(vector, "scheme"), &scheme), L"Unknown version scheme in test vector");
   return scheme;
}

/**
 * Convert comparison result from test vector form
 */
static FirmwareVersionOrder ParseExpectedOrder(const String& s)
{
   if (s.equals(L"<"))
      return FirmwareVersionOrder::LESS;
   if (s.equals(L"="))
      return FirmwareVersionOrder::EQUAL;
   if (s.equals(L">"))
      return FirmwareVersionOrder::GREATER;
   AssertTrueEx(s.equals(L"~"), L"Invalid comparison result in test vector");
   return FirmwareVersionOrder::INCOMPARABLE;
}

/**
 * Get result expected when compared versions are swapped
 */
static FirmwareVersionOrder ReverseOrder(FirmwareVersionOrder order)
{
   if (order == FirmwareVersionOrder::LESS)
      return FirmwareVersionOrder::GREATER;
   if (order == FirmwareVersionOrder::GREATER)
      return FirmwareVersionOrder::LESS;
   return order;
}

/**
 * Report failed test vector and exit
 */
static void FailVector(const wchar_t *section, size_t index, json_t *vector)
{
   char *text = json_dumps(vector, JSON_COMPACT);
   wchar_t message[1024];
   nx_swprintf(message, 1024, L"Test vector %s[%d] failed: %hs", section, static_cast<int>(index), text);
   MemFree(text);
   AssertTrueEx(false, message);
}

/**
 * Run comparison vectors
 */
static void TestComparisonVectors(json_t *vectors)
{
   StartTest(L"Firmware versions - comparison vectors");
   AssertTrue(json_array_size(vectors) > 0);

   size_t i;
   json_t *vector;
   json_array_foreach(vectors, i, vector)
   {
      FirmwareVersionScheme scheme = GetVectorScheme(vector);
      String a = GetVectorString(vector, "a");
      String b = GetVectorString(vector, "b");
      FirmwareVersionOrder expected = ParseExpectedOrder(GetVectorString(vector, "result"));
      if ((CompareFirmwareVersions(scheme, a, b) != expected) || (CompareFirmwareVersions(scheme, b, a) != ReverseOrder(expected)))
         FailVector(L"comparison", i, vector);

      // Every valid version is equal to itself, invalid version is incomparable with anything
      if (CompareFirmwareVersions(scheme, a, a) != (IsValidFirmwareVersion(scheme, a) ? FirmwareVersionOrder::EQUAL : FirmwareVersionOrder::INCOMPARABLE))
         FailVector(L"comparison", i, vector);
      if (CompareFirmwareVersions(scheme, b, b) != (IsValidFirmwareVersion(scheme, b) ? FirmwareVersionOrder::EQUAL : FirmwareVersionOrder::INCOMPARABLE))
         FailVector(L"comparison", i, vector);
   }
   EndTest();
}

/**
 * Run canonical form vectors
 */
static void TestValidityVectors(json_t *vectors)
{
   StartTest(L"Firmware versions - canonical form vectors");
   AssertTrue(json_array_size(vectors) > 0);

   size_t i;
   json_t *vector;
   json_array_foreach(vectors, i, vector)
   {
      FirmwareVersionScheme scheme = GetVectorScheme(vector);
      String version = GetVectorString(vector, "version");
      json_t *valid = json_object_get(vector, "valid");
      AssertTrueEx(json_is_boolean(valid), L"Property \"valid\" is missing or is not a boolean");
      if (IsValidFirmwareVersion(scheme, version) != json_is_true(valid))
         FailVector(L"validity", i, vector);
   }
   EndTest();
}

/**
 * Run normalization vectors
 */
static void TestNormalizationVectors(json_t *vectors)
{
   StartTest(L"Firmware versions - normalization vectors");
   AssertTrue(json_array_size(vectors) > 0);

   size_t i;
   json_t *vector;
   json_array_foreach(vectors, i, vector)
   {
      FirmwareVersionScheme scheme = GetVectorScheme(vector);
      String raw = GetVectorString(vector, "raw");

      wchar_t version[VERSION_BUFFER_SIZE] = L"unchanged";
      bool evolved = false;
      bool success;
      switch(scheme)
      {
         case FirmwareVersionScheme::EXACT:
            success = NormalizeExactFirmwareVersion(raw, version, VERSION_BUFFER_SIZE);
            break;
         case FirmwareVersionScheme::INTDOT:
            success = NormalizeIntDotFirmwareVersion(raw, version, VERSION_BUFFER_SIZE);
            break;
         case FirmwareVersionScheme::JUNOS:
            success = NormalizeJunosFirmwareVersion(raw, version, VERSION_BUFFER_SIZE, &evolved);
            break;
         case FirmwareVersionScheme::ROUTEROS:
            success = NormalizeRouterOSFirmwareVersion(raw, version, VERSION_BUFFER_SIZE);
            break;
         default:
            AssertTrueEx(false, L"Test vector for scheme without normalizer");
            return;
      }

      json_t *expected = json_object_get(vector, "version");
      if (json_is_null(expected))
      {
         if (success || (version[0] != 0) || evolved)
            FailVector(L"normalization", i, vector);
         continue;
      }

      if (!success || wcscmp(version, GetVectorString(vector, "version")) || !IsValidFirmwareVersion(scheme, version))
         FailVector(L"normalization", i, vector);

      // Platform named by the vector must be registered with the same scheme
      json_t *platformId = json_object_get(vector, "platform");
      if (platformId != nullptr)
      {
         String id = GetVectorString(vector, "platform");
         const FirmwarePlatform *platform = FindFirmwarePlatform(id);
         if ((platform == nullptr) || (platform->scheme != scheme))
            FailVector(L"normalization", i, vector);
         if ((scheme == FirmwareVersionScheme::JUNOS) && (evolved != id.equals(L"junos-evo")))
            FailVector(L"normalization", i, vector);
      }
   }
   EndTest();
}

/**
 * Test firmware version schemes, normalizers and platform registry. Shared test vectors are loaded from given file.
 */
void TestFirmwareVersions(const char *vectorFile)
{
   StartTest(L"Firmware versions - scheme names");
   FirmwareVersionScheme scheme;
   AssertTrue(ParseFirmwareVersionSchemeName(L"exact", &scheme));
   AssertTrue(scheme == FirmwareVersionScheme::EXACT);
   AssertTrue(ParseFirmwareVersionSchemeName(L"intdot", &scheme));
   AssertTrue(scheme == FirmwareVersionScheme::INTDOT);
   AssertTrue(ParseFirmwareVersionSchemeName(L"semver", &scheme));
   AssertTrue(scheme == FirmwareVersionScheme::SEMVER);
   AssertTrue(ParseFirmwareVersionSchemeName(L"junos", &scheme));
   AssertTrue(scheme == FirmwareVersionScheme::JUNOS);
   AssertTrue(ParseFirmwareVersionSchemeName(L"routeros", &scheme));
   AssertTrue(scheme == FirmwareVersionScheme::ROUTEROS);
   AssertFalse(ParseFirmwareVersionSchemeName(L"JUNOS", &scheme));
   AssertFalse(ParseFirmwareVersionSchemeName(L"comware", &scheme));
   AssertFalse(ParseFirmwareVersionSchemeName(L"", &scheme));
   EndTest();

   StartTest(L"Firmware versions - platform registry");
   const FirmwarePlatform *platform = FindFirmwarePlatform(L"junos");
   AssertNotNull(platform);
   AssertEquals(platform->id, L"junos");
   AssertEquals(platform->vendor, L"juniper");
   AssertEquals(platform->name, L"Junos OS");
   AssertTrue(platform->scheme == FirmwareVersionScheme::JUNOS);
   platform = FindFirmwarePlatform(L"junos-evo");
   AssertNotNull(platform);
   AssertTrue(platform->scheme == FirmwareVersionScheme::JUNOS);
   platform = FindFirmwarePlatform(L"routerboot");
   AssertNotNull(platform);
   AssertEquals(platform->vendor, L"mikrotik");
   AssertTrue(platform->scheme == FirmwareVersionScheme::ROUTEROS);
   platform = FindFirmwarePlatform(L"ios-xe");
   AssertNotNull(platform);
   AssertTrue(platform->scheme == FirmwareVersionScheme::EXACT);
   platform = FindFirmwarePlatform(L"cip-device");
   AssertNotNull(platform);
   AssertEquals(platform->vendor, L"");
   AssertTrue(platform->scheme == FirmwareVersionScheme::INTDOT);
   AssertNull(FindFirmwarePlatform(L"JunOS"));
   AssertNull(FindFirmwarePlatform(L"juno"));
   AssertNull(FindFirmwarePlatform(L""));
   EndTest();

   StartTest(L"Firmware versions - normalizer output buffer");
   wchar_t version[VERSION_BUFFER_SIZE];
   bool evolved;
   AssertTrue(NormalizeJunosFirmwareVersion(L"21.4R3-S5.4", version, 12, &evolved));   // exact fit
   AssertEquals(version, L"21.4R3-S5.4");
   AssertFalse(NormalizeJunosFirmwareVersion(L"21.4R3-S5.4", version, 11, &evolved));
   AssertEquals(version, L"");
   AssertFalse(NormalizeJunosFirmwareVersion(L"22.4R3-S2-EVO", version, 9, &evolved));
   AssertFalse(evolved);
   AssertTrue(NormalizeRouterOSFirmwareVersion(L"7.16.2 (stable)", version, 7));
   AssertEquals(version, L"7.16.2");
   AssertFalse(NormalizeRouterOSFirmwareVersion(L"7.16.2 (stable)", version, 6));
   AssertEquals(version, L"");
   AssertTrue(NormalizeIntDotFirmwareVersion(L"FL.10.13.1080", version, 11));
   AssertEquals(version, L"10.13.1080");
   AssertFalse(NormalizeIntDotFirmwareVersion(L"FL.10.13.1080", version, 10));
   AssertEquals(version, L"");
   AssertTrue(NormalizeExactFirmwareVersion(L" 17.9.4a ", version, 8));
   AssertEquals(version, L"17.9.4a");
   AssertFalse(NormalizeExactFirmwareVersion(L" 17.9.4a ", version, 7));
   AssertEquals(version, L"");
   EndTest();

   json_error_t error;
   json_t *root = json_load_file(vectorFile, 0, &error);
   if (root == nullptr)
   {
      wchar_t message[1024];
      nx_swprintf(message, 1024, L"Cannot load firmware version test vectors from %hs: %hs (line %d)", vectorFile, error.text, error.line);
      AssertTrueEx(false, message);
   }

   TestComparisonVectors(json_object_get(root, "comparison"));
   TestValidityVectors(json_object_get(root, "validity"));
   TestNormalizationVectors(json_object_get(root, "normalization"));

   json_decref(root);
}
