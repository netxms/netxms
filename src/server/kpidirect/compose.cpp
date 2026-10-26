/*
** NetXMS - Network Management System
** Direct KPI computation provider
** Copyright (C) 2026 Raden Solutions
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
** File: compose.cpp
**
** Composition of the eight components from integrated PRIMARY series and operator-declared static proxies.
** Pure functions over the provider input types; the result must not depend on the order of series, samples
** or proxies in the input.
**/

#include "kpidirect.h"
#include <algorithm>
#include <map>

/**
 * Diagnostic code names
 */
const wchar_t *DiagnosticCodeName(ComputationDiagnosticCode code)
{
   switch(code)
   {
      case ComputationDiagnosticCode::DUPLICATE_TIMESTAMP: return L"DUPLICATE_TIMESTAMP";
      case ComputationDiagnosticCode::TOO_FEW_SAMPLES: return L"TOO_FEW_SAMPLES";
      case ComputationDiagnosticCode::NON_MEASURED_SAMPLE: return L"NON_MEASURED_SAMPLE";
      case ComputationDiagnosticCode::NEGATIVE_VALUE: return L"NEGATIVE_VALUE";
      case ComputationDiagnosticCode::GAP_NOT_BRIDGED: return L"GAP_NOT_BRIDGED";
      case ComputationDiagnosticCode::COUNTER_DECREASE: return L"COUNTER_DECREASE";
      case ComputationDiagnosticCode::IMPLAUSIBLE_INCREMENT: return L"IMPLAUSIBLE_INCREMENT";
      case ComputationDiagnosticCode::UNRESOLVED_BOUNDARY: return L"UNRESOLVED_BOUNDARY";
      case ComputationDiagnosticCode::INTERVAL_WITHOUT_START: return L"INTERVAL_WITHOUT_START";
      case ComputationDiagnosticCode::INTERVAL_AFTER_GAP: return L"INTERVAL_AFTER_GAP";
      case ComputationDiagnosticCode::UNCOVERED_TIME: return L"UNCOVERED_TIME";
      case ComputationDiagnosticCode::INSUFFICIENT_COVERAGE: return L"INSUFFICIENT_COVERAGE";
      case ComputationDiagnosticCode::PROFILE_INVALID: return L"PROFILE_INVALID";
      case ComputationDiagnosticCode::SLOT_ABSENT: return L"SLOT_ABSENT";
      case ComputationDiagnosticCode::SLOT_LEVEL_AMBIGUOUS: return L"SLOT_LEVEL_AMBIGUOUS";
      case ComputationDiagnosticCode::BINDING_COUNT_MISMATCH: return L"BINDING_COUNT_MISMATCH";
      case ComputationDiagnosticCode::NO_PRIMARY_BINDINGS: return L"NO_PRIMARY_BINDINGS";
      case ComputationDiagnosticCode::INPUT_REFUSED: return L"INPUT_REFUSED";
      case ComputationDiagnosticCode::SUBSET_UNRESOLVED: return L"SUBSET_UNRESOLVED";
      case ComputationDiagnosticCode::NEGATIVE_RESULT: return L"NEGATIVE_RESULT";
      case ComputationDiagnosticCode::NO_VALID_PROXY: return L"NO_VALID_PROXY";
      case ComputationDiagnosticCode::DEDUCTION_WITHOUT_MEASUREMENT: return L"DEDUCTION_WITHOUT_MEASUREMENT";
      case ComputationDiagnosticCode::PROXY_IGNORED_STATE: return L"PROXY_IGNORED_STATE";
      case ComputationDiagnosticCode::PROXY_USED: return L"PROXY_USED";
   }
   return L"UNKNOWN";
}

/**
 * Subset relation: parent component -> subsets in table order, with the rule whether an ABSENT subset refuses the parent.
 * Generator and on-site renewable are separate supply points, so a gross supply that does not include them is incomplete;
 * freshwater is a classification of water that can be metered under eed.win, so an undeclared one does not refuse WIN.
 */
struct SubsetRule
{
   EnergyComponent subset;
   bool absentRefusesParent;
};

static const SubsetRule s_edcSubsets[] = { { EnergyComponent::EDC_BG, true }, { EnergyComponent::RES_OS, true } };
static const SubsetRule s_winSubsets[] = { { EnergyComponent::WIN_FRE, false } };

/**
 * Composed component with its diagnostics and, for EIT, the measurement basis
 */
struct ComposedComponent
{
   bool done;
   ComponentValue value;
   EitBasis basis;
   std::vector<ComputationDiagnostic> diagnostics;
};

/**
 * Composition state for one provider input
 */
class Composition
{
private:
   const ComputationInput& m_input;
   double m_dayLength;
   std::vector<const ResolvedSeries*> m_primarySeries;                 // ascending DCI ID
   std::map<uint32_t, SeriesIntegrationResult> m_integrated;           // by DCI ID
   std::vector<const ProxyDeclaration*> m_proxies;                     // ascending proxy ID
   ComposedComponent m_components[ENERGY_COMPONENT_COUNT];
   std::vector<ComputationDiagnostic> m_seriesDiagnostics;

   ComposedComponent& component(EnergyComponent c) { return m_components[static_cast<int>(c)]; }
   MeteringProfileEntry profileEntry(EnergyComponent c) const;
   void proxiesFor(EnergyComponent c, std::vector<const ProxyDeclaration*> *list) const;
   void addDiagnostic(std::vector<ComputationDiagnostic> *diagnostics, EnergyComponent c, ComputationDiagnosticCode code, double detail = NAN) const;
   void setMissing(EnergyComponent c, ComputationDiagnosticCode code, double completeness = 0, double detail = NAN);
   void compose(EnergyComponent c);
   void composeMetered(EnergyComponent c, const MeteringProfileEntry& entry, const std::vector<const ProxyDeclaration*>& proxies);

public:
   Composition(const ComputationInput& input);

   void run(ComputationOutput *output);
};

/**
 * Constructor: order series and proxies, integrate every PRIMARY series once
 */
Composition::Composition(const ComputationInput& input) : m_input(input)
{
   m_dayLength = static_cast<double>(input.dayEnd - input.dayStart);

   for(const ResolvedSeries& s : input.series)
      if (s.binding.role == BindingRole::PRIMARY)
         m_primarySeries.push_back(&s);
   std::sort(m_primarySeries.begin(), m_primarySeries.end(), [](const ResolvedSeries *a, const ResolvedSeries *b) { return a->binding.dciId < b->binding.dciId; });

   for(const ProxyDeclaration& p : input.proxies)
      m_proxies.push_back(&p);
   std::sort(m_proxies.begin(), m_proxies.end(), [](const ProxyDeclaration *a, const ProxyDeclaration *b) { return a->id < b->id; });

   for(const ResolvedSeries *s : m_primarySeries)
      m_integrated[s->binding.dciId] = IntegrateSeries(*s, input.dayStart, input.dayEnd, &m_seriesDiagnostics);

   for(int i = 0; i < ENERGY_COMPONENT_COUNT; i++)
      m_components[i].done = false;
}

/**
 * Effective profile entry for component. A slot without an entry is ABSENT.
 */
MeteringProfileEntry Composition::profileEntry(EnergyComponent c) const
{
   for(const MeteringProfileEntry& e : m_input.profile)
      if (e.component == c)
         return e;

   MeteringProfileEntry absent;
   memset(&absent, 0, sizeof(absent));
   absent.component = c;
   absent.state = SlotState::ABSENT;
   absent.summingDomainType = -1;
   absent.measurementBasis = -1;
   return absent;
}

/**
 * Proxies declared on the component's slot, ascending ID
 */
void Composition::proxiesFor(EnergyComponent c, std::vector<const ProxyDeclaration*> *list) const
{
   for(const ProxyDeclaration *p : m_proxies)
      if (p->component == c)
         list->push_back(p);
}

/**
 * Add component-level diagnostic
 */
void Composition::addDiagnostic(std::vector<ComputationDiagnostic> *diagnostics, EnergyComponent c, ComputationDiagnosticCode code, double detail) const
{
   ComputationDiagnostic d;
   d.component = static_cast<int16_t>(c);
   d.dciId = 0;
   d.code = code;
   d.from = 0;
   d.to = 0;
   d.detail = detail;
   diagnostics->push_back(d);
}

/**
 * Record component as MISSING with given reason. Diagnostics already collected for the component are kept.
 */
void Composition::setMissing(EnergyComponent c, ComputationDiagnosticCode code, double completeness, double detail)
{
   ComposedComponent& cc = component(c);
   cc.value.value = NAN;
   cc.value.attributes = SampleAttributes();
   cc.value.attributes.quality = SampleQuality::MISSING;
   cc.value.attributes.completeness = completeness;
   cc.basis.lowest = -1;
   memset(cc.basis.share, 0, sizeof(cc.basis.share));
   addDiagnostic(&cc.diagnostics, c, code, detail);
   cc.done = true;
   nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 4, L"Facility %u day %lld: component %d refused (%s)", m_input.facilityId, static_cast<long long>(m_input.dayStart), static_cast<int>(c), DiagnosticCodeName(code));
}

/**
 * Compose one component by its profile state
 */
void Composition::compose(EnergyComponent c)
{
   ComposedComponent& cc = component(c);
   if (cc.done)
      return;

   MeteringProfileEntry entry = profileEntry(c);
   std::vector<const ProxyDeclaration*> proxies;
   proxiesFor(c, &proxies);

   bool stateKnown = (entry.state == SlotState::METERED) || (entry.state == SlotState::PROXIED) || (entry.state == SlotState::ABSENT) ||
            (entry.state == SlotState::NOT_APPLICABLE) || (entry.state == SlotState::LEVEL_AMBIGUOUS);
   bool zeroForbidden = (entry.state == SlotState::NOT_APPLICABLE) && ((c == EnergyComponent::EIT) || (c == EnergyComponent::EDC_GROSS));
   bool countMissing = (entry.state == SlotState::METERED) && (entry.expectedBindings <= 0);
   if (!stateKnown || zeroForbidden || countMissing)
   {
      setMissing(c, ComputationDiagnosticCode::PROFILE_INVALID);
      return;
   }

   switch(entry.state)
   {
      case SlotState::NOT_APPLICABLE:
         // Declared zero: the flow does not exist at this facility
         cc.value.value = 0;
         cc.value.attributes = SampleAttributes();
         cc.value.attributes.bounded = true;
         cc.value.attributes.lower = 0;
         cc.value.attributes.upper = 0;
         cc.basis.lowest = 0;
         memset(cc.basis.share, 0, sizeof(cc.basis.share));
         for(const ProxyDeclaration *p : proxies)
            addDiagnostic(&cc.diagnostics, c, ComputationDiagnosticCode::PROXY_IGNORED_STATE, p->id);
         cc.done = true;
         break;
      case SlotState::ABSENT:
      case SlotState::LEVEL_AMBIGUOUS:
         for(const ProxyDeclaration *p : proxies)
            addDiagnostic(&cc.diagnostics, c, ComputationDiagnosticCode::PROXY_IGNORED_STATE, p->id);
         setMissing(c, (entry.state == SlotState::ABSENT) ? ComputationDiagnosticCode::SLOT_ABSENT : ComputationDiagnosticCode::SLOT_LEVEL_AMBIGUOUS);
         break;
      case SlotState::PROXIED:
      {
         bool hasAdd = false, hasDeduct = false;
         for(const ProxyDeclaration *p : proxies)
         {
            if (p->kind == ProxyKind::DEDUCT)
               hasDeduct = true;
            else
               hasAdd = true;
         }
         if (hasDeduct)
         {
            // A deduction needs a measured flow to deduct from
            setMissing(c, ComputationDiagnosticCode::DEDUCTION_WITHOUT_MEASUREMENT);
            break;
         }
         if (!hasAdd)
         {
            setMissing(c, ComputationDiagnosticCode::NO_VALID_PROXY);
            break;
         }
         double value = 0;
         for(const ProxyDeclaration *p : proxies)
         {
            value += p->dailyQuantity * m_dayLength / 86400.0;
            addDiagnostic(&cc.diagnostics, c, ComputationDiagnosticCode::PROXY_USED, p->id);
         }
         cc.value.value = value;
         cc.value.attributes = SampleAttributes();
         cc.value.attributes.quality = SampleQuality::PROXY;
         cc.basis.lowest = 0;
         cc.basis.share[0] = 1;
         cc.basis.share[1] = cc.basis.share[2] = cc.basis.share[3] = 0;
         cc.done = true;
         break;
      }
      case SlotState::METERED:
         composeMetered(c, entry, proxies);
         break;
      default:
         break;
   }
}

/**
 * Compose metered component from its PRIMARY series, subsets and proxies
 */
void Composition::composeMetered(EnergyComponent c, const MeteringProfileEntry& entry, const std::vector<const ProxyDeclaration*>& proxies)
{
   ComposedComponent& cc = component(c);

   std::vector<const ResolvedSeries*> primary;
   for(const ResolvedSeries *s : m_primarySeries)
      if (s->binding.component == c)
         primary.push_back(s);

   // A meter that drops out of the bindings must not silently lower the total
   if (static_cast<int32_t>(primary.size()) != entry.expectedBindings)
   {
      double completeness = primary.empty() ? 0 : 1;
      for(const ResolvedSeries *s : primary)
         completeness = std::min(completeness, m_integrated[s->binding.dciId].coverage);
      setMissing(c, ComputationDiagnosticCode::BINDING_COUNT_MISMATCH, completeness, static_cast<double>(primary.size()));
      return;
   }

   double total = 0;
   double completeness = 1;
   bool refused = false;
   for(const ResolvedSeries *s : primary)
   {
      const SeriesIntegrationResult& r = m_integrated[s->binding.dciId];
      completeness = std::min(completeness, r.coverage);
      if (r.accepted)
         total += r.value;
      else
         refused = true;
   }
   if (refused)
   {
      setMissing(c, ComputationDiagnosticCode::INPUT_REFUSED, completeness);
      return;
   }

   // Subsets contribute to the parent only when the parent is not metered at facility level: a facility-level
   // meter is by definition the total of all supplies
   bool usedProxy = false;
   const SubsetRule *subsets = nullptr;
   size_t subsetCount = 0;
   if (c == EnergyComponent::EDC_GROSS)
   {
      subsets = s_edcSubsets;
      subsetCount = sizeof(s_edcSubsets) / sizeof(SubsetRule);
   }
   else if (c == EnergyComponent::WIN)
   {
      subsets = s_winSubsets;
      subsetCount = sizeof(s_winSubsets) / sizeof(SubsetRule);
   }
   if ((subsets != nullptr) && (entry.summingClass != OBJECT_FACILITY))
   {
      for(size_t i = 0; i < subsetCount; i++)
      {
         EnergyComponent sub = subsets[i].subset;
         compose(sub);
         const ComposedComponent& sc = component(sub);
         if (sc.value.attributes.quality == SampleQuality::MISSING)
         {
            if ((profileEntry(sub).state == SlotState::ABSENT) && !subsets[i].absentRefusesParent)
               continue;
            setMissing(c, ComputationDiagnosticCode::SUBSET_UNRESOLVED, std::min(completeness, sc.value.attributes.completeness), static_cast<double>(static_cast<int>(sub)));
            return;
         }
         total += sc.value.value;
         completeness = std::min(completeness, sc.value.attributes.completeness);
         if (sc.value.attributes.quality == SampleQuality::PROXY)
            usedProxy = true;
      }
   }

   // Proxies in ascending ID, prorated to the actual day length; additions and deductions accumulated separately
   double added = 0, deducted = 0;
   for(const ProxyDeclaration *p : proxies)
   {
      double quantity = p->dailyQuantity * m_dayLength / 86400.0;
      if (p->kind == ProxyKind::ADD)
         added += quantity;
      else
         deducted += quantity;
      addDiagnostic(&cc.diagnostics, c, ComputationDiagnosticCode::PROXY_USED, p->id);
   }

   double value = (total + added) - deducted;
   if (value < 0)
   {
      // A deduction larger than the measured flow is a declaration error
      cc.diagnostics.clear();
      setMissing(c, ComputationDiagnosticCode::NEGATIVE_RESULT, completeness, value);
      return;
   }

   cc.value.value = value;
   cc.value.attributes = SampleAttributes();
   cc.value.attributes.quality = (!proxies.empty() || usedProxy) ? SampleQuality::PROXY : SampleQuality::MEASURED;
   cc.value.attributes.completeness = completeness;

   if (c == EnergyComponent::EIT)
   {
      // Metered energy keeps the basis of the summing level; additive proxy energy has basis 0; deductions adjust the metered flow
      int b = ((entry.measurementBasis >= 0) && (entry.measurementBasis <= 3)) ? entry.measurementBasis : 0;
      memset(cc.basis.share, 0, sizeof(cc.basis.share));
      if (value > 0)
      {
         cc.basis.share[b] += (total - deducted) / value;
         cc.basis.share[0] += added / value;
      }
      cc.basis.lowest = (added > 0) ? 0 : static_cast<int16_t>(b);
   }
   cc.done = true;
}

/**
 * Compose all components and fill provider output
 */
void Composition::run(ComputationOutput *output)
{
   for(int i = 0; i < ENERGY_COMPONENT_COUNT; i++)
      compose(static_cast<EnergyComponent>(i));

   output->diagnostics = m_seriesDiagnostics;
   for(int i = 0; i < ENERGY_COMPONENT_COUNT; i++)
   {
      output->components[i] = m_components[i].value;
      output->diagnostics.insert(output->diagnostics.end(), m_components[i].diagnostics.begin(), m_components[i].diagnostics.end());
   }
   output->eitBasis = component(EnergyComponent::EIT).basis;
}

/**
 * Compose the eight components for one facility-day
 */
void ComposeComponents(const ComputationInput& input, ComputationOutput *output)
{
   Composition composition(input);
   composition.run(output);
}
