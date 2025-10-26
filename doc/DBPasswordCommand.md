# DBPasswordCommand Configuration Option

## Overview

The `DBPasswordCommand` configuration option allows NetXMS to retrieve database passwords from external sources such as HashiCorp Vault, AWS Secrets Manager, or any custom script.

## Configuration

Add the following to your `netxmsd.conf`:

```
DBPasswordCommand = <command>
```

## How It Works

1. When NetXMS server starts, it executes the specified command
2. The command's standard output is captured and used as the database password
3. Trailing whitespace (newlines, spaces, tabs) is automatically trimmed
4. The command must complete within 30 seconds
5. If the command fails or times out, NetXMS will use the password from `DBPassword` or `DBEncryptedPassword` if specified

## Examples

### HashiCorp Vault

```
DBPasswordCommand = vault kv get -field=password database/netxms
```

### AWS Secrets Manager

```
DBPasswordCommand = aws secretsmanager get-secret-value --secret-id netxms/db --query SecretString --output text | jq -r .password
```

### Azure Key Vault

```
DBPasswordCommand = az keyvault secret show --name netxms-db-password --vault-name MyKeyVault --query value -o tsv
```

### Custom Script

```
DBPasswordCommand = /opt/netxms/scripts/get_db_password.sh
```

## Error Handling

- If the command exits with non-zero status, an error is logged
- If the command times out (30 seconds), it is terminated and an error is logged
- If the command returns empty output, a warning is logged
- In all error cases, NetXMS falls back to the statically configured password

## Security Considerations

1. Ensure the command and any scripts have appropriate permissions
2. The command is executed with the same privileges as the NetXMS server process
3. Avoid logging sensitive information in your scripts
4. Consider using environment variables or secure credential stores

## Debugging

Enable debug logging to see command execution details:

```
DebugLevel = 7
DebugTags = config:3
```

## Requirements

- NetXMS version 6.0.0 or later
- External command must be in PATH or use absolute path
- Command must output password to stdout (stderr is ignored)
