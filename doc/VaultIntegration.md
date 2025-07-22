# HashiCorp Vault Integration

## Overview

NetXMS server supports native integration with HashiCorp Vault for secure credential management. This allows storing sensitive credentials like database passwords in Vault instead of the configuration file.

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

For KV v1:

```bash
vault write secret/netxms/database \
    login=netxms \
    password=mysecretpassword
```

## Secret Structure

The Vault path should contain a secret with the following fields:

- `login` - Database username (optional, uses config value if not present)
- `password` - Database password (required)

## Security Considerations

1. **AppRole Secret ID**: Can be encrypted using `nxencpassw`:
   ```bash
   nxencpassw netxms
   ```
   Then use the encrypted value in the configuration.

2. **TLS Verification**: Always enable TLS verification in production:
   ```
   [VAULT]
   TLSVerify = true
   ```

3. **Token Renewal**: NetXMS automatically handles token renewal based on the lease duration.

4. **Least Privilege**: Grant only read access to the specific paths NetXMS needs.

## Priority Order

NetXMS checks for database credentials in this order:

1. DBPasswordCommand (if configured)
2. Vault integration (if configured)
3. DBPassword/DBEncryptedPassword from config file

The first successful method is used.

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

# Vault configuration
[VAULT]
URL = https://vault.example.com:8200
AppRoleId = 5a7a7c45-d3d1-a0e8-5e44-5c91fcb78b44
AppRoleSecretId = AgBQ1SLH8hKsw2pOgNBvUJTZs+H6086um2nsq0mtOix4FA==
DBCredentialPath = secret/netxms/database
TLSVerify = true
Timeout = 5000

# Logging
LogFile = {syslog}
DebugLevel = 6
DebugTags = vault:5
```