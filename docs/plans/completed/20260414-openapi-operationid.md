# Fix OpenAPI Spec Vacuum Lint Errors

## Overview
- Fix all 144 vacuum lint errors in `src/server/webapi/openapi.yaml`
- 143 operations missing `operationId` + 1 missing `info.description`
- Brings error count to 0; warnings/info intentionally left for future work
- No code changes — spec-only, no impact on runtime behavior

## Context
- File: `src/server/webapi/openapi.yaml` (~6800 lines, 143 operations)
- Current vacuum score: 10/100 (144 errors, 441 warnings, 511 info)
- No existing vacuum config or spectral ruleset in repo
- `operationId` naming convention: **camelCase verb-resource** (e.g. `getAlarms`, `createUser`)

## Naming Convention

| HTTP Pattern | `operationId` Pattern | Example |
|---|---|---|
| `GET /collection` | `list{Resources}` | `listAlarms` |
| `POST /collection` | `create{Resource}` | `createUser` |
| `GET /collection/{id}` | `get{Resource}` | `getAlarm` |
| `PUT /collection/{id}` | `update{Resource}` | `updateUser` |
| `DELETE /collection/{id}` | `delete{Resource}` | `deleteUser` |
| `POST /resource/{id}/action` | `{action}{Resource}` | `acknowledgeAlarm` |

## Full operationId Mapping

### Server / Auth
| Method | Path | operationId |
|---|---|---|
| GET | `/` | `getRoot` |
| GET | `/v1/server-info` | `getServerInfo` |
| GET | `/v1/status` | `getSessionStatus` |
| POST | `/v1/login` | `login` |
| POST | `/v1/logout` | `logout` |

### TCP Proxy
| Method | Path | operationId |
|---|---|---|
| POST | `/v1/tcp-proxy` | `createTcpProxySession` |
| GET | `/v1/tcp-proxy/{token}` | `connectTcpProxy` |

### AI Chat
| Method | Path | operationId |
|---|---|---|
| POST | `/v1/ai/chat` | `createAiChat` |
| POST | `/v1/ai/chat/{chat-id}/message` | `sendAiChatMessage` |
| GET | `/v1/ai/chat/{chat-id}/status` | `getAiChatStatus` |
| GET | `/v1/ai/chat/{chat-id}/question` | `getAiChatQuestion` |
| POST | `/v1/ai/chat/{chat-id}/answer` | `answerAiChatQuestion` |
| POST | `/v1/ai/chat/{chat-id}/clear` | `clearAiChatHistory` |
| DELETE | `/v1/ai/chat/{chat-id}` | `deleteAiChat` |

### AI Management
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/ai/skills-and-functions` | `getAiSkillsAndFunctions` |
| POST | `/v1/ai/disabled-items` | `addAiDisabledItem` |
| DELETE | `/v1/ai/disabled-items/{item-type}/{item-name}` | `removeAiDisabledItem` |

### AI Saved Prompts
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/ai/saved-prompts` | `listAiSavedPrompts` |
| POST | `/v1/ai/saved-prompts` | `createAiSavedPrompt` |
| GET | `/v1/ai/saved-prompts/{prompt-id}` | `getAiSavedPrompt` |
| PUT | `/v1/ai/saved-prompts/{prompt-id}` | `updateAiSavedPrompt` |
| DELETE | `/v1/ai/saved-prompts/{prompt-id}` | `deleteAiSavedPrompt` |

### Alarms
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/alarms` | `listAlarms` |
| GET | `/v1/alarms/{alarm-id}` | `getAlarm` |
| POST | `/v1/alarms/{alarm-id}/acknowledge` | `acknowledgeAlarm` |
| POST | `/v1/alarms/{alarm-id}/resolve` | `resolveAlarm` |
| POST | `/v1/alarms/{alarm-id}/terminate` | `terminateAlarm` |

### Find
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/connection-history` | `getConnectionHistory` |
| GET | `/v1/find/mac-address` | `findMacAddress` |

### Geo Areas
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/geo-areas` | `listGeoAreas` |
| POST | `/v1/geo-areas` | `createGeoArea` |
| GET | `/v1/geo-areas/{area-id}` | `getGeoArea` |
| PUT | `/v1/geo-areas/{area-id}` | `updateGeoArea` |
| DELETE | `/v1/geo-areas/{area-id}` | `deleteGeoArea` |

### Object Categories
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/object-categories` | `listObjectCategories` |
| POST | `/v1/object-categories` | `createObjectCategory` |
| GET | `/v1/object-categories/{category-id}` | `getObjectCategory` |
| PUT | `/v1/object-categories/{category-id}` | `updateObjectCategory` |
| DELETE | `/v1/object-categories/{category-id}` | `deleteObjectCategory` |

### Image Library
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/image-library` | `listImages` |
| GET | `/v1/image-library/{guid}` | `getImageMetadata` |
| GET | `/v1/image-library/{guid}/data` | `getImageData` |

### Grafana
| Method | Path | operationId |
|---|---|---|
| POST | `/v1/grafana/infinity/alarms` | `grafanaQueryAlarms` |
| POST | `/v1/grafana/infinity/object-query` | `grafanaQueryObjects` |
| POST | `/v1/grafana/infinity/summary-table` | `grafanaQuerySummaryTable` |
| GET | `/v1/grafana/objects/{object-id}/dci-list` | `grafanaGetDciList` |
| POST | `/v1/grafana/objects-status` | `grafanaGetObjectsStatus` |
| GET | `/v1/grafana/object-list` | `grafanaGetObjectList` |
| GET | `/v1/grafana/summary-table-list` | `grafanaGetSummaryTableList` |
| GET | `/v1/grafana/query-list` | `grafanaGetQueryList` |

### Objects
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/objects` | `listObjects` |
| GET | `/v1/objects/{object-id}` | `getObject` |
| POST | `/v1/objects/{object-id}/execute-agent-command` | `executeAgentCommand` |
| POST | `/v1/objects/{object-id}/execute-dashboard-script` | `executeDashboardScript` |
| POST | `/v1/objects/{object-id}/execute-script` | `executeScript` |
| POST | `/v1/objects/{object-id}/expand-text` | `expandText` |
| POST | `/v1/objects/{object-id}/remote-control` | `createRemoteControlSession` |
| POST | `/v1/objects/{object-id}/set-maintenance` | `setObjectMaintenance` |
| POST | `/v1/objects/{object-id}/set-managed` | `setObjectManaged` |
| GET | `/v1/objects/{object-id}/take-screenshot` | `takeScreenshot` |
| GET | `/v1/objects/{object-id}/topology/l2` | `getL2Topology` |
| GET | `/v1/objects/{object-id}/topology/ip` | `getIpTopology` |
| GET | `/v1/objects/{object-id}/topology/ospf` | `getOspfTopology` |
| GET | `/v1/objects/{object-id}/topology/internal` | `getInternalTopology` |
| GET | `/v1/objects/{object-id}/status-explanation` | `getObjectStatusExplanation` |
| GET | `/v1/objects/{object-id}/sub-tree` | `getObjectSubTree` |
| GET | `/v1/objects/{object-id}/object-tools` | `listObjectToolsForObject` |
| POST | `/v1/objects/query` | `queryObjects` |
| POST | `/v1/objects/search` | `searchObjects` |

### Data Collection
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/objects/{object-id}/data-collection/current-values` | `getDciCurrentValues` |
| GET | `/v1/objects/{object-id}/data-collection/{dci-id}/history` | `getDciHistory` |
| GET | `/v1/objects/{object-id}/data-collection/performance-view` | `getDciPerformanceView` |

### Scheduled Tasks
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/scheduled-task-handlers` | `listScheduledTaskHandlers` |
| GET | `/v1/scheduled-tasks` | `listScheduledTasks` |
| POST | `/v1/scheduled-tasks` | `createScheduledTask` |
| GET | `/v1/scheduled-tasks/{task-id}` | `getScheduledTask` |
| PUT | `/v1/scheduled-tasks/{task-id}` | `updateScheduledTask` |
| DELETE | `/v1/scheduled-tasks/{task-id}` | `deleteScheduledTask` |

### Script Library
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/script-library` | `listScripts` |
| POST | `/v1/script-library` | `createScript` |
| GET | `/v1/script-library/{script-id}` | `getScript` |
| PUT | `/v1/script-library/{script-id}` | `updateScript` |
| DELETE | `/v1/script-library/{script-id}` | `deleteScript` |

### Notification Channels
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/notification-channels` | `listNotificationChannels` |
| POST | `/v1/notification-channels` | `createNotificationChannel` |
| GET | `/v1/notification-channels/{channel-name}` | `getNotificationChannel` |
| PUT | `/v1/notification-channels/{channel-name}` | `updateNotificationChannel` |
| DELETE | `/v1/notification-channels/{channel-name}` | `deleteNotificationChannel` |
| POST | `/v1/notification-channels/{channel-name}/rename` | `renameNotificationChannel` |
| POST | `/v1/notification-channels/{channel-name}/clear-queue` | `clearNotificationChannelQueue` |
| POST | `/v1/notification-channels/{channel-name}/send` | `sendNotification` |
| GET | `/v1/notification-drivers` | `listNotificationDrivers` |

### SSH Keys
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/ssh-keys` | `listSshKeys` |
| POST | `/v1/ssh-keys` | `createSshKey` |
| GET | `/v1/ssh-keys/{key-id}` | `getSshKey` |
| PUT | `/v1/ssh-keys/{key-id}` | `updateSshKey` |
| DELETE | `/v1/ssh-keys/{key-id}` | `deleteSshKey` |

### Web Service Definitions
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/web-service-definitions` | `listWebServiceDefinitions` |
| POST | `/v1/web-service-definitions` | `createWebServiceDefinition` |
| GET | `/v1/web-service-definitions/{definition-id}` | `getWebServiceDefinition` |
| PUT | `/v1/web-service-definitions/{definition-id}` | `updateWebServiceDefinition` |
| DELETE | `/v1/web-service-definitions/{definition-id}` | `deleteWebServiceDefinition` |

### Server Actions
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/server-actions` | `listServerActions` |
| POST | `/v1/server-actions` | `createServerAction` |
| GET | `/v1/server-actions/{action-id}` | `getServerAction` |
| PUT | `/v1/server-actions/{action-id}` | `updateServerAction` |
| DELETE | `/v1/server-actions/{action-id}` | `deleteServerAction` |

### Object Tools
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/object-tools` | `listObjectTools` |
| GET | `/v1/object-tools/{tool-id}` | `getObjectTool` |
| POST | `/v1/object-tools/{tool-id}/execute` | `executeObjectTool` |
| GET | `/v1/object-tools/output/{token}` | `connectObjectToolOutput` |

### Event Templates
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/event-templates` | `listEventTemplates` |
| POST | `/v1/event-templates` | `createEventTemplate` |
| GET | `/v1/event-templates/{event-code}` | `getEventTemplate` |
| PUT | `/v1/event-templates/{event-code}` | `updateEventTemplate` |
| DELETE | `/v1/event-templates/{event-code}` | `deleteEventTemplate` |

### DCI Summary Tables
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/dci-summary-tables` | `listDciSummaryTables` |
| POST | `/v1/dci-summary-tables` | `createDciSummaryTable` |
| GET | `/v1/dci-summary-tables/{table-id}` | `getDciSummaryTable` |
| PUT | `/v1/dci-summary-tables/{table-id}` | `updateDciSummaryTable` |
| DELETE | `/v1/dci-summary-tables/{table-id}` | `deleteDciSummaryTable` |
| POST | `/v1/dci-summary-tables/{table-id}/query` | `queryDciSummaryTable` |
| POST | `/v1/dci-summary-tables/adhoc-query` | `adhocQueryDciSummaryTable` |

### Users
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/users` | `listUsers` |
| POST | `/v1/users` | `createUser` |
| GET | `/v1/users/{user-id}` | `getUser` |
| PUT | `/v1/users/{user-id}` | `updateUser` |
| DELETE | `/v1/users/{user-id}` | `deleteUser` |
| POST | `/v1/users/{user-id}/password` | `setUserPassword` |

### User Groups
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/user-groups` | `listUserGroups` |
| POST | `/v1/user-groups` | `createUserGroup` |
| GET | `/v1/user-groups/{group-id}` | `getUserGroup` |
| PUT | `/v1/user-groups/{group-id}` | `updateUserGroup` |
| DELETE | `/v1/user-groups/{group-id}` | `deleteUserGroup` |

### Two-Factor Authentication
| Method | Path | operationId |
|---|---|---|
| GET | `/v1/users/{user-id}/2fa-bindings` | `listUser2faBindings` |
| PUT | `/v1/users/{user-id}/2fa-bindings/{method-name}` | `setUser2faBinding` |
| DELETE | `/v1/users/{user-id}/2fa-bindings/{method-name}` | `deleteUser2faBinding` |
| GET | `/v1/2fa/drivers` | `list2faDrivers` |
| GET | `/v1/2fa/methods` | `list2faMethods` |
| POST | `/v1/2fa/methods` | `create2faMethod` |
| GET | `/v1/2fa/methods/{method-name}` | `get2faMethod` |
| PUT | `/v1/2fa/methods/{method-name}` | `update2faMethod` |
| DELETE | `/v1/2fa/methods/{method-name}` | `delete2faMethod` |

## Development Approach
- Spec-only changes — no runtime code, no tests needed
- Validation: run `vacuum lint src/server/webapi/openapi.yaml` and confirm 0 errors
- Parse with python/yaml to confirm all 143 operations have operationId and no duplicates

## Implementation Steps

### Task 1: Add info description

**Files:**
- Modify: `src/server/webapi/openapi.yaml`

- [x] Add `description` field to `info` block
- [x] Verify with `vacuum lint` that `info-description` error is gone

### Task 2: Add operationId to all 143 operations

**Files:**
- Modify: `src/server/webapi/openapi.yaml`

- [x] Add operationId to Server/Auth endpoints (5 operations)
- [x] Add operationId to TCP Proxy endpoints (2 operations)
- [x] Add operationId to AI Chat endpoints (7 operations)
- [x] Add operationId to AI Management endpoints (3 operations)
- [x] Add operationId to AI Saved Prompts endpoints (5 operations)
- [x] Add operationId to Alarms endpoints (5 operations)
- [x] Add operationId to Find endpoints (2 operations)
- [x] Add operationId to Geo Areas endpoints (5 operations)
- [x] Add operationId to Object Categories endpoints (5 operations)
- [x] Add operationId to Image Library endpoints (3 operations)
- [x] Add operationId to Grafana endpoints (8 operations)
- [x] Add operationId to Objects endpoints (19 operations)
- [x] Add operationId to Data Collection endpoints (3 operations)
- [x] Add operationId to Scheduled Tasks endpoints (6 operations)
- [x] Add operationId to Script Library endpoints (5 operations)
- [x] Add operationId to Notification Channels endpoints (9 operations)
- [x] Add operationId to SSH Keys endpoints (5 operations)
- [x] Add operationId to Web Service Definitions endpoints (5 operations)
- [x] Add operationId to Server Actions endpoints (5 operations)
- [x] Add operationId to Object Tools endpoints (4 operations)
- [x] Add operationId to Event Templates endpoints (5 operations)
- [x] Add operationId to DCI Summary Tables endpoints (7 operations)
- [x] Add operationId to Users endpoints (6 operations)
- [x] Add operationId to User Groups endpoints (5 operations)
- [x] Add operationId to Two-Factor Authentication endpoints (9 operations)

### Task 3: Validate

- [x] Run `vacuum lint src/server/webapi/openapi.yaml` — confirm 0 errors (score 25/100, 0 errors)
- [x] Run python yaml parse to confirm all operationIds are unique (143/143, all unique)
- [x] Verify no other regressions introduced (warning/info counts unchanged)

## Post-Completion
- Warnings/info violations (examples, descriptions, camelCase) left for future cleanup
- Consider adding a vacuum config (`.vacuum.yml`) to codify which rules matter
- Consider adding vacuum to CI pipeline
