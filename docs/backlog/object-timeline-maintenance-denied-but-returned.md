---
worth: yes
where: src/server/core/object_timeline.cpp:871
added: 2026-11-17
---
# object timeline returns maintenance journal records in a category marked access-denied

For a user without _View event log_, the event provider marks every event-derived category denied
(`setAccessDenied`, line 465), including `maintenance`, because maintenance enter/leave come from
the event log. `MaintenanceJournalProvider` then adds maintenance journal records to the same
category without any access check, so `get-object-timeline` reports `maintenance` as
`accessDenied` while still returning records in it. Either the journal records are visible to
such users (then the category should not be marked denied when journal records exist) or they are
not (then the provider should skip them). Found while documenting the tool for 7.1 (docs repo,
`ai/ai-concepts.adoc`, "Object Timeline").
