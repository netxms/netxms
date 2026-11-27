# Design contributions

This document is for designers who want to work on the NetXMS user interface: the desktop console (SWT) and the web
console (RWT). Both share most of the code, so a design has to work within what both toolkits can render.

## Process

1. Open an [issue](https://github.com/netxms/netxms/issues) describing the problem: which screen, which task, what
   goes wrong for the user. Attach mockups, wireframes or a link to a public Figma file if you have them.
2. Discuss the approach in the issue, on the [forum](https://www.netxms.org/forum/index.php) or in
   [GitHub Discussions](https://github.com/netxms/netxms/discussions) before investing in detailed design.
3. Iterate on the design in the open; share work in progress early.
4. When the team picks the issue up for implementation, review the pull request against your design.

The general contribution rules in [CONTRIBUTING.md](CONTRIBUTING.md) apply.

## Users

Two groups use the console, often different people with different skills:

**Administrators** set up monitoring: add nodes, create data collection items and templates, configure event
processing, notifications and access rights. They have deep technical knowledge, work on a desktop or laptop, and
need complete configuration options with clear validation and error messages. Their main complaints are complex
configuration dialogs and missing feedback on invalid input.

**Operators** watch dashboards, network maps and alarm lists, investigate problems and escalate them. They work in
operations centers on multiple monitors, often under time pressure, and are less technical. They need clear status at
a glance and a fast path from an alarm to its cause. Their main complaints are hard-to-read dashboards and maps, weak
information hierarchy and small fonts on high-resolution displays.

## Principles

* Clarity of information over visual appearance
* No design that slows down operator workflows or adds clicks to frequent tasks
* Status must be readable without relying on color alone
* Consistency between desktop and web consoles

## Known problem areas

* Navigation: balance between showing enough information and keeping views uncluttered
* Icons are outdated and inconsistent
* Typography and font rendering, especially on network maps
* Initial setup is hard to understand for new users
* Configuration dialogs: layout, validation and error messages
* Network map auto-layout for large networks

## License

Design contributions (UI designs, mockups, icons, graphics and documentation) are licensed under the
[MIT License](https://opensource.org/licenses/MIT), which allows NetXMS to use, modify and distribute them in both
open-source and commercial products.
