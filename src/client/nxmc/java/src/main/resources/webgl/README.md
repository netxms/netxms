# WebGL resources for the 3D rack view

These assets back `org.netxms.nxmc.modules.objects.widgets.Rack3DWidget`.

| File | Purpose |
|------|---------|
| `rack3d.html` | Page template loaded into the embedded `Browser`. Contains the `nxSetScene()` entry point and the `nxOnSelect()` / `nxGetTexture()` callbacks. |
| `three.min.js` | **Not committed.** Three.js library, inlined into `rack3d.html` at load time (replaces the `/*__THREEJS__*/` marker). |

## Adding three.min.js

The widget inlines `three.min.js` rather than fetching it from a CDN, because the
desktop client and many server networks are offline, and `Browser.setText()`
renders without a base URL (so `<script src>` would not resolve).

Drop a pinned build of Three.js (and, if orbit controls are wanted, an
`OrbitControls` build appended into the same file) here as `three.min.js`. It
must ship as a Maven resource so it lands on the classpath at
`/webgl/three.min.js`. Until it is present, the widget renders a "library not
bundled" placeholder instead of throwing.

## Java/JS contract

`Rack3DWidget` serializes a scene object to JSON and calls `nxSetScene(json)`.
Field names below are consumed verbatim in `rack3d.html`:

```
{ height, topBottomNumbering, frontOnly,
  units: [ { id, name, position, height, side, statusColor,
             frontImage, rearImage, passive } ] }
```

`id` is the NetXMS object id for active devices and `-1` for passive elements;
`side` is `FRONT` / `REAR` / `FILL`; `frontImage` / `rearImage` are image-library
GUIDs (or `null`); `statusColor` is a `#rrggbb` string computed on the Java side.

## Depth from orientation

The page derives partial depth from `side` for active devices: a `FRONT`- or
`REAR`-only device occupies a fraction of the rack depth (`PARTIAL_DEPTH`,
default ½) flush with the matching end, while `FILL` spans the full depth. This
needs no model change. The textured plate is placed on the outward-facing box
face (+Z for front, -Z for rear).

Passive elements (`type` field) get fixed, type-specific depths instead, since
they are faceplates or shallow assemblies, not full-depth gear: filler panel
~15 mm, patch panel ~40 mm, organiser ~60 mm, PDU ~100 mm. A `FILL`-oriented
passive panel is treated as a front-mounted faceplate.

True per-device depth/width would require new model fields and is out of scope
here.
