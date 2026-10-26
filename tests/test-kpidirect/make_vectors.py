#!/usr/bin/env python3
"""Generate kpidirect test vectors with expected outputs from the reference implementation.

Usage: python3 -I make_vectors.py <directory containing kpidirect_ref.py>
Writes kpidirect_vectors.json to the current directory and prints the Appendix A table.
"""

import copy
import json
import math
import sys

sys.path.insert(0, sys.argv[1] if len(sys.argv) > 1 else '.')
from kpidirect_ref import compose, classify, COMPONENTS  # noqa: E402

TZ = 'EET-2EEST,M3.5.0/3,M10.5.0/4'   # Europe/Riga
DAY = {'dayKey': 20261021, 'dayStart': 1792530000, 'dayEnd': 1792616400}   # 24 h
SHORT = {'dayKey': 20260329, 'dayStart': 1774735200, 'dayEnd': 1774818000}  # 23 h, DST start
LONG = {'dayKey': 20261025, 'dayStart': 1792875600, 'dayEnd': 1792965600}   # 25 h, DST end
MARGIN = 3600
S = DAY['dayStart']


def grid(day, step, offset=0):
   t = day['dayStart'] - MARGIN + offset
   out = []
   while t <= day['dayEnd'] + MARGIN:
      out.append(t)
      t += step
   return out


def rate(day, step, f, offset=0):
   return [[t, f(t)] for t in grid(day, step, offset)]


def counter(day, step, kw, base=50000.0, offset=0):
   t0 = day['dayStart'] - MARGIN
   return [[t, base + kw * (t - t0) / 3600.0] for t in grid(day, step, offset)]


def drop(samples, t_from, t_to):
   return [s for s in samples if not (t_from < s[0] < t_to)]


def series(dci, slot, unit, samples, role='PRIMARY', delta='ORIGINAL', poll=300, rated=0):
   kind, factor, excl = classify(unit, delta, slot)
   assert excl is None, excl
   return {'dciId': dci, 'slot': slot, 'role': role, 'unit': unit, 'deltaMode': delta,
           'kind': kind, 'unitFactor': factor, 'nominalInterval': poll, 'ratedPower': rated,
           'samples': samples}


def metered(expected=1, level='PDU', basis=None):
   e = {'state': 'METERED', 'expected': expected, 'summingLevel': level}
   if basis is not None:
      e['basis'] = basis
   return e


def eit_profile(expected=1):
   return {'eed.eit': metered(expected, 'PDU', 2)}


def proxy(pid, slot, kind, qty, why):
   return {'id': pid, 'slot': slot, 'kind': kind, 'dailyQuantity': qty, 'justification': why}


V = []


def add(vid, title, day, profile, ser, proxies=None, focus=None):
   v = dict(copy.deepcopy(day))
   v.update({'id': vid, 'title': title, 'timeZoneRule': TZ, 'profile': profile, 'series': ser,
             'proxies': proxies or [], 'focus': focus or ['EIT']})
   V.append(v)


flat = lambda t: 100.0  # noqa: E731

add('V01', 'Power, constant 100 kW, 5 min polling', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kW', rate(DAY, 300, flat))])
add('V02', 'Power, linear ramp 100->340 kW, 15 min polling', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kW', rate(DAY, 900, lambda t: 100.0 + 240.0 * (t - S) / 86400.0), poll=900)])
add('V03', 'Power, gap of exactly 4 intervals (20 min = G)', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kW', drop(rate(DAY, 300, flat), S + 36000, S + 37200))])
add('V04', 'Power, gap of 25 min (5 intervals)', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kW', drop(rate(DAY, 300, flat), S + 36000, S + 37500))])
add('V05', 'Power, 1 h polling, one sample missing -> 2 h gap', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kW', drop(rate(DAY, 3600, flat), S + 36000, S + 43200), poll=3600)])
add('V06', 'Energy counter, 15 min, samples 8 min before / 7 min after midnight', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kWh', counter(DAY, 900, 100.0, offset=420), poll=900)])
add('V07', 'Energy counter with 6 h interior gap', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kWh', drop(counter(DAY, 900, 100.0, offset=420), S + 20000, S + 41600), poll=900)])
add('V08', 'Energy counter, 90 min gap straddling midnight', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kWh', drop(counter(DAY, 900, 100.0), S - 2700, S + 2700), poll=900)])

c9 = [[t, v if t < S + 43200 else 10.0 + 100.0 * (t - S - 43200) / 3600.0] for t, v in counter(DAY, 900, 100.0)]
add('V09', 'Energy counter decreases at noon (meter replaced)', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kWh', c9, poll=900)])
c10 = [[t, v + (1000.0 if t >= S + 43200 else 0.0)] for t, v in counter(DAY, 900, 100.0)]
add('V10', 'Energy counter +1000 kWh in 15 min, rated power 150 kW', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kWh', c10, poll=900, rated=150000)])
add('V11', 'Interval energy (delta SIMPLE), 25 kWh per 15 min, samples 8 min before / 7 min after midnight', DAY,
    eit_profile(), [series(101, 'eed.eit', 'kWh', [[t, 25.0] for t in grid(DAY, 900, 420)], delta='SIMPLE', poll=900)])
add('V12', 'DST start day (23 h), constant 100 kW', SHORT, eit_profile(),
    [series(101, 'eed.eit', 'kW', rate(SHORT, 300, flat))])
add('V13', 'DST end day (25 h), constant 100 kW', LONG, eit_profile(),
    [series(101, 'eed.eit', 'kW', rate(LONG, 300, flat))])
add('V14', 'A/B PDU feed counters 60 kW each (PRIMARY) + UPS 125 kW (AUXILIARY), expected 2', DAY, eit_profile(2),
    [series(101, 'eed.eit', 'kWh', counter(DAY, 900, 60.0), poll=900),
     series(102, 'eed.eit', 'kWh', counter(DAY, 900, 60.0, base=70000.0), poll=900),
     series(201, 'eed.eit', 'kWh', counter(DAY, 900, 125.0), role='AUXILIARY', poll=900)])
add('V15', 'Profile expects 3 PRIMARY series, 2 present', DAY, eit_profile(3),
    [series(101, 'eed.eit', 'kWh', counter(DAY, 900, 60.0), poll=900),
     series(102, 'eed.eit', 'kWh', counter(DAY, 900, 60.0), poll=900)])

EDC_GRID = metered(1, 'GRID_ENTRY')
NA = {'state': 'NOT_APPLICABLE'}
add('V16', 'eed.edc at GRID_ENTRY counter 150 kW, DEDUCT proxy 48 kWh/day; no generator, no on-site RES', DAY,
    {'eed.edc': EDC_GRID, 'eed.edc_bg': NA, 'eed.res_os': NA},
    [series(301, 'eed.edc', 'kWh', counter(DAY, 900, 150.0), poll=900)],
    proxies=[proxy(1, 'eed.edc', 'DEDUCT', 48.0, 'Office floor on site meter')], focus=['EDC_GROSS'])
add('V17', 'eed.win PROXIED, ADD proxy 1.2 m3/day, 23 h day', SHORT, {'eed.win': {'state': 'PROXIED'}}, [],
    proxies=[proxy(2, 'eed.win', 'ADD', 1.2, 'Quarterly utility bill average')], focus=['WIN'])
add('V18', 'eed.reuse_upgrade NOT_APPLICABLE', DAY, {'eed.reuse_upgrade': NA}, [], focus=['HU'])
add('V19', 'EIT counter 100 kW at PDU + ADD proxy 120 kWh/day', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kWh', counter(DAY, 900, 100.0), poll=900)],
    proxies=[proxy(3, 'eed.eit', 'ADD', 120.0, 'Two racks on unmetered board')])
r20 = [[t, -0.5 if t == S + 30000 else v] for t, v in rate(DAY, 300, flat)]
add('V20', 'Power 100 kW with one sample of -0.5 kW', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kW', r20)])
add('V21', 'eed.edc at GRID_ENTRY 150 kW + eed.edc_bg generator 20 kW; res_os NOT_APPLICABLE', DAY,
    {'eed.edc': EDC_GRID, 'eed.edc_bg': metered(1, 'GENERATOR'), 'eed.res_os': NA},
    [series(301, 'eed.edc', 'kWh', counter(DAY, 900, 150.0), poll=900),
     series(302, 'eed.edc_bg', 'kWh', counter(DAY, 900, 20.0), poll=900)], focus=['EDC_GROSS', 'EDC_BG'])
add('V22', 'eed.edc at FACILITY 170 kW (sees generator) + eed.edc_bg 20 kW; res_os ABSENT', DAY,
    {'eed.edc': metered(1, 'FACILITY'), 'eed.edc_bg': metered(1, 'GENERATOR')},
    [series(301, 'eed.edc', 'kWh', counter(DAY, 900, 170.0), poll=900),
     series(302, 'eed.edc_bg', 'kWh', counter(DAY, 900, 20.0), poll=900)], focus=['EDC_GROSS', 'EDC_BG'])
add('V23', 'UPS output in kVA excluded by binding resolution; profile expects 1 series', DAY,
    {'eed.eit': metered(1, 'UPS', 1)}, [])
add('V24', 'eed.win volume counter in litres, 50 L/h', DAY, {'eed.win': metered(1, 'FACILITY')},
    [series(401, 'eed.win', 'L', counter(DAY, 900, 50.0, base=1000.0), poll=900)], focus=['WIN'])
add('V25', 'eed.edc at GRID_ENTRY + eed.edc_bg expects 2 meters, 1 present', DAY,
    {'eed.edc': EDC_GRID, 'eed.edc_bg': metered(2, 'GENERATOR'), 'eed.res_os': NA},
    [series(301, 'eed.edc', 'kWh', counter(DAY, 900, 150.0), poll=900),
     series(302, 'eed.edc_bg', 'kWh', counter(DAY, 900, 20.0), poll=900)], focus=['EDC_GROSS', 'EDC_BG'])
add('V26', 'eed.edc at GRID_ENTRY, eed.edc_bg ABSENT (generator exists, unmetered)', DAY,
    {'eed.edc': EDC_GRID, 'eed.res_os': NA},
    [series(301, 'eed.edc', 'kWh', counter(DAY, 900, 150.0), poll=900)], focus=['EDC_GROSS', 'EDC_BG'])
add('V27', 'eed.edc at GRID_ENTRY 150 kW, eed.res_os PROXIED 50 kWh/day, no generator', DAY,
    {'eed.edc': EDC_GRID, 'eed.edc_bg': NA, 'eed.res_os': {'state': 'PROXIED'}},
    [series(301, 'eed.edc', 'kWh', counter(DAY, 900, 150.0), poll=900)],
    proxies=[proxy(4, 'eed.res_os', 'ADD', 50.0, 'PV inverter monthly readout')], focus=['EDC_GROSS', 'RES_OS'])
add('V28', 'eed.win metered at FACILITY, eed.win_fre ABSENT (unclassified water)', DAY,
    {'eed.win': metered(1, 'FACILITY')},
    [series(401, 'eed.win', 'm3', counter(DAY, 900, 0.05, base=10.0), poll=900)], focus=['WIN', 'WIN_FRE'])
c29 = counter(DAY, 900, 100.0)
c29.append([S + 43200, 10.0])     # replaced meter reading recorded with the same timestamp
add('V29', 'Energy counter with two readings at the same timestamp (newest-first input)', DAY, eit_profile(),
    [series(101, 'eed.eit', 'kWh', list(reversed(c29)), poll=900)])
add('V30', 'eed.win PROXIED with only a DEDUCT proxy of 0', DAY, {'eed.win': {'state': 'PROXIED'}}, [],
    proxies=[proxy(5, 'eed.win', 'DEDUCT', 0.0, 'Erroneous entry')], focus=['WIN'])
add('V31', 'Power series in W, 15 min polling, input newest first', DAY, eit_profile(),
    [series(101, 'eed.eit', 'W', list(reversed(rate(DAY, 900, lambda t: 80000.0))), poll=900)])
add('V32', 'eed.eit declared NOT_APPLICABLE (invalid profile)', DAY, {'eed.eit': NA}, [])

CLASSIFICATION = []
for unit, delta, slot in [
      ('kWh', 'ORIGINAL', 'eed.eit'), ('kWh', 'SIMPLE', 'eed.eit'), ('kWh', 'AVERAGE_PER_SECOND', 'eed.eit'),
      ('kW', 'ORIGINAL', 'eed.eit'), ('kW', 'SIMPLE', 'eed.eit'), ('W', 'ORIGINAL', 'eed.edc'),
      ('MWh', 'ORIGINAL', 'eed.edc'), ('kVA', 'ORIGINAL', 'eed.eit'), ('kvar', 'ORIGINAL', 'eed.eit'),
      ('A', 'ORIGINAL', 'eed.eit'), ('', 'ORIGINAL', 'eed.eit'), ('kwh', 'ORIGINAL', 'eed.eit'),
      (' kWh ', 'ORIGINAL', 'eed.eit'), ('m3', 'ORIGINAL', 'eed.eit'), ('kWh', 'ORIGINAL', 'eed.win'),
      ('m3', 'ORIGINAL', 'eed.win'), ('L', 'SIMPLE', 'eed.win_fre'), ('L/min', 'ORIGINAL', 'eed.win'),
      ('m3/h', 'SIMPLE', 'eed.win')]:
   kind, factor, excl = classify(unit, delta, slot)
   CLASSIFICATION.append({'unit': unit, 'deltaMode': delta, 'slot': slot, 'origin': 'POLLED',
                          'kind': kind, 'unitFactor': factor, 'exclusion': excl})
kind, factor, excl = classify('kWh', 'ORIGINAL', 'eed.eit', 'COMPUTED')
CLASSIFICATION.append({'unit': 'kWh', 'deltaMode': 'ORIGINAL', 'slot': 'eed.eit', 'origin': 'COMPUTED',
                       'kind': kind, 'unitFactor': factor, 'exclusion': excl})


def clean(o):
   if isinstance(o, float):
      return o if math.isfinite(o) else None
   if isinstance(o, dict):
      return {k: clean(x) for k, x in o.items()}
   if isinstance(o, (list, tuple)):
      return [clean(x) for x in o]
   return o


for v in V:
   res = compose(copy.deepcopy(v))
   v['expected'] = clean(res)

json.dump({'methodVersion': '1.0', 'classification': CLASSIFICATION, 'vectors': V},
          open('kpidirect_vectors.json', 'w'), indent=1)

for v in V:
   parts = []
   comps = v['expected']['components']
   for c in v['focus']:
      r = comps[c]
      unit = 'm³' if c.startswith('WIN') else 'kWh'
      if r['quality'] == 'MISSING':
         codes = [d['code'] for d in r['diag'] if d['component'] >= 0 and d['code'] != 'PROXY_IGNORED_STATE']
         s = '%s MISSING — %s' % (c, ', '.join(codes))
         if r['completeness'] not in (0.0, 1.0):
            s += ' (completeness %.6f)' % r['completeness']
      else:
         s = '%s %s %s %s' % (c, r['quality'], ('%.6f' % r['value']).rstrip('0').rstrip('.'), unit)
         if r.get('bounded'):
            s += ', bounded [0, 0]'
         if 'eitBasis' in r and r['eitBasis']['lowest'] != 2:
            s += '; basis lowest %d, share [%s]' % (r['eitBasis']['lowest'],
                                                    ', '.join(('%.6f' % x).rstrip('0').rstrip('.') for x in r['eitBasis']['share']))
      parts.append(s)
   sc = sorted({d['code'] for d in v['expected']['seriesDiagnostics']} - {'UNCOVERED_TIME', 'INSUFFICIENT_COVERAGE'})
   extra = ('; series: ' + ', '.join(sc)) if sc else ''
   print('| %s | %s | %s%s |' % (v['id'], v['title'], '; '.join(parts), extra))
print()
for c in CLASSIFICATION:
   print(c)
