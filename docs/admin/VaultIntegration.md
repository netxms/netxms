# HashiCorp Vault Integration

## Overview

NetXMS server supports native integration with HashiCorp Vault for secure credential management. This allows storing sensitive credentials like database passwords in Vault instead of the configuration file.

Database credentials are read from Vault once, when the server starts. `nxdbmgr` reads the same `[VAULT]` section from `netxmsd.conf` and retrieves the credentials on every run.

## Configuration Options

Add the following section to your `netxmsd.conf`:

```
[VAULT]
# Vault server URL
URL = https://vault.example.com:8200

# AppRole authentication
AppRoleId = netxms-role
AppRoleSecretId = <secret-id>

# Path to database credentials in Vault
DBCredentialPath = secret/netxms/database

# Optional settings
Timeout = 5000          # Timeout in milliseconds (default: 5000)
TLSVerify = true        # Verify TLS certificate (default: true)
```

## Vault Setup

### 1. Enable AppRole Authentication

```bash
vault auth enable approle
```

### 2. Create Policy for NetXMS

Create a policy file `netxms-policy.hcl`:

```hcl
path "secret/data/netxms/*" {
  capabilities = ["read"]
}
```

Apply the policy:

```bash
vault policy write netxms netxms-policy.hcl
```

### 3. Create AppRole

```bash
# Create the role
vault write auth/approle/role/netxms \
    token_policies="netxms" \
    token_ttl=1h \
    token_max_ttl=4h

# Get Role ID
vault read auth/approle/role/netxms/role-id

# Get Secret ID
vault write -f auth/approle/role/netxms/secret-id
```

### 4. Store Database Credentials

For KV v2 (default):

```bash
vault kv put secret/netxms/database \
    login=netxms \
    password=mysecretpassword
```

The server expects a KV v2 secrets engine: for `DBCredentialPath = secret/netxms/database` it reads `secret/data/netxms/database`, taking the first path component as the mount point and inserting `data/` after it. If the mount point name has more than one component, give the full API path including `data/` (for example `team/kv/data/netxms/database`); a path that already contains `/data/` is used as is.

On a KV v1 secrets engine the same request addresses the secret `data/netxms/database`, so the secret has to be stored under that name:

```bash
vault write secret/data/netxms/database \
    login=netxms \
    password=mysecretpassword
```

## Secret Structure

The Vault path should contain a secret with the following fields:

- `login` - Database username (optional, uses config value if not present)
- `password` - Database password (required)

## Security Considerations

1. **AppRole Secret ID**: Can be encrypted using `nxencpasswd` with login name `netxms`:
   ```bash
   nxencpasswd netxms
   ```
   The tool asks for the secret ID and prints the encrypted value; use that value as `AppRoleSecretId` in the configuration.

2. **TLS Verification**: Always enable TLS verification in production:
   ```
   [VAULT]
   TLSVerify = true
   ```

3. **Token Lifetime**: The Vault token obtained through AppRole is used only to read the credentials during startup and is not renewed, so a short `token_ttl` is sufficient. A password changed in Vault takes effect on the next server start.

4. **Least Privilege**: Grant only read access to the specific paths NetXMS needs.

## Priority Order

All configured sources are processed in this order, and each one that succeeds replaces the value obtained from the previous one:

1. DBPassword/DBEncryptedPassword from config file
2. DBPasswordCommand (if configured)
3. Vault integration (if configured)

Credentials from Vault therefore take precedence over the output of `DBPasswordCommand`, which takes precedence over the configuration file. If Vault is configured but authentication fails or the secret cannot be read, an error is logged and the password from the previous sources is used. Database login from Vault, when present, replaces `DBLogin`.

## Troubleshooting

Enable debug logging to see Vault operations:

```
DebugLevel = 7
DebugTags = config:5,vault:6
```

Common issues:

1. **Authentication failures**: Check Role ID and Secret ID
2. **Permission denied**: Verify the AppRole policy includes the correct path
3. **Connection timeout**: Check network connectivity and firewall rules
4. **TLS errors**: Verify certificate or set `TLSVerify = false` in [VAULT] section for testing

## Example Complete Configuration

```
# Database configuration
DBDriver = pgsql
DBServer = localhost
DBName = netxms_db
DBLogin = netxms

# Logging
LogFile = {syslog}
DebugLevel = 6
DebugTags = vault:5

# Vault configuration
[VAULT]
URL = https://vault.example.com:8200
AppRoleId = 5a7a7c45-d3d1-a0e8-5e44-5c91fcb78b44
AppRoleSecretId = AgBQ1SLH8hKsw2pOgNBvUJTZs+H6086um2nsq0mtOix4FA==
DBCredentialPath = secret/netxms/database
TLSVerify = true
Timeout = 5000
```