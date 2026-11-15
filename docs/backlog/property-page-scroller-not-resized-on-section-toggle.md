---
worth: yes
where: src/client/nxmc/java/src/main/java/org/netxms/nxmc/base/propertypages/PropertyPage.java
added: 2026-11-15
---
# property dialog scroller is not resized when a page shows or hides a section

Pages that toggle a section by flipping `GridData.exclude` (`RuleIncident`, `RuleAlarm`, `Transformation`,
`TrafficObserverCommunication`; find others with `grep -rn "exclude = " src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/*/propertypages`)
only call `layout(true, true)` on their own composite. `PreferenceDialog` (both desktop JFace and RAP JFace)
sets the page container's `ScrolledComposite` minimum size once in `showPage()` and never recomputes it.
When an expanded section grows past the visible height, the dialog has no usable scrollbar. When a tall
page collapses, the extra scroll space stays.

Fix: add a shared `PropertyPage` helper that re-lays out the page control, recomputes
`getControl().computeSize(SWT.DEFAULT, SWT.DEFAULT, true)` (`PreferencePage.computeSize()` caches, so don't
use it), and updates the minimum size of the enclosing `ScrolledComposite`, found by walking up the parents.
Only call it after page construction. Convert every page that toggles sections. Verify in a deliberately
short dialog on both desktop and web.

Surfaced while fixing the EPP rule "Incident" page checkbox state bug. No user report: the default dialog
size fits the expanded incident page.
