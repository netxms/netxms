/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxkpi.h
**
**/

#ifndef _nxkpi_h_
#define _nxkpi_h_

#include <nms_util.h>
#include <nxcldefs.h>
#include <math.h>

/**
 * Quality class of a sample. Numeric order is the degradation order (worst-of = max).
 */
enum class SampleQuality : int16_t
{
   MEASURED = 0,
   PROXY = 1,
   ESTIMATED = 2,
   INTERPOLATED = 3,
   MISSING = 4
};

/**
 * Per-sample attributes (provenance of a single DCI history sample)
 */
struct SampleAttributes
{
   SampleQuality quality;
   bool bounded;          // false: [lower, upper] undefined
   double lower;
   double upper;
   double completeness;   // 0..1
   uint32_t methodId;     // kpi_methods.id; 0 = none

   /**
    * Default attributes: measured, unbounded, complete, no method. Samples with default attributes have no stored attribute record.
    */
   SampleAttributes()
   {
      quality = SampleQuality::MEASURED;
      bounded = false;
      lower = 0;
      upper = 0;
      completeness = 1;
      methodId = 0;
   }

   /**
    * Check if attributes are default ones
    */
   bool isDefault() const
   {
      return (quality == SampleQuality::MEASURED) && !bounded && (completeness == 1) && (methodId == 0);
   }
};

/**
 * DCI history sample with attributes. Value is NaN for samples with quality MISSING.
 */
struct AttributedSample
{
   Timestamp timestamp;
   double value;
   SampleAttributes attributes;
};

/**
 * Computation method tier
 */
enum class MethodTier : char
{
   OPEN = '0',
   COMMERCIAL = '1'
};

/**
 * Computation method descriptor. Method is identified by method ID and version.
 */
struct MethodDescriptor
{
   const wchar_t *methodId;     // e.g. "kpidirect"
   const wchar_t *version;
   const wchar_t *engine;
   const wchar_t *displayName;
   MethodTier tier;
   double coverageLevel;        // coverage level of bounds produced by this method (e.g. 0.95)
};

/**
 * Energy and water components computed per facility per local day. Numeric values are stored in daily records and must not change.
 */
enum class EnergyComponent : int16_t
{
   EIT = 0,          // IT equipment energy (eed.eit)
   EDC_GROSS = 1,    // Gross data centre energy supply (eed.edc)
   EDC_BG = 2,       // Back-up generator energy, subset of EDC_GROSS (eed.edc_bg)
   RES_OS = 3,       // On-site renewable energy, subset of EDC_GROSS (eed.res_os)
   HU = 4,           // Heat upgrade energy (eed.reuse_upgrade)
   WIN = 5,          // Water input (eed.win)
   WIN_FRE = 6,      // Freshwater input, subset of WIN (eed.win_fre)
   REUSE = 7         // Reused energy (eed.reuse)
};

constexpr int ENERGY_COMPONENT_COUNT = 8;

/**
 * Interpretation of a bound series, derived by binding resolution from the DCI's unit and delta calculation method
 */
enum class SeriesKind : int16_t
{
   RATE = 0,         // power or flow; integrated over time
   COUNTER = 1,      // cumulative energy or volume register; differences are used
   INTERVAL = 2      // energy or volume since previous sample (delta calculation "simple")
};

/**
 * Role of a binding within its slot. Only PRIMARY bindings (those at the slot's summing level) are summed.
 */
enum class BindingRole : int16_t
{
   PRIMARY = 0,
   AUXILIARY = 1
};

/**
 * Effective state of a slot in facility's metering profile. Values 0..3 are stored in kpi_metering_profiles;
 * LEVEL_AMBIGUOUS is derived by binding resolution when bindings sit at several levels and no summing level is declared.
 */
enum class SlotState : int16_t
{
   METERED = 0,
   PROXIED = 1,
   ABSENT = 2,
   NOT_APPLICABLE = 3,
   LEVEL_AMBIGUOUS = 4
};

/**
 * Binding of a DCI to a slot, resolved and classified by the engine. Value snapshot: all fields are plain data.
 */
struct ResolvedBinding
{
   wchar_t slotTag[MAX_DCI_TAG_LENGTH];   // slot tag without qualifier, e.g. "eed.eit"
   wchar_t qualifier[16];                 // feed qualifier ("A" from "eed.eit/A"); empty if none
   EnergyComponent component;
   BindingRole role;
   SeriesKind kind;
   double unitFactor;          // stored value * unitFactor = canonical unit (kW, kWh, m3/h, m3)
   int32_t nominalInterval;    // seconds; 0 for event-driven origins (push, OTLP, imported)
   int32_t ratedPower;         // W, rated power of binding object; 0 = undeclared
   int16_t measurementBasis;   // eed.eit only: 0..3 (rack 3, PDU/busway 2, UPS 1, other 0); -1 otherwise
   uint32_t dciId;
   uint32_t ownerId;           // object owning the DCI
   uint32_t bindingObjectId;   // facility, power domain, cooling zone, or rack the series is bound to
   int32_t bindingObjectClass;
};

/**
 * Bound series with its samples over [dayStart - margin, dayEnd + margin], newest first as returned by ReadAttributedSamples()
 */
struct ResolvedSeries
{
   ResolvedBinding binding;
   std::vector<AttributedSample> samples;
};

/**
 * Effective metering profile entry for one slot. Summing level and measurement basis are filled by the engine
 * from the declared profile or, when undeclared, from the single level at which the slot is bound.
 */
struct MeteringProfileEntry
{
   wchar_t slotTag[MAX_DCI_TAG_LENGTH];
   EnergyComponent component;
   SlotState state;
   int32_t expectedBindings;   // PRIMARY bindings expected at the summing level; 0 = unspecified (invalid for METERED)
   int32_t summingClass;       // object class of the summing level; 0 = unspecified
   int32_t summingDomainType;  // PowerDomainType when summingClass is power domain; -1 otherwise
   int16_t measurementBasis;   // eed.eit only: basis of the summing level; -1 otherwise
};

/**
 * Kind of operator-declared static proxy
 */
enum class ProxyKind : char
{
   ADD = 'A',        // flow the slot should contain but no meter sees
   DEDUCT = 'D'      // flow a meter sees but the slot must exclude
};

/**
 * Operator-declared static proxy in force on the computed day (kpi_proxy_declarations)
 */
struct ProxyDeclaration
{
   uint32_t id;
   wchar_t slotTag[MAX_DCI_TAG_LENGTH];
   EnergyComponent component;
   ProxyKind kind;
   double dailyQuantity;       // canonical unit (kWh or m3) per 24 h, >= 0
};

/**
 * Fully materialized provider input for one facility and one local day
 */
struct ComputationInput
{
   uint32_t facilityId;
   time_t dayStart;            // UTC bound of the facility-local day (inclusive)
   time_t dayEnd;              // UTC bound of the facility-local day (exclusive)
   int32_t marginSeconds;      // samples are supplied over [dayStart - marginSeconds, dayEnd + marginSeconds]
   std::vector<ResolvedSeries> series;
   std::vector<MeteringProfileEntry> profile;
   std::vector<ProxyDeclaration> proxies;   // declarations in force on this day only
};

/**
 * Computed value of one component
 */
struct ComponentValue
{
   double value;               // canonical unit; NaN if quality is MISSING
   SampleAttributes attributes;
};

/**
 * EIT measurement basis
 */
struct EitBasis
{
   int16_t lowest;             // lowest basis among inputs actually used; -1 if EIT is MISSING
   double share[4];            // energy share at basis 0..3, summing to 1 (all zero if EIT is MISSING or zero)
};

/**
 * Diagnostic codes reported by computation providers. Numeric values are stored with daily records and must not change.
 * Codes below 100 are series-level, others are component-level.
 */
enum class ComputationDiagnosticCode : int16_t
{
   DUPLICATE_TIMESTAMP = 1,
   TOO_FEW_SAMPLES = 2,
   NON_MEASURED_SAMPLE = 3,
   NEGATIVE_VALUE = 4,
   GAP_NOT_BRIDGED = 5,
   COUNTER_DECREASE = 6,
   IMPLAUSIBLE_INCREMENT = 7,
   UNRESOLVED_BOUNDARY = 8,
   INTERVAL_WITHOUT_START = 9,
   INTERVAL_AFTER_GAP = 10,
   UNCOVERED_TIME = 11,
   INSUFFICIENT_COVERAGE = 12,

   PROFILE_INVALID = 101,
   SLOT_ABSENT = 102,
   SLOT_LEVEL_AMBIGUOUS = 103,
   BINDING_COUNT_MISMATCH = 104,
   INPUT_REFUSED = 105,
   SUBSET_UNRESOLVED = 106,
   NEGATIVE_RESULT = 107,
   NO_VALID_PROXY = 108,
   DEDUCTION_WITHOUT_MEASUREMENT = 109,
   PROXY_IGNORED_STATE = 110,
   PROXY_USED = 111
};

/**
 * Diagnostic entry explaining a refusal or a degradation. Series-level entries have component -1 and a DCI ID;
 * component-level entries name the component and have dciId 0.
 */
struct ComputationDiagnostic
{
   int16_t component;          // EnergyComponent or -1
   uint32_t dciId;
   ComputationDiagnosticCode code;
   time_t from;                // affected interval or sample time; 0 if not applicable
   time_t to;
   double detail;              // code-specific (coverage, uncovered seconds, proxy ID, subset component, actual count); NaN if none
};

/**
 * Provider output. Constructed with all components MISSING, so a provider that leaves a component untouched has it recorded as such.
 */
struct ComputationOutput
{
   ComponentValue components[ENERGY_COMPONENT_COUNT];
   EitBasis eitBasis;
   std::vector<ComputationDiagnostic> diagnostics;

   ComputationOutput()
   {
      for(int i = 0; i < ENERGY_COMPONENT_COUNT; i++)
      {
         components[i].value = NAN;
         components[i].attributes.quality = SampleQuality::MISSING;
         components[i].attributes.completeness = 0;
      }
      eitBasis.lowest = -1;
      memset(eitBasis.share, 0, sizeof(eitBasis.share));
   }
};

/**
 * Computation provider interface. Implemented by server modules and registered with RegisterComputationProvider().
 * Input is a value snapshot and compute() must be deterministic: identical input and version must give identical output.
 */
class ComputationProvider
{
public:
   virtual ~ComputationProvider() = default;

   virtual const wchar_t *id() const = 0;            // method ID, e.g. "kpidirect"
   virtual const wchar_t *version() const = 0;       // method version; any behaviour change is a new version
   virtual const wchar_t *displayName() const = 0;
   virtual bool isCommercial() const = 0;
   virtual double coverageLevel() const { return 0.95; }    // coverage level of bounds produced; 0 if no bounds are produced
   virtual int32_t inputMarginSeconds() const { return 3600; }
   virtual void compute(const ComputationInput& input, ComputationOutput *output) = 0;
};

#endif   /* _nxkpi_h_ */
