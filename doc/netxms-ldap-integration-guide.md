---
title: "NetXMS LDAP Integration Guide"
author: [Alex Kirhenshtein]
date: "2026-01-28"
lang: "en"
...

# NetXMS LDAP Integration Guide

## Overview

NetXMS supports integration with LDAP-based directory services for centralized user authentication and authorization. This includes full compatibility with **Microsoft Active Directory** and **OpenLDAP** servers.

With LDAP integration:

- Users authenticate against the LDAP directory using their existing credentials
- User and group objects are automatically synchronized from LDAP into NetXMS
- LDAP group memberships map directly to NetXMS groups for access control
- Login is fully transparent -- users enter their directory username and password

NetXMS performs **one-way synchronization**: users and groups are read from LDAP and replicated into NetXMS. Changes in NetXMS (such as access rights assignments) are stored locally and not written back to LDAP.

---

## How It Works

### Architecture

```
+------------------+         +---------------------------+
|   LDAP Server    |<--------|       NetXMS Server       |
|  (AD / OpenLDAP) |  LDAP   |         (netxmsd)         |
|                  |  Sync   |                           |
|  Users & Groups  |-------->|  Internal User Database   |
+------------------+         |  (synced replica)         |
                             +----------v----------------+
                                        |
                             +----------v---------------+
                             |  User Login              |
                             |  1. Lookup user in DB    |
                             |  2. Bind to LDAP with    |
                             |     user's DN + password |
                             |  3. Grant/deny access    |
                             +--------------------------+
```

### Synchronization Process

1. NetXMS server connects to the LDAP server using a dedicated service account
2. Searches configured base DN(s) with the specified filter
3. Identifies user and group objects by their object class
4. Creates or updates corresponding entries in the NetXMS internal database
5. Synchronizes group memberships (including nested groups)
6. Handles deleted LDAP entries (disables or removes them based on configuration)

### Authentication Flow

1. User enters username and password in the NetXMS management console
2. NetXMS looks up the user in its internal database
3. If the user is an LDAP-synced user, NetXMS performs an LDAP bind using the user's DN and the provided password
4. On successful bind, access is granted according to the user's NetXMS permissions

---

## Prerequisites

- NetXMS server built with LDAP support (enabled by default on most distributions)
- Network connectivity from the NetXMS server to the LDAP server (port 389 for LDAP, port 636 for LDAPS)
- A dedicated LDAP service account with read access to the user/group directory tree
- Knowledge of your LDAP directory structure (base DN, user/group object classes, attribute names)

---

## Configuration

All LDAP settings are configured as server configuration variables. These can be set via:

- **Management Console**: Configuration > Server Configuration
- **Server debug console**: using `set` command
- **Command line**: `nxdbmgr set <parameter> <value>`

### Required Parameters

| Parameter | Description |
|-----------|-------------|
| `LDAP.ConnectionString` | LDAP server URI(s). Format: `ldap://host:389` or `ldaps://host:636`. Multiple servers can be listed (comma or space separated) for failover. |
| `LDAP.SyncUser` | DN of the service account used to search the directory. Example: `CN=netxms-svc,CN=Users,DC=example,DC=com` |
| `LDAP.SyncUserPassword` | Password for the service account |
| `LDAP.SearchBase` | Base DN for searches. Example: `DC=example,DC=com`. Multiple bases can be separated with `;` |
| `LDAP.SearchFilter` | LDAP search filter. Use `(objectClass=*)` to include all objects |
| `LDAP.UserClass` | Object class that identifies user objects (e.g., `user` for AD, `inetOrgPerson` for OpenLDAP) |
| `LDAP.GroupClass` | Object class that identifies group objects (e.g., `group` for AD, `groupOfNames` for OpenLDAP) |
| `LDAP.Mapping.UserName` | Attribute used as the login name (e.g., `sAMAccountName` for AD, `cn` or `uid` for OpenLDAP) |
| `LDAP.Mapping.GroupName` | Attribute used as the group name |
| `LDAP.SyncInterval` | Synchronization interval in minutes. Set to `0` to disable automatic sync. Recommended: `1440` (daily) or `60` (hourly) |

### Optional Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `LDAP.Mapping.FullName` | Attribute for the user's full/display name | (empty) |
| `LDAP.Mapping.Description` | Attribute for user description | (empty) |
| `LDAP.Mapping.Email` | Attribute for user email address | (empty) |
| `LDAP.Mapping.PhoneNumber` | Attribute for user phone number | (empty) |
| `LDAP.UserUniqueId` | Attribute used as a stable unique identifier for users. Recommended if user DNs may change (e.g., `objectGUID` for AD, `entryUUID` for OpenLDAP) | DN-based |
| `LDAP.GroupUniqueId` | Attribute used as a stable unique identifier for groups | DN-based |
| `LDAP.UserDeleteAction` | Action when an LDAP user is removed from the directory: `0` = delete from NetXMS, `1` = disable in NetXMS | `1` (disable) |
| `LDAP.NewUserAuthMethod` | Authentication method for newly synced users. Default is LDAP password authentication | LDAP |
| `LDAP.PageSize` | Maximum records per search page (for large directories) | `1000` |

---

## Configuration Examples

### Microsoft Active Directory

| Parameter | Value |
|-----------|-------|
| `LDAP.ConnectionString` | `ldap://dc01.example.com:389` |
| `LDAP.SyncUser` | `CN=netxms-svc,CN=Users,DC=example,DC=com` |
| `LDAP.SyncUserPassword` | *(service account password)* |
| `LDAP.SearchBase` | `DC=example,DC=com` |
| `LDAP.SearchFilter` | `(objectClass=*)` |
| `LDAP.UserClass` | `user` |
| `LDAP.GroupClass` | `group` |
| `LDAP.Mapping.UserName` | `sAMAccountName` |
| `LDAP.Mapping.GroupName` | `sAMAccountName` |
| `LDAP.Mapping.FullName` | `displayName` |
| `LDAP.Mapping.Description` | `description` |
| `LDAP.Mapping.Email` | `mail` |
| `LDAP.UserUniqueId` | `objectGUID` |
| `LDAP.GroupUniqueId` | `objectGUID` |
| `LDAP.SyncInterval` | `1440` |
| `LDAP.UserDeleteAction` | `1` |

**Notes for Active Directory:**
- Use `sAMAccountName` as the user/group name mapping for traditional Windows logon names
- `objectGUID` is recommended as the unique identifier since it persists across DN changes (e.g., user moves between OUs)
- For LDAPS (encrypted), use `ldaps://dc01.example.com:636`
- The sync user needs read access to the directory subtree defined by the search base
- To limit synchronization scope, set `LDAP.SearchBase` to a specific OU: `OU=NetXMS Users,DC=example,DC=com`

### OpenLDAP

| Parameter | Value |
|-----------|-------|
| `LDAP.ConnectionString` | `ldap://ldap.example.com:389` |
| `LDAP.SyncUser` | `cn=admin,dc=example,dc=com` |
| `LDAP.SyncUserPassword` | *(admin/service password)* |
| `LDAP.SearchBase` | `dc=example,dc=com` |
| `LDAP.SearchFilter` | `(objectClass=*)` |
| `LDAP.UserClass` | `inetOrgPerson` |
| `LDAP.GroupClass` | `groupOfNames` |
| `LDAP.Mapping.UserName` | `uid` |
| `LDAP.Mapping.GroupName` | `cn` |
| `LDAP.Mapping.FullName` | `displayName` |
| `LDAP.Mapping.Description` | `description` |
| `LDAP.Mapping.Email` | `mail` |
| `LDAP.UserUniqueId` | `entryUUID` |
| `LDAP.GroupUniqueId` | `entryUUID` |
| `LDAP.SyncInterval` | `1440` |
| `LDAP.UserDeleteAction` | `1` |

---

## Security Considerations

### Encrypted Connections

For production environments, use LDAPS (LDAP over TLS) to encrypt traffic between NetXMS and the LDAP server:

```
LDAP.ConnectionString = ldaps://ldap.example.com:636
```

This prevents credentials from being transmitted in plain text over the network.

### Service Account

- Create a dedicated service account with **read-only** access to the directory
- Limit the account's permissions to the minimum required search base
- Use a strong password and rotate it according to your organization's policy
- The service account password is stored encrypted in the NetXMS database

### Deleted User Handling

The recommended setting is `LDAP.UserDeleteAction = 1` (disable). This ensures:
- Removed LDAP users are disabled rather than deleted
- Audit trail is preserved
- Accidental deletion in LDAP doesn't permanently remove access records
- Disabled users can be manually reviewed and cleaned up

---

## Access Control (Authorization)

LDAP integration handles **authentication** (verifying identity). **Authorization** (what users can access) is managed within NetXMS:

1. **LDAP groups sync into NetXMS groups** -- group memberships are automatically maintained during each synchronization cycle
2. **Assign access rights to NetXMS groups** -- configure object access rights, system access rights, and other permissions on the synced LDAP groups
3. **Group membership changes in LDAP propagate automatically** -- when a user is added to or removed from an LDAP group, the corresponding NetXMS group membership updates on the next sync

### Recommended Workflow

1. Configure LDAP synchronization and run an initial sync
2. Identify the synced LDAP groups in NetXMS (they appear with an LDAP indicator)
3. Assign appropriate access rights to these groups (e.g., read access to specific object trees, dashboard access, alarm management rights)
4. Users logging in via LDAP inherit permissions from their group memberships

**Important**: Pre-existing NetXMS users (e.g., the built-in `admin` account) and groups (e.g., `Everyone`) are not modified by LDAP synchronization.

---

## Multiple Search Bases

If users and groups reside in different parts of the directory tree, specify multiple search bases separated by semicolons:

```
LDAP.SearchBase = OU=Users,DC=example,DC=com;OU=Groups,DC=example,DC=com;OU=ServiceAccounts,DC=example,DC=com
```

Use a backslash to include a literal semicolon: `\;`

---

## High Availability

### Multiple LDAP Servers

NetXMS supports listing multiple LDAP servers for failover:

```
LDAP.ConnectionString = ldap://dc01.example.com:389 ldap://dc02.example.com:389
```

If the first server is unreachable, NetXMS will attempt the next server in the list.

---

## Verification and Troubleshooting

### Initial Verification Steps

1. Set `LDAP.SyncInterval` to a non-zero value (e.g., `1`) for initial testing
2. Trigger a manual sync via the server debug console: **Tools > Server Console**, then type `ldapsync`
3. Check **Configuration > User Manager** to verify LDAP users and groups appear
4. Test login with an LDAP user account
5. Once verified, adjust `LDAP.SyncInterval` to the desired production value

### Enabling Debug Logging

For detailed LDAP diagnostics, set the server debug level to 4 or higher and filter by the `ldap` debug tag. Run the `ldap` command in the server debug console to see detailed synchronization output.

### Common Issues

**Users not appearing after sync:**
- Verify `LDAP.SearchBase` is correct and contains the target users
- Check that `LDAP.UserClass` matches the actual object class in your directory
- Ensure `LDAP.Mapping.UserName` points to a populated attribute
- Confirm the sync service account has read access to the search base

**Authentication fails for synced users:**
- Verify the user's DN is correctly stored (check User Manager > user properties)
- Ensure the LDAP server is reachable on the configured port
- Check that the user's password is correct in the LDAP directory
- Look for "Invalid credentials" in debug logs

**Sync account credentials incorrect:**
```
LDAPConnection::loginLDAP(): LDAP could not login. Error code: Invalid credentials
```
- Verify `LDAP.SyncUser` DN and `LDAP.SyncUserPassword`

**Search base not found:**
```
LDAPConnection::syncUsers(): LDAP could not get search results. Error code: No such object
```
- Check that `LDAP.SearchBase` DN exists in the directory

**Successful sync log output (for reference):**
```
LDAPConnection::initLDAP(): Connecting to LDAP server
LDAPConnection::syncUsers(): Found entry count: 3
LDAPConnection::syncUsers(): User added: dn: CN=jsmith,OU=Users,DC=example,DC=com, login name: jsmith
```

---

## Advanced Features

### LDAP Synchronization Hook Script

NetXMS supports a server-side script hook (`Hook::LDAPSynchronization`) that runs for each LDAP object during synchronization. This allows filtering or custom validation:

- The script receives an `$ldapObject` parameter with properties: `id`, `loginName`, `fullName`, `description`, `email`, `phoneNumber`, `type`, `isUser`, `isGroup`
- Return `true` to include the object, `false` to skip it

Use case examples:
- Exclude service accounts from synchronization
- Synchronize only users from specific departments
- Apply naming conventions before user creation

### Nested Group Support

NetXMS resolves nested group memberships. If Group A contains Group B, members of Group B are recognized as indirect members of Group A during synchronization.

### Large Directory Support

For directories with thousands of entries, NetXMS uses LDAP paged search controls. The `LDAP.PageSize` parameter (default 1000) controls the page size. This is automatically enabled and handles Active Directory's member attribute range extensions (e.g., `member;range=0-999`).

---

## Summary

| Capability | Supported |
|------------|-----------|
| Active Directory | Yes |
| OpenLDAP | Yes |
| LDAP over TLS (LDAPS) | Yes |
| Automatic user/group sync | Yes |
| Transparent user login | Yes |
| Nested group membership | Yes |
| Multiple LDAP servers (failover) | Yes |
| Multiple search bases | Yes |
| Paged search (large directories) | Yes |
| Custom sync filtering (hook scripts) | Yes |
| Two-way synchronization | No (one-way: LDAP to NetXMS) |
