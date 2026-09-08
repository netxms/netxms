---
worth: later
added: 2026-09-08
---
# out-of-range rack placements are accepted everywhere and silently hidden

Nothing validates a rack position or height against the rack:

- console: `PhysicalContainerPlacement.java:398,407` fix the position and height spinners at 1..50
  regardless of the rack's height, and `RackProperties.java:108` lets the rack height shrink freely
  after devices are placed
- server: `Node::modifyFromMessageInternal` (node.cpp:11042-11045) and
  `Chassis::modifyFromMessageInternal` store `VID_RACK_POSITION` / `VID_RACK_HEIGHT` as sent
- display: `RackWidget.java:339-343` skips drawing any entity whose extent falls outside the rack, so
  the device just disappears from the rack view with no marker and no error

A node at position 45 in a rack later set to 42U is an ordinary reachable state. PR #3633 adds strict
extent validation on the JSON path only, so once it merges the two write paths disagree, and an object
already out of range cannot be patched through the API until its geometry is repaired.

Unresolved: whether the NXCP path should become strict too (rejects console edits that used to succeed,
and needs the rack's current height at modify time) or stay lenient with the console clamping its
spinners to the rack height and the widget showing an out-of-bounds marker. Settle that before touching
either side.
