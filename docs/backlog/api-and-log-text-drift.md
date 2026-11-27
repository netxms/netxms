---
worth: yes
added: 2026-11-27
---
# API descriptions and log texts that disagree with the code

Found while writing the data dictionary (master fc2a1b0ed8).

- openapi.yaml `AiOperator.cycle` (~16289) says "Number of completed cycles"; `m_cycle++` runs when a
  cycle starts and counts failed ones (`ai_operator.cpp:462`).
- openapi.yaml cloud domain grace period (`:21249`) says seconds; code multiplies by 86400
  (`cloud_domain.cpp:556`), default 30 = days.
- `actions.cpp:569` debug type-name array is out of sync with `ServerActionType` (SSH actions log as
  "SEND EMAIL"); `epp.cpp:2352` sizes the custom attribute delete batch from `m_pstorageDeleteActions`.
