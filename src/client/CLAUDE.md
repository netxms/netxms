# Client Components

## Management Console (nxmc)

### Tools vs Configuration Perspective

A new **on-demand utility** view (search, scan, probe, ad-hoc query) belongs in the **Tools perspective**, registered via an `org.netxms.nxmc.services.ToolDescriptor` implementation in `modules/tools/` (e.g. `FindByMacAddressDescriptor`, `ObjectFinderDescriptor`, `ServerConsoleDescriptor`), with the view class in `modules/tools/views/`. The descriptor returns a `View` from `createView()` plus name/icon/optional required-component ID, and the Tools perspective picks it up automatically.

The **Configuration perspective** is for persistent server-side config (network discovery, credentials, EPP) — different SPI and registration. `NetworkCredentialsView` is a Configuration view and is *not* the reference for transient tool windows.

### Constrained Selectors

A combo that exists to restrict input to a known vocabulary (e.g. the DCI "Interpretation" system tag selector) stays read-only. When a value needs a variable suffix or qualifier, add a separate field for that part instead of making the combo editable — a free-text combo defeats the selector's purpose. Still preserve values the list does not know: show them as an extra entry and write them back unchanged so saving never silently drops data.

### SWT vs RWT

`WidgetHelper` (`org.netxms.nxmc.tools`) has separate SWT (`src/swt/`) and RWT (`src/rwt/`) implementations that abstract API differences between the two toolkits. In shared code (`src/main/`), always check `WidgetHelper` for a wrapper before calling SWT widget methods that may not exist in RWT. For example, never call `FileDialog.setFilterExtensions()`, `setFilterNames()`, or `getFileNames()` directly — use `WidgetHelper.setFileDialogFilterExtensions()`, `setFileDialogFilterNames()`, and `getFileDialogFileList()` instead. Direct calls will compile for desktop but break the web build.

## Java Client Library (netxms-client)

**Parsing NXCP responses:** read new fields directly (`msg.getFieldAsX(VID_...)`). Do not add `isFieldPresent()` guards, "default when absent" fallbacks, or comments about older servers. Every release bumps the NXCP protocol version, so a client never connects to a server that lacks the fields it expects; such fallbacks are dead code.

## nxshell

Interactive shell working via web API: object navigation, server debug console, AI assistant,
and ad-hoc NXSL execution. Started as `nxai`, it works as AI assistant client. Input parsing
lives in `parser.cpp`, which has no network or terminal dependencies and is covered by
`tests/test-nxshell`. See [nxshell/README.md](nxshell/README.md).
