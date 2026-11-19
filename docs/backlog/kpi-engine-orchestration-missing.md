---
worth: later
where: src/server/core/kpi_methods.cpp:162
added: 2026-11-19
---
# KPI engine: providers are registered but never run

The Facility object has _Settlement lag (days)_ and _Computation provider (empty to disable KPI engine)_,
`RegisterComputationProvider` keeps a registry, and `kpidirect` (since release-7.0.2) registers provider
`kpidirect` 1.0 with tested arithmetic for the eight daily energy/water components. But nothing calls
`FindComputationProvider` or `ComputationProvider::compute` outside tests (master, release-7.0.4, private
EE tree): no code resolves meter bindings into a `ComputationInput`, picks settled days by the settlement
lag, calls the provider, or stores the `ComputationOutput`. The metering profile and proxy declaration
tables named in `include/nxkpi.h` comments are not in the schema. `Facility::isEngineEnabled()` / REST
`engineEnabled` is only `!providerId.isEmpty()`, so it reports an engine that does not run.

Open question: is the orchestration planned for 7.1 (then the console label and `engineEnabled` are just
ahead of the code), or should the Facility fields be hidden or relabelled until it exists?

The admin guide does not describe the engine; netxms-doc-antora `docs/backlog/kpi-engine-undocumented.md`
waits for this.
