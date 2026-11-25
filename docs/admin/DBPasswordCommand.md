# DBPasswordCommand Configuration Option

## Overview

The `DBPasswordCommand` configuration option allows NetXMS to retrieve database passwords from external sources such as HashiCorp Vault, AWS Secrets Manager, or any custom script.

## Configuration

Add the following to your `netxmsd.conf`:

```
DBPasswordCommand = <command>
```

The option is read by both the server (`netxmsd`) and the database manager (`nxdbmgr`).

## How It Works

1. When NetXMS server starts (or `nxdbmgr` is run), it executes the specified command through the system shell, so pipes and redirections can be used
2. The command's output is captured and used as the database password. Standard output and standard error are captured together, so the command must print the password and nothing else
3. Trailing whitespace (newlines, spaces, tabs) is automatically trimmed
4. The command must complete within 30 seconds
5. If the command fails, times out, or prints nothing, NetXMS will use the password from `DBPassword` or `DBEncryptedPassword` if specified
6. If [HashiCorp Vault integration](VaultIntegration.md) is also configured, it is processed after the command, and a password retrieved from Vault replaces the one returned by the command

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
- Anything the command writes to standard error becomes part of the password when the command exits with status 0. Redirect diagnostic output away (`2>/dev/null` on UNIX, `2>NUL` on Windows) if the tool can print warnings on success

## Security Considerations

1. Ensure the command and any scripts have appropriate permissions
2. The command is executed with the same privileges as the NetXMS server process
3. Avoid logging sensitive information in your scripts
4. The command line is written to the server log (and printed by `nxdbmgr`) each time it is executed, so do not put tokens or other secrets on the command line itself
5. Consider using environment variables or secure credential stores

## Debugging

The server reports command execution in its log with the tag `config`: an informational message when the command is started and when the password is retrieved, a warning for empty output, and an error for a failed start, non-zero exit status or timeout. These are regular log messages and do not depend on the debug level. `nxdbmgr` prints the same messages to the console.

## Requirements

- NetXMS version 6.0.0 or later
- External command must be in PATH or use absolute path
- Command must output only the password (stdout and stderr are both captured)
- Password must not be longer than 255 characters
