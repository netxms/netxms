/*
** NetXMS - Network Management System
** Unit tests for the direct KPI computation provider (kpidirect)
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: test-kpidirect.cpp
**
** Runs the provider's pure functions against kpidirect_vectors.json, generated from the normative
** reference implementation (kpidirect_ref.py). Each vector is run three times: samples, series and
** proxies as listed, fully reversed, and with samples sorted newest first as the server supplies them.
** All three must agree with the expected output within tolerance and with each other exactly.
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcldefs.h>
#include <testtools.h>
#include <netxms-version.h>
#include <kpidirect.h>
#include <algorithm>
#include <set>
#include <tuple>

NETXMS_EXECUTABLE_HEADER(test-kpidirect)

/**
 * Component names and slot tags, in EnergyComponent order
 */
static const char *s_componentNames[ENERGY_COMPONENT_COUNT] = { "EIT", "EDC_GROSS", "EDC_BG", "RES_OS", "HU", "WIN", "WIN_FRE", "REUSE" };
static const char *s_slotTags[ENERGY_COMPONENT_COUNT] = { "eed.eit", "eed.edc", "eed.edc_bg", "eed.res_os", "eed.reuse_upgrade", "eed.win", "eed.win_fre", "eed.reuse" };

/**
 * Diagnostic multiset element: (component, dciId, code)
 */
typedef std::tuple<int, uint32_t, int> DiagnosticKey;
typedef std::multiset<DiagnosticKey> DiagnosticSet;

/**
 * Copy UTF-8 string into fixed wide buffer
 */
static void CopyTag(wchar_t *dst, size_t size, const char *src)
{
   size_t chars = utf8_to_wchar(src, -1, dst, size - 1);
   dst[chars] = 0;
}

/**
 * Component index from slot tag; -1 if not a component slot
 */
static int ComponentFromSlot(const char *slot)
{
   for(int i = 0; i < ENERGY_COMPONENT_COUNT; i++)
      if (!strcmp(slot, s_slotTags[i]))
         return i;
   return -1;
}

/**
 * Diagnostic code from name; -1 if unknown
 */
static int CodeFromName(const char *name)
{
   wchar_t wname[64];
   CopyTag(wname, 64, name);
   static const ComputationDiagnosticCode codes[] =
   {
      ComputationDiagnosticCode::DUPLICATE_TIMESTAMP, ComputationDiagnosticCode::TOO_FEW_SAMPLES, ComputationDiagnosticCode::NON_MEASURED_SAMPLE,
      ComputationDiagnosticCode::NEGATIVE_VALUE, ComputationDiagnosticCode::GAP_NOT_BRIDGED, ComputationDiagnosticCode::COUNTER_DECREASE,
      ComputationDiagnosticCode::IMPLAUSIBLE_INCREMENT, ComputationDiagnosticCode::UNRESOLVED_BOUNDARY, ComputationDiagnosticCode::INTERVAL_WITHOUT_START,
      ComputationDiagnosticCode::INTERVAL_AFTER_GAP, ComputationDiagnosticCode::UNCOVERED_TIME, ComputationDiagnosticCode::INSUFFICIENT_COVERAGE,
      ComputationDiagnosticCode::PROFILE_INVALID, ComputationDiagnosticCode::SLOT_ABSENT, ComputationDiagnosticCode::SLOT_LEVEL_AMBIGUOUS,
      ComputationDiagnosticCode::BINDING_COUNT_MISMATCH, ComputationDiagnosticCode::NO_PRIMARY_BINDINGS, ComputationDiagnosticCode::INPUT_REFUSED,
      ComputationDiagnosticCode::SUBSET_UNRESOLVED, ComputationDiagnosticCode::NEGATIVE_RESULT, ComputationDiagnosticCode::NO_VALID_PROXY,
      ComputationDiagnosticCode::DEDUCTION_WITHOUT_MEASUREMENT, ComputationDiagnosticCode::PROXY_IGNORED_STATE, ComputationDiagnosticCode::PROXY_USED
   };
   for(ComputationDiagnosticCode c : codes)
      if (!wcscmp(wname, DiagnosticCodeName(c)))
         return static_cast<int>(c);
   return -1;
}

/**
 * Slot state from name
 */
static SlotState StateFromName(const char *name)
{
   if (!strcmp(name, "METERED")) return SlotState::METERED;
   if (!strcmp(name, "PROXIED")) return SlotState::PROXIED;
   if (!strcmp(name, "ABSENT")) return SlotState::ABSENT;
   if (!strcmp(name, "NOT_APPLICABLE")) return SlotState::NOT_APPLICABLE;
   if (!strcmp(name, "LEVEL_AMBIGUOUS")) return SlotState::LEVEL_AMBIGUOUS;
   return static_cast<SlotState>(-1);
}

/**
 * Series kind from name
 */
static SeriesKind KindFromName(const char *name)
{
   if (!strcmp(name, "COUNTER")) return SeriesKind::COUNTER;
   if (!strcmp(name, "INTERVAL")) return SeriesKind::INTERVAL;
   return SeriesKind::RATE;
}

/**
 * Sample quality from name
 */
static SampleQuality QualityFromName(const char *name)
{
   if (!strcmp(name, "PROXY")) return SampleQuality::PROXY;
   if (!strcmp(name, "ESTIMATED")) return SampleQuality::ESTIMATED;
   if (!strcmp(name, "INTERPOLATED")) return SampleQuality::INTERPOLATED;
   if (!strcmp(name, "MISSING")) return SampleQuality::MISSING;
   return SampleQuality::MEASURED;
}

/**
 * Summing level name to object class and domain type
 */
static void SummingLevelFromName(const char *name, int32_t *objectClass, int32_t *domainType)
{
   *objectClass = 0;
   *domainType = -1;
   if (name == nullptr)
      return;
   if (!strcmp(name, "FACILITY"))
   {
      *objectClass = OBJECT_FACILITY;
   }
   else if (!strcmp(name, "RACK"))
   {
      *objectClass = OBJECT_RACK;
   }
   else
   {
      *objectClass = OBJECT_POWERDOMAIN;
      if (!strcmp(name, "GRID_ENTRY")) *domainType = POWER_DOMAIN_GRID_ENTRY;
      else if (!strcmp(name, "GENERATOR")) *domainType = POWER_DOMAIN_GENERATOR;
      else if (!strcmp(name, "UPS")) *domainType = POWER_DOMAIN_UPS;
      else if (!strcmp(name, "PDU")) *domainType = POWER_DOMAIN_PDU;
      else if (!strcmp(name, "BUSWAY")) *domainType = POWER_DOMAIN_BUSWAY;
      else *domainType = POWER_DOMAIN_OTHER;
   }
}

/**
 * Input ordering variants
 */
enum class InputOrder
{
   AS_LISTED,
   REVERSED,
   NEWEST_FIRST
};

/**
 * Build provider input from vector
 */
static void BuildInput(json_t *vector, InputOrder order, ComputationInput *input)
{
   input->facilityId = 1;
   input->dayStart = static_cast<time_t>(json_integer_value(json_object_get(vector, "dayStart")));
   input->dayEnd = static_cast<time_t>(json_integer_value(json_object_get(vector, "dayEnd")));
   input->marginSeconds = KPIDIRECT_INPUT_MARGIN;

   const char *slot;
   json_t *entry;
   json_object_foreach(json_object_get(vector, "profile"), slot, entry)
   {
      MeteringProfileEntry e;
      memset(&e, 0, sizeof(e));
      CopyTag(e.slotTag, MAX_DCI_TAG_LENGTH, slot);
      e.component = static_cast<EnergyComponent>(ComponentFromSlot(slot));
      e.state = StateFromName(json_string_value(json_object_get(entry, "state")));
      json_t *expected = json_object_get(entry, "expected");
      e.expectedBindings = json_is_integer(expected) ? static_cast<int32_t>(json_integer_value(expected)) : 0;
      SummingLevelFromName(json_string_value(json_object_get(entry, "summingLevel")), &e.summingClass, &e.summingDomainType);
      json_t *basis = json_object_get(entry, "basis");
      e.measurementBasis = json_is_integer(basis) ? static_cast<int16_t>(json_integer_value(basis)) : -1;
      input->profile.push_back(e);
   }

   json_t *seriesList = json_object_get(vector, "series");
   size_t i;
   json_t *series;
   json_array_foreach(seriesList, i, series)
   {
      ResolvedSeries s;
      memset(&s.binding, 0, sizeof(s.binding));
      const char *seriesSlot = json_string_value(json_object_get(series, "slot"));
      CopyTag(s.binding.slotTag, MAX_DCI_TAG_LENGTH, seriesSlot);
      s.binding.component = static_cast<EnergyComponent>(ComponentFromSlot(seriesSlot));
      s.binding.role = strcmp(json_string_value(json_object_get(series, "role")), "AUXILIARY") ? BindingRole::PRIMARY : BindingRole::AUXILIARY;
      s.binding.kind = KindFromName(json_string_value(json_object_get(series, "kind")));
      s.binding.unitFactor = json_number_value(json_object_get(series, "unitFactor"));
      s.binding.nominalInterval = static_cast<int32_t>(json_integer_value(json_object_get(series, "nominalInterval")));
      s.binding.ratedPower = static_cast<int32_t>(json_integer_value(json_object_get(series, "ratedPower")));
      s.binding.measurementBasis = -1;
      s.binding.dciId = static_cast<uint32_t>(json_integer_value(json_object_get(series, "dciId")));

      size_t j;
      json_t *sample;
      json_array_foreach(json_object_get(series, "samples"), j, sample)
      {
         AttributedSample a;
         a.timestamp = Timestamp::fromTime(static_cast<time_t>(json_integer_value(json_array_get(sample, 0))));
         a.value = json_number_value(json_array_get(sample, 1));
         if (json_array_size(sample) > 2)
            a.attributes.quality = QualityFromName(json_string_value(json_array_get(sample, 2)));
         s.samples.push_back(a);
      }
      if (order == InputOrder::REVERSED)
         std::reverse(s.samples.begin(), s.samples.end());
      else if (order == InputOrder::NEWEST_FIRST)
         std::sort(s.samples.begin(), s.samples.end(), [](const AttributedSample& a, const AttributedSample& b) { return a.timestamp.asMilliseconds() > b.timestamp.asMilliseconds(); });
      input->series.push_back(s);
   }
   if (order == InputOrder::REVERSED)
      std::reverse(input->series.begin(), input->series.end());

   json_t *proxy;
   json_array_foreach(json_object_get(vector, "proxies"), i, proxy)
   {
      ProxyDeclaration p;
      memset(&p, 0, sizeof(p));
      p.id = static_cast<uint32_t>(json_integer_value(json_object_get(proxy, "id")));
      const char *proxySlot = json_string_value(json_object_get(proxy, "slot"));
      CopyTag(p.slotTag, MAX_DCI_TAG_LENGTH, proxySlot);
      p.component = static_cast<EnergyComponent>(ComponentFromSlot(proxySlot));
      p.kind = strcmp(json_string_value(json_object_get(proxy, "kind")), "DEDUCT") ? ProxyKind::ADD : ProxyKind::DEDUCT;
      p.dailyQuantity = json_number_value(json_object_get(proxy, "dailyQuantity"));
      input->proxies.push_back(p);
   }
   if (order == InputOrder::REVERSED)
      std::reverse(input->proxies.begin(), input->proxies.end());
}

/**
 * Compare within tolerance: relative 1e-9, absolute 1e-12 near zero
 */
static bool Close(double a, double b)
{
   if (std::isnan(a) && std::isnan(b))
      return true;
   return fabs(a - b) <= std::max(1e-12, 1e-9 * std::max(fabs(a), fabs(b)));
}

/**
 * Diagnostic multiset of provider output
 */
static DiagnosticSet OutputDiagnostics(const ComputationOutput& output)
{
   DiagnosticSet set;
   for(const ComputationDiagnostic& d : output.diagnostics)
      set.insert(DiagnosticKey(d.component, d.dciId, static_cast<int>(d.code)));
   return set;
}

/**
 * Diagnostic multiset of expected output
 */
static DiagnosticSet ExpectedDiagnostics(json_t *expected)
{
   DiagnosticSet set;
   json_t *components = json_object_get(expected, "components");
   const char *name;
   json_t *component;
   json_object_foreach(components, name, component)
   {
      size_t i;
      json_t *d;
      json_array_foreach(json_object_get(component, "diag"), i, d)
         set.insert(DiagnosticKey(static_cast<int>(json_integer_value(json_object_get(d, "component"))),
                  static_cast<uint32_t>(json_integer_value(json_object_get(d, "dciId"))), CodeFromName(json_string_value(json_object_get(d, "code")))));
   }
   size_t i;
   json_t *d;
   json_array_foreach(json_object_get(expected, "seriesDiagnostics"), i, d)
      set.insert(DiagnosticKey(static_cast<int>(json_integer_value(json_object_get(d, "component"))),
               static_cast<uint32_t>(json_integer_value(json_object_get(d, "dciId"))), CodeFromName(json_string_value(json_object_get(d, "code")))));
   return set;
}

/**
 * Print diagnostic set
 */
static void PrintDiagnostics(const char *label, const DiagnosticSet& set)
{
   _tprintf(_T("      %hs:"), label);
   for(const DiagnosticKey& k : set)
   {
      int code = std::get<2>(k);
      _tprintf(_T(" (%d,%u,%s)"), std::get<0>(k), std::get<1>(k), (code > 0) ? DiagnosticCodeName(static_cast<ComputationDiagnosticCode>(code)) : _T("?"));
   }
   _tprintf(_T("\n"));
}

/**
 * Compare provider output with expected output. Prints every mismatch; returns true if none.
 */
static bool CompareWithExpected(const char *vectorId, const ComputationOutput& output, json_t *expected)
{
   bool ok = true;
   json_t *components = json_object_get(expected, "components");
   for(int i = 0; i < ENERGY_COMPONENT_COUNT; i++)
   {
      json_t *e = json_object_get(components, s_componentNames[i]);
      const ComponentValue& c = output.components[i];

      SampleQuality quality = QualityFromName(json_string_value(json_object_get(e, "quality")));
      if (c.attributes.quality != quality)
      {
         _tprintf(_T("   %hs %hs: quality %d, expected %d\n"), vectorId, s_componentNames[i], static_cast<int>(c.attributes.quality), static_cast<int>(quality));
         ok = false;
      }

      json_t *value = json_object_get(e, "value");
      if (json_is_null(value) ? !std::isnan(c.value) : !Close(c.value, json_number_value(value)))
      {
         _tprintf(_T("   %hs %hs: value %.12g, expected %.12g\n"), vectorId, s_componentNames[i], c.value, json_is_null(value) ? NAN : json_number_value(value));
         ok = false;
      }

      bool bounded = json_is_true(json_object_get(e, "bounded"));
      if (c.attributes.bounded != bounded)
      {
         _tprintf(_T("   %hs %hs: bounded %d, expected %d\n"), vectorId, s_componentNames[i], c.attributes.bounded, bounded);
         ok = false;
      }
      else if (bounded && (!Close(c.attributes.lower, json_number_value(json_object_get(e, "lower"))) || !Close(c.attributes.upper, json_number_value(json_object_get(e, "upper")))))
      {
         _tprintf(_T("   %hs %hs: bounds [%g, %g], expected [%g, %g]\n"), vectorId, s_componentNames[i], c.attributes.lower, c.attributes.upper,
                  json_number_value(json_object_get(e, "lower")), json_number_value(json_object_get(e, "upper")));
         ok = false;
      }

      double completeness = json_number_value(json_object_get(e, "completeness"));
      if (!Close(c.attributes.completeness, completeness))
      {
         _tprintf(_T("   %hs %hs: completeness %.9f, expected %.9f\n"), vectorId, s_componentNames[i], c.attributes.completeness, completeness);
         ok = false;
      }

      if (i == static_cast<int>(EnergyComponent::EIT))
      {
         json_t *basis = json_object_get(e, "eitBasis");
         int lowest = static_cast<int>(json_integer_value(json_object_get(basis, "lowest")));
         if (output.eitBasis.lowest != lowest)
         {
            _tprintf(_T("   %hs EIT: basis lowest %d, expected %d\n"), vectorId, output.eitBasis.lowest, lowest);
            ok = false;
         }
         json_t *share = json_object_get(basis, "share");
         for(int b = 0; b < 4; b++)
         {
            if (!Close(output.eitBasis.share[b], json_number_value(json_array_get(share, b))))
            {
               _tprintf(_T("   %hs EIT: basis share[%d] %.9f, expected %.9f\n"), vectorId, b, output.eitBasis.share[b], json_number_value(json_array_get(share, b)));
               ok = false;
            }
         }
      }
   }

   DiagnosticSet actual = OutputDiagnostics(output);
   DiagnosticSet wanted = ExpectedDiagnostics(expected);
   if (actual != wanted)
   {
      _tprintf(_T("   %hs: diagnostics differ\n"), vectorId);
      PrintDiagnostics("actual  ", actual);
      PrintDiagnostics("expected", wanted);
      ok = false;
   }
   return ok;
}

/**
 * Compare two outputs for exact equality (determinism under input reordering)
 */
static bool SameOutput(const char *vectorId, const char *variant, const ComputationOutput& a, const ComputationOutput& b)
{
   bool ok = true;
   for(int i = 0; i < ENERGY_COMPONENT_COUNT; i++)
   {
      const ComponentValue& x = a.components[i];
      const ComponentValue& y = b.components[i];
      bool sameValue = (std::isnan(x.value) && std::isnan(y.value)) || (memcmp(&x.value, &y.value, sizeof(double)) == 0);
      if (!sameValue || (x.attributes.quality != y.attributes.quality) || (x.attributes.bounded != y.attributes.bounded) ||
          (memcmp(&x.attributes.completeness, &y.attributes.completeness, sizeof(double)) != 0))
      {
         _tprintf(_T("   %hs %hs: %hs input gives different result (%.17g vs %.17g)\n"), vectorId, s_componentNames[i], variant, x.value, y.value);
         ok = false;
      }
   }
   if ((a.eitBasis.lowest != b.eitBasis.lowest) || (memcmp(a.eitBasis.share, b.eitBasis.share, sizeof(a.eitBasis.share)) != 0))
   {
      _tprintf(_T("   %hs EIT: %hs input gives different basis\n"), vectorId, variant);
      ok = false;
   }
   if (OutputDiagnostics(a) != OutputDiagnostics(b))
   {
      _tprintf(_T("   %hs: %hs input gives different diagnostics\n"), vectorId, variant);
      ok = false;
   }
   return ok;
}

/**
 * Run one vector
 */
static void TestVector(json_t *vector)
{
   const char *id = json_string_value(json_object_get(vector, "id"));
   const char *title = json_string_value(json_object_get(vector, "title"));
   TCHAR name[256];
   _sntprintf(name, 256, _T("%hs: %hs"), id, title);
   StartTest(name);

   json_t *expected = json_object_get(vector, "expected");

   ComputationInput input;
   BuildInput(vector, InputOrder::AS_LISTED, &input);
   ComputationOutput output;
   ComposeComponents(input, &output);
   bool ok = CompareWithExpected(id, output, expected);

   ComputationInput reversedInput;
   BuildInput(vector, InputOrder::REVERSED, &reversedInput);
   ComputationOutput reversedOutput;
   ComposeComponents(reversedInput, &reversedOutput);
   ok = SameOutput(id, "reversed", output, reversedOutput) && ok;

   ComputationInput newestFirstInput;
   BuildInput(vector, InputOrder::NEWEST_FIRST, &newestFirstInput);
   ComputationOutput newestFirstOutput;
   ComposeComponents(newestFirstInput, &newestFirstOutput);
   ok = SameOutput(id, "newest-first", output, newestFirstOutput) && ok;

   AssertTrue(ok);
   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   const char *path = (argc > 1) ? argv[1] : "kpidirect_vectors.json";
   json_error_t error;
   json_t *root = json_load_file(path, 0, &error);
   if (root == nullptr)
   {
      _tprintf(_T("Cannot load test vectors from %hs: %hs (line %d)\n"), path, error.text, error.line);
      return 1;
   }

   _tprintf(_T("Method version %hs, %d vectors, %d classification entries (classification belongs to binding resolution and is not tested here)\n\n"),
            json_string_value(json_object_get(root, "methodVersion")), static_cast<int>(json_array_size(json_object_get(root, "vectors"))),
            static_cast<int>(json_array_size(json_object_get(root, "classification"))));

   size_t i;
   json_t *vector;
   json_array_foreach(json_object_get(root, "vectors"), i, vector)
      TestVector(vector);

   json_decref(root);
   InitiateProcessShutdown();
   return 0;
}
