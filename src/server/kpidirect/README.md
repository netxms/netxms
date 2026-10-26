# kpidirect — direct KPI computation provider

Open computation provider of the NetXMS KPI engine. For one facility and one local calendar
day it turns the raw samples of the bound meter series into the eight daily energy and water
components (EIT, EDC_GROSS, EDC_BG, RES_OS, HU, WIN, WIN_FRE, REUSE). It does arithmetic only:
counter differences and trapezoid integration. It never estimates. A component whose inputs do
not fully cover the day is refused and recorded as `MISSING`, with a diagnostic saying why.

Method id `kpidirect`, version `1.0`. Selected with `Facility.provider_id = "kpidirect"`.

The provider is specified by *GreenDC KPIDirect Design v0.1* and verified against the reference
implementation and test vectors in `tests/test-kpidirect`. The constants below are part of the
method version: changing any of them is a new version, not a setting.

| Constant | Value |
|---|---|
| Bridgeable gap | `min(4 × nominal interval, 3600 s)` |
| Required coverage of a PRIMARY series | 1.0 (the whole day) |
| Plausibility limit on a counter increment | 2 × rated power of the binding object |

## Degradation rules

### Per series

Series are classified by binding resolution from the DCI's unit and delta calculation method:
`RATE` (power, flow), `COUNTER` (cumulative energy or volume register), `INTERVAL` (energy or
volume since the previous sample). Samples are supplied over the day plus one hour on each side.

- Samples are ordered by time. Two samples at one timestamp are collapsed (`MEASURED` wins, then
  the higher value) and reported as `DUPLICATE_TIMESTAMP`.
- Samples that are not `MEASURED` or not finite are dropped (`NON_MEASURED_SAMPLE`); negative
  values are dropped (`NEGATIVE_VALUE`). Fewer than two usable samples refuse the series
  (`TOO_FEW_SAMPLES`).
- `RATE`: trapezoid rule between consecutive usable samples, cut at the day boundaries. A spacing
  longer than the bridgeable gap leaves that interval uncovered (`GAP_NOT_BRIDGED`).
- `COUNTER`: the increment between consecutive usable samples, in full for a pair inside the day
  at any length, as a linear share for a pair across a day boundary. A decrease
  (`COUNTER_DECREASE`), an increment implying more than twice the rated power
  (`IMPLAUSIBLE_INCREMENT`), or a pair across a boundary longer than the bridgeable gap
  (`UNRESOLVED_BOUNDARY`) leaves the interval uncovered and is never repaired.
- `INTERVAL`: each sample's quantity is attributed to the interval since the previous stored
  sample. A sample without predecessor in the input (`INTERVAL_WITHOUT_START`), after a spacing
  longer than the bridgeable gap (`INTERVAL_AFTER_GAP`), negative or implausible is uncovered.
- Coverage is the covered fraction of the day's duration. Any uncovered time is reported
  (`UNCOVERED_TIME`); a series below the required coverage is not accepted
  (`INSUFFICIENT_COVERAGE`). A value is never scaled up for missing coverage.

### Per component

Only PRIMARY series (those at the slot's summing level) are summed, in ascending DCI id.

| Profile state | Result |
|---|---|
| `NOT_APPLICABLE` (not allowed for EIT and EDC_GROSS) | 0, `MEASURED`, bounded [0, 0], completeness 1 |
| `ABSENT`, `LEVEL_AMBIGUOUS` | `MISSING` (`SLOT_ABSENT`, `SLOT_LEVEL_AMBIGUOUS`) |
| `PROXIED` | sum of ADD proxies, `PROXY`; a DEDUCT proxy or no ADD proxy gives `MISSING` |
| `METERED` | below |
| anything else, or `METERED` without an expected binding count (zero means unspecified) | `MISSING` (`PROFILE_INVALID`) |

A metered component is `MISSING` if the number of PRIMARY series differs from the profile's
expected count (`BINDING_COUNT_MISMATCH`), if any PRIMARY series is not accepted
(`INPUT_REFUSED`), if a required subset is unresolved (`SUBSET_UNRESOLVED`), or if the result is
negative (`NEGATIVE_RESULT`). Its completeness is the minimum coverage of the contributing series.

Subsets are added to their parent unless the parent is metered at facility level: EDC_BG and
RES_OS into EDC_GROSS, where an `ABSENT` subset refuses the parent (a separate supply point that
is not metered); WIN_FRE into WIN, where an `ABSENT` subset does not (a classification of water
already metered under WIN).

Operator-declared static proxies are applied pro rata to the day length, additions and
deductions in ascending id, and mark the component `PROXY` (`PROXY_USED`). Proxies on a slot
whose state ignores them are reported (`PROXY_IGNORED_STATE`). A proxy never fills a meter
outage.

### EIT measurement basis

The basis is that of the `eed.eit` summing level (rack 3, PDU or busway 2, UPS 1, other 0) and
is never upgraded. Additive proxy energy has basis 0 and lowers the reported lowest basis to 0;
deductions keep the basis of the metered flow. A proxied EIT has basis 0; a missing EIT has
basis −1.

## Determinism

The output does not depend on the order of series, samples or proxies in the input. Series are
summed in ascending DCI id, samples in ascending time, proxies in ascending id; no clock, random
or global state is read. The engine treats a different value from an identical input digest and
method version as a defect.

## Logging

Debug tag `kpi.direct`: level 4 one line per refused component, level 6 per series (kind,
nominal interval, gap limit, coverage, value), level 7 each rejected interval.
