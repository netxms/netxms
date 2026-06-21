# Dashboard Building

This skill builds and edits NetXMS dashboards from natural-language requests such as
"build a dashboard for server X with CPU and memory line charts and an alarm viewer".
Dashboards are assembled **incrementally**: create an empty dashboard, then add one
element at a time. Each element is validated as it is added, so problems surface
immediately and per element.

## Workflow

Follow this order:

1. **Discover element types.** Call `list-dashboard-element-types` to see what can be
   placed on a dashboard. Each entry has a `type` name, a `category`
   (`reference`, `chart`, or `static`), and a one-line description.
2. **Learn a type's schema on demand.** Before adding an element of an unfamiliar type,
   call `describe-dashboard-element-type` for that type to get its configuration fields
   (with which are required) and a filled example. Do **not** guess field names.
3. **Resolve data references first.** Chart elements reference nodes and DCIs by numeric
   ID. Use the data-collection tools (for example `get-metrics`) to resolve the
   `nodeId`/`dciId` for each metric the user wants to chart, and the object tools to
   resolve object IDs for reference elements. Never invent IDs.
4. **Create the dashboard.** Call `create-dashboard` with a name and a column count.
   Two or three columns is a good default. It returns the dashboard `id`.
5. **Add elements one by one.** For each element call `add-dashboard-element` with the
   dashboard ID, the element `type`, a `config` object that matches the type's schema,
   and a `width` intent. Check the result before adding the next element.
6. **Review and adjust.** Call `get-dashboard` to read back the column count and the
   ordered elements, each with its stable `guid`. Use the GUID to `update-dashboard-element`,
   `remove-dashboard-element`, or `move-dashboard-element`.

## Layout

Do not compute grid spans yourself. Express the desired width as an **intent** and the
skill maps it to the dashboard's column count:

- `full` — span all columns
- `half` — span half the columns
- `third` — span a third of the columns
- `quarter` — span a quarter of the columns

Optionally pass `height` (pixels) for a fixed element height; otherwise the element grows
to fill available vertical space. If a width intent would exceed the dashboard's column
count it is reduced to fit, and the adjustment is reported back in the `notes` field of
the result.

By default an element grabs extra vertical space. A **label** has no natural height of its
own, so always pass `grabVerticalSpace: false` for labels to keep section headings compact.
For other elements this is a layout judgement rather than a rule — leave the default unless
an element row would otherwise look stretched or sparse.

Plan the layout from the column count. For a two-column dashboard, two `half`-width charts
sit side by side on one row; a `full`-width alarm viewer spans both columns on the next row.

## Element identity

Every element has a stable GUID that is independent of its position. `get-dashboard`
returns these GUIDs, and the update/remove/move tools target elements **by GUID**, not by
position. Moving an element changes its order but preserves its GUID.

## Validation and error handling

The skill validates before changing anything:

- **Missing required fields** are rejected; the element is not added.
- **Dangling or inaccessible references** (a node, object, or DCI that does not exist or
  that the user cannot read) are rejected with a descriptive error; the element is not added.
  Re-resolve the correct ID and retry — do not remove the reference to force it through.
- **Out-of-range layout width** is clamped to fit and reported in `notes`.

When a reference cannot be resolved, tell the user what is missing rather than fabricating
an ID or silently dropping the metric.

## Element categories

- **Reference elements** display an existing object or a URL: network maps, rack diagrams,
  geo maps, status maps, alarm viewers, service component views, embedded web pages, and
  embedded dashboards. Their configuration is mostly an object ID or URL.
- **Chart elements** visualize collected data: line, bar, and pie charts (each takes a
  `dciList` of `{nodeId, dciId}` references), dial/gauge charts, availability
  charts (a business service), table-value grids (a table DCI), and DCI summary tables.
  A dial-chart accepts a `dciList` of one or more DCIs, with a single shared scale
  (`minValue`/`maxValue`) and zones. **Combine every gauge that shares the same scale into
  one dial-chart element** with multiple data sources rather than creating one element per
  gauge — NetXMS packs the gauges side by side, which uses space far more effectively. The
  test for combining is the **numeric scale, not what the metric measures**: percentage
  metrics all share the 0–100 scale, so a CPU-usage %, a memory %, and a disk-usage % gauge
  belong together in a single dial-chart element even though they measure different things.
  A typical "current usage" request therefore becomes **one** full-width dial-chart element
  with several data sources, not three separate gauges. Use separate dial-chart elements
  only when the scale (min/max) or the zones genuinely differ.

  Whether a gauge element should grab extra vertical space depends on the surrounding
  layout. If a gauge row ends up over-tall or sparse next to other content, pass
  `grabVerticalSpace: false` (optionally with a modest `height`) to keep it compact;
  otherwise the default is fine.
- **Static elements** are layout aids: text labels (useful as section headings) and
  separators.

Prefer the curated element types this skill exposes. For a request that needs an element
type not in the list (for example scripted/NXSL charts or dashboard templates), explain the
limitation instead of approximating it with the wrong element type.

## Placement

By default a new dashboard is created at the top level (under Dashboards). Pass `group` to
place it inside a named dashboard group. Pass `associateWith` to attach the dashboard to a
specific object so it appears in that object's Dashboards tab.
