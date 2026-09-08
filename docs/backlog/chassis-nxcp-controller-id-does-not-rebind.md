---
worth: yes
where: src/server/core/chassis.cpp:178
added: 2026-09-08
---
# chassis NXCP controller id change does not rebind the object tree on its own

`Chassis::modifyFromMessageInternal` stores `VID_CONTROLLER_ID` into `m_controllerId` and returns. The
only trigger for `updateControllerBinding()` is `Chassis::updateFlags` (chassis.cpp:202-210), which fires
when the modification mask carries `CHF_BIND_UNDER_CONTROLLER`. The console always sends the flag mask
together with the controller id (`Communication.java:297-299`), so the desktop path rebinds by accident
of client behaviour. A raw NXCP client, nxshell or the Java API calling `setControllerId` without
`setObjectFlags` gets a persisted controller id and a chassis still linked under the old controller,
with the old controller's inherited access rights, until restart.

Surfaced reviewing PR #3633, whose WebAPI `controllerId` property has the same omission with no masking
client. Fix is a `ThreadPoolExecute(g_mainThreadPool, this, &Chassis::updateControllerBinding)` next to
the assignment, the same shape `VID_PHYSICAL_CONTAINER_ID` already uses two lines below. Any fix should
land together with #3634, which serialises these rebind jobs.
