---
worth: later
where: src/client/nxmc/java/src/rwt/java/org/netxms/nxmc/base/windows/PopOutViewWindow.java
added: 2026-11-20
---
# web pop-out windows share the main display's ObjectIcons images

In the web console, `PopOutViewWindow` calls `Registry.copyToDisplay(r.display, display)`, which puts the
main window's whole `Registry` instance into the pop-out's UI session. Singletons registered through
`Registry.setSingleton` — `ObjectIcons` among them — therefore hold images created on the main display,
and `ObjectIcons` disposes them from the main display's `disposeExec`. If the main window logs out while a
pop-out is still open, a pop-out view that sets a cached object image (e.g. restoring a filtered row in an
object tree/table) passes a disposed `Image` to RWT, which throws.

Fix direction: keep display-bound resources such as `ObjectIcons` per `Display` rather than in the
shared `Registry`, or have `copyToDisplay` share only the session-level state.

Surfaced by the revmux review of PR #3992 (node type icons in the object browser), which adds more images
to the same cache; the defect predates it. Unknown that settles the value: whether a pop-out actually
stays usable after the main window logs out, or is torn down with it — if it is torn down, this is a `no`.
