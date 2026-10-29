---
worth: yes
where: src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/objects/ObjectsPerspective.java:467
added: 2026-10-29
---
# "Manage views" drop-down menu is never disposed

Every click on the "Manage views" tool item's drop-down arrow calls `createClosedViewMenu()`, which creates
a new `Menu(perspectiveToolBar)` — a pop-up menu, so its parent is the shell. Nothing disposes it:
`menuHidden` is empty and the menu is never attached with `setMenu()`. `menuShown` fills it with items
whose selection listeners capture hidden `View` objects, so each click keeps a menu, its items and those
views' widget trees reachable until the shell is disposed — unbounded over a long RWT session.

Same leak class as issue #3846 (sortable viewer header menu). Fix: create the menu once next to the tool
item, dispose it with the item, and clear its items in `menuShown` before rebuilding; or dispose it in
`menuHidden` via `getDisplay().asyncExec(menu::dispose)` (dispose before item selection is delivered
would drop the click). Surfaced as pre-existing by revmux task `issue-3846`, round `01-initial`.
