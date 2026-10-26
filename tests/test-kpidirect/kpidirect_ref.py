#!/usr/bin/env python3
"""
Reference implementation of the kpidirect provider (Green DC direct KPI computation), method version 1.0.

Normative companion of GreenDC_KPIDirect_Design_v0.1.md. The C++ module is verified against the
test vectors produced from this file (kpidirect_vectors.json), not against the prose of the
document. Where the two disagree, the disagreement is a defect in one of them and is resolved
by changing the document first.

Pure functions only; no NetXMS dependencies. Times are UTC seconds, values in the DCI's declared
unit, outputs in canonical units (kWh, m3). Section references (§) are to the design document.
"""

import math

# --------------------------------------------------------------------------------------------
# Constants (§6.1)
# --------------------------------------------------------------------------------------------

GAP_FACTOR = 4
GAP_CAP = 3600.0
THETA = 1.0
PLAUSIBILITY_FACTOR = 2.0
EPS = 1e-9

# --------------------------------------------------------------------------------------------
# Classification (§5) — performed by binding resolution in production
# --------------------------------------------------------------------------------------------

UNITS = {
   'Wh': ('energy', 0.001), 'kWh': ('energy', 1.0), 'MWh': ('energy', 1000.0), 'GWh': ('energy', 1e6),
   'W': ('power', 0.001), 'kW': ('power', 1.0), 'MW': ('power', 1000.0),
   'm3': ('volume', 1.0), 'm³': ('volume', 1.0), 'L': ('volume', 0.001), 'l': ('volume', 0.001),
   'm3/h': ('flow', 1.0), 'm³/h': ('flow', 1.0),
   'L/h': ('flow', 0.001), 'l/h': ('flow', 0.001),
   'L/min': ('flow', 0.06), 'l/min': ('flow', 0.06),
   'L/s': ('flow', 3.6), 'l/s': ('flow', 3.6),
}
REJECTED_UNITS = {
   'VA': 'APPARENT_POWER', 'kVA': 'APPARENT_POWER', 'MVA': 'APPARENT_POWER',
   'var': 'REACTIVE_POWER', 'kvar': 'REACTIVE_POWER', 'A': 'NOT_A_FLOW_QUANTITY', '%': 'NOT_A_FLOW_QUANTITY',
}
ENERGY_SLOTS = {'eed.eit', 'eed.edc', 'eed.edc_bg', 'eed.res_os', 'eed.reuse_upgrade', 'eed.reuse'}
WATER_SLOTS = {'eed.win', 'eed.win_fre'}


def classify(unit, delta_mode, slot, origin='POLLED'):
   """Returns (kind, factor, None) or (None, None, exclusion_code). Checks in the order of §5."""
   if origin == 'COMPUTED':
      return None, None, 'COMPUTED_ORIGIN'
   u = (unit or '').strip()
   if u == '':
      return None, None, 'NO_UNIT'
   if u in REJECTED_UNITS:
      return None, None, REJECTED_UNITS[u]
   if u not in UNITS:
      return None, None, 'UNKNOWN_UNIT'
   quantity, factor = UNITS[u]
   wanted = ('energy', 'power') if slot in ENERGY_SLOTS else ('volume', 'flow')
   if quantity not in wanted:
      return None, None, 'UNIT_SLOT_MISMATCH'
   if quantity in ('power', 'flow'):
      if delta_mode != 'ORIGINAL':
         return None, None, 'RATE_WITH_DELTA'
      return 'RATE', factor, None
   if delta_mode == 'ORIGINAL':
      return 'COUNTER', factor, None
   if delta_mode == 'SIMPLE':
      return 'INTERVAL', factor, None
   return None, None, 'AVERAGE_DELTA_ON_QUANTITY'


# --------------------------------------------------------------------------------------------
# Diagnostics (§4.2 A-3, §10.1)
# --------------------------------------------------------------------------------------------

def dg(code, component=-1, dci=0, frm=0, to=0, detail=None):
   return {'component': component, 'dciId': dci, 'code': code, 'from': frm, 'to': to, 'detail': detail}


# --------------------------------------------------------------------------------------------
# Per-series daily integration (§6)
# --------------------------------------------------------------------------------------------

def prepare(series):
   """§6.2 steps 1-2: ascending order, duplicate timestamps collapsed to the highest value."""
   diag = []
   by_t = {}
   for s in series['samples']:
      t, v = s[0], s[1]
      q = s[2] if len(s) > 2 else 'MEASURED'
      if t in by_t:
         diag.append(dg('DUPLICATE_TIMESTAMP', dci=series['dciId'], frm=t, to=t))
         o = by_t[t]
         # deterministic: keep MEASURED over others, then the higher finite value
         def rank(x):
            return (x[1] == 'MEASURED', x[0] if (x[0] is not None and math.isfinite(x[0])) else -math.inf)
         if rank((v, q)) > rank(o):
            by_t[t] = (v, q)
      else:
         by_t[t] = (v, q)
   return [(t, by_t[t][0], by_t[t][1]) for t in sorted(by_t)], diag


def nominal_interval(series, stored):
   if series.get('nominalInterval', 0) > 0:
      return float(series['nominalInterval'])
   ts = [s[0] for s in stored]
   d = sorted(b - a for a, b in zip(ts, ts[1:]) if b > a)
   if not d:
      return None
   n = len(d)
   return float(d[n // 2]) if n % 2 == 1 else (d[n // 2 - 1] + d[n // 2]) / 2.0


def overlap(a, b, S, E):
   lo = max(a, S)
   hi = min(b, E)
   return (lo, hi) if hi > lo else None


def integrate_series(series, S, E):
   """Integrate one classified series over [S, E). Returns dict(value, coverage, accepted, diag)."""
   dci = series['dciId']
   kind, factor = series['kind'], series['unitFactor']
   stored, diag = prepare(series)
   L = E - S

   def refuse(code):
      diag.append(dg(code, dci=dci))
      return {'value': None, 'coverage': 0.0, 'accepted': False, 'diag': diag}

   dn = nominal_interval(series, stored)
   if dn is None:
      return refuse('TOO_FEW_SAMPLES')
   G = min(GAP_FACTOR * dn, GAP_CAP)
   rated_kw = series.get('ratedPower', 0) / 1000.0
   check_plausible = rated_kw > 0 and series['slot'] in ENERGY_SLOTS

   value = 0.0
   covered = 0.0

   if kind in ('RATE', 'COUNTER'):
      usable = []
      for t, v, q in stored:
         if q != 'MEASURED' or v is None or not math.isfinite(v):
            diag.append(dg('NON_MEASURED_SAMPLE', dci=dci, frm=t, to=t))
            continue
         if v < 0:
            diag.append(dg('NEGATIVE_VALUE', dci=dci, frm=t, to=t, detail=v))
            continue
         usable.append((t, v * factor))
      if len(usable) < 2:
         return refuse('TOO_FEW_SAMPLES')
      for (ta, va), (tb, vb) in zip(usable, usable[1:]):
         ov = overlap(ta, tb, S, E)
         if ov is None:
            continue
         lo, hi = ov
         dt = tb - ta
         if kind == 'RATE':
            if dt > G + EPS:
               diag.append(dg('GAP_NOT_BRIDGED', dci=dci, frm=lo, to=hi))
               continue
            vlo = va + (vb - va) * (lo - ta) / dt
            vhi = va + (vb - va) * (hi - ta) / dt
            value += (vlo + vhi) / 2.0 * (hi - lo) / 3600.0
            covered += hi - lo
         else:
            inc = vb - va
            if inc < 0:
               diag.append(dg('COUNTER_DECREASE', dci=dci, frm=lo, to=hi, detail=inc))
               continue
            if check_plausible and inc / (dt / 3600.0) > PLAUSIBILITY_FACTOR * rated_kw + EPS:
               diag.append(dg('IMPLAUSIBLE_INCREMENT', dci=dci, frm=lo, to=hi, detail=inc / (dt / 3600.0)))
               continue
            straddles = (ta < S < tb) or (ta < E < tb)
            if straddles and dt > G + EPS:
               diag.append(dg('UNRESOLVED_BOUNDARY', dci=dci, frm=lo, to=hi))
               continue
            value += inc * (hi - lo) / dt
            covered += hi - lo
   else:  # INTERVAL
      if sum(1 for t, v, q in stored if q == 'MEASURED' and v is not None and math.isfinite(v)) < 1:
         return refuse('TOO_FEW_SAMPLES')
      prev_t = None
      for t, v, q in stored:
         start = prev_t
         prev_t = t
         if q != 'MEASURED' or v is None or not math.isfinite(v):
            diag.append(dg('NON_MEASURED_SAMPLE', dci=dci, frm=t, to=t))
            continue
         if start is None:
            if S <= t < E + G:   # its interval could reach into the day
               diag.append(dg('INTERVAL_WITHOUT_START', dci=dci, frm=t, to=t))
            continue
         ov = overlap(start, t, S, E)
         if ov is None:
            continue
         lo, hi = ov
         dt = t - start
         qv = v * factor
         if qv < 0:
            diag.append(dg('NEGATIVE_VALUE', dci=dci, frm=lo, to=hi, detail=qv))
            continue
         if dt > G + EPS:
            diag.append(dg('INTERVAL_AFTER_GAP', dci=dci, frm=lo, to=hi))
            continue
         if check_plausible and qv / (dt / 3600.0) > PLAUSIBILITY_FACTOR * rated_kw + EPS:
            diag.append(dg('IMPLAUSIBLE_INCREMENT', dci=dci, frm=lo, to=hi, detail=qv / (dt / 3600.0)))
            continue
         value += qv * (hi - lo) / dt
         covered += hi - lo

   coverage = covered / L
   if coverage < 1.0 - EPS:
      diag.append(dg('UNCOVERED_TIME', dci=dci, detail=L - covered))
   accepted = coverage >= THETA - EPS
   if not accepted:
      diag.append(dg('INSUFFICIENT_COVERAGE', dci=dci, detail=coverage))
   return {'value': value, 'coverage': coverage, 'accepted': accepted, 'diag': diag}


# --------------------------------------------------------------------------------------------
# Component composition (§7) and static proxies (§8)
# --------------------------------------------------------------------------------------------

COMPONENTS = ['EIT', 'EDC_GROSS', 'EDC_BG', 'RES_OS', 'HU', 'WIN', 'WIN_FRE', 'REUSE']
CINDEX = {c: i for i, c in enumerate(COMPONENTS)}
SLOT_OF = {'EIT': 'eed.eit', 'EDC_GROSS': 'eed.edc', 'EDC_BG': 'eed.edc_bg', 'RES_OS': 'eed.res_os',
           'HU': 'eed.reuse_upgrade', 'WIN': 'eed.win', 'WIN_FRE': 'eed.win_fre', 'REUSE': 'eed.reuse'}
# parent -> [(subset component, ABSENT subset makes parent incomplete)]
SUBSETS = {'EDC_GROSS': [('EDC_BG', True), ('RES_OS', True)], 'WIN': [('WIN_FRE', False)]}
SUBSET_COMPONENTS = {'EDC_BG', 'RES_OS', 'WIN_FRE'}
NOT_APPLICABLE_FORBIDDEN = {'EIT', 'EDC_GROSS'}
STATES = {'METERED', 'PROXIED', 'ABSENT', 'NOT_APPLICABLE', 'LEVEL_AMBIGUOUS'}


def proxies_in_force(slot, proxies):
   """Proxies are supplied already filtered to those in force on the day (§8.2). Ascending ID."""
   return sorted((p for p in proxies if p['slot'] == slot), key=lambda p: p['id'])


def missing(comp, code, completeness=0.0, diag=None, detail=None):
   d = list(diag or [])
   d.append(dg(code, component=CINDEX[comp], detail=detail))
   r = {'quality': 'MISSING', 'value': None, 'bounded': False, 'completeness': completeness, 'diag': d}
   if comp == 'EIT':
      r['eitBasis'] = {'lowest': -1, 'share': [0.0, 0.0, 0.0, 0.0]}
   return r


def compose(vec):
   S, E = vec['dayStart'], vec['dayEnd']
   L = E - S
   profile = vec['profile']
   proxies = vec.get('proxies', [])
   series = sorted(vec['series'], key=lambda s: s['dciId'])

   series_diag = []
   integ = {}
   for s in series:
      if s['role'] == 'PRIMARY':
         r = integrate_series(s, S, E)
         integ[s['dciId']] = r
         series_diag += r['diag']

   out = {}

   def component(comp):
      if comp in out:
         return out[comp]
      slot = SLOT_OF[comp]
      pe = profile.get(slot, {'state': 'ABSENT'})
      state = pe.get('state')
      px = proxies_in_force(slot, proxies)
      ignored = [dg('PROXY_IGNORED_STATE', component=CINDEX[comp], detail=p['id']) for p in px]

      if state not in STATES or (state == 'NOT_APPLICABLE' and comp in NOT_APPLICABLE_FORBIDDEN) or \
            (state == 'METERED' and not isinstance(pe.get('expected'), int)):
         r = missing(comp, 'PROFILE_INVALID')
      elif state == 'NOT_APPLICABLE':
         r = {'quality': 'MEASURED', 'value': 0.0, 'bounded': True, 'lower': 0.0, 'upper': 0.0,
              'completeness': 1.0, 'diag': ignored}
      elif state in ('ABSENT', 'LEVEL_AMBIGUOUS'):
         r = missing(comp, 'SLOT_' + state, diag=ignored)
      elif state == 'PROXIED':
         if any(p['kind'] == 'DEDUCT' for p in px):
            r = missing(comp, 'DEDUCTION_WITHOUT_MEASUREMENT')
         elif not any(p['kind'] == 'ADD' for p in px):
            r = missing(comp, 'NO_VALID_PROXY')
         else:
            value = 0.0
            for p in px:
               value += p['dailyQuantity'] * L / 86400.0
            r = {'quality': 'PROXY', 'value': value, 'bounded': False, 'completeness': 1.0,
                 'diag': [dg('PROXY_USED', component=CINDEX[comp], detail=p['id']) for p in px]}
            if comp == 'EIT':
               r['eitBasis'] = {'lowest': 0, 'share': [1.0, 0.0, 0.0, 0.0]}
      else:
         r = metered(comp, slot, pe, px)
      out[comp] = r
      return r

   def metered(comp, slot, pe, px):
      prim = [s for s in series if s['slot'] == slot and s['role'] == 'PRIMARY']
      n = len(prim)
      if n != pe['expected']:
         compl = min([integ[s['dciId']]['coverage'] for s in prim], default=0.0)
         return missing(comp, 'BINDING_COUNT_MISMATCH', compl, detail=n)
      if n == 0:
         return missing(comp, 'NO_PRIMARY_BINDINGS')
      total = 0.0
      compl = 1.0
      refused = False
      for s in prim:                      # ascending DCI ID
         r = integ[s['dciId']]
         compl = min(compl, r['coverage'])
         if r['accepted']:
            total += r['value']
         else:
            refused = True
      if refused:
         return missing(comp, 'INPUT_REFUSED', compl)
      used_proxy = False
      diag = []
      # subset rule 4 (§7.3 step 3): subsets in table order
      if comp in SUBSETS and pe.get('summingLevel') != 'FACILITY':
         for sub, absent_blocks in SUBSETS[comp]:
            sr = component(sub)
            sstate = profile.get(SLOT_OF[sub], {'state': 'ABSENT'}).get('state')
            if sr['quality'] == 'MISSING':
               if sstate == 'ABSENT' and not absent_blocks:
                  continue
               return missing(comp, 'SUBSET_UNRESOLVED', min(compl, sr['completeness']), detail=CINDEX[sub])
            total += sr['value']
            compl = min(compl, sr['completeness'])
            if sr['quality'] == 'PROXY':
               used_proxy = True
      add = 0.0
      ded = 0.0
      for p in px:                        # ascending ID; ADD and DEDUCT accumulated separately
         q = p['dailyQuantity'] * L / 86400.0
         if p['kind'] == 'ADD':
            add += q
         else:
            ded += q
         diag.append(dg('PROXY_USED', component=CINDEX[comp], detail=p['id']))
      value = (total + add) - ded
      if value < 0:
         return missing(comp, 'NEGATIVE_RESULT', compl, detail=value)
      r = {'quality': 'PROXY' if (px or used_proxy) else 'MEASURED', 'value': value, 'bounded': False,
           'completeness': compl, 'diag': diag}
      if comp == 'EIT':
         b = pe.get('basis', 0)
         share = [0.0, 0.0, 0.0, 0.0]
         if value > 0:
            share[b] += (total - ded) / value
            share[0] += add / value
         r['eitBasis'] = {'lowest': 0 if add > 0 else b, 'share': share}
      return r

   for comp in COMPONENTS:
      component(comp)
   return {'components': out, 'seriesDiagnostics': series_diag}
