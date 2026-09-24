# Security Policy

## Reporting a Vulnerability

Please do not report security vulnerabilities through public GitHub issues, the forum, or Telegram.

Use one of these private channels instead:

- **GitHub private vulnerability reporting**: [Report a vulnerability](https://github.com/netxms/netxms/security/advisories/new)
- **Email**: [security@netxms.com](mailto:security@netxms.com)

Include as much of the following as you can:

- Affected component (server, agent, subagent, management console, WebAPI, client library, etc.) and version
- Type of issue (for example authentication bypass, injection, memory corruption, information disclosure)
- Steps to reproduce, or a proof-of-concept
- Impact assessment: what an attacker can achieve and under which preconditions (network position, required privileges)
- Any suggested fix or mitigation, if you have one

You will receive an acknowledgement within a few business days. We will keep you informed of the progress towards a fix and may ask for additional information.

## Coordinated Disclosure

We ask reporters to give us reasonable time to investigate and release a fix before any public disclosure. We will work with you to agree on a disclosure date.

When a fix is released, we publish a [GitHub Security Advisory](https://github.com/netxms/netxms/security/advisories) and note the fix in the [changelog](https://github.com/netxms/changelog/blob/master/ChangeLog.md). Reporters are credited in the advisory unless they prefer to remain anonymous.

## Supported Versions

Reports are accepted for any NetXMS version. Fixes are released for the currently maintained stable release series; if you are running an older release, the fix will require an upgrade.

## No Bug Bounty

NetXMS does not run a bug bounty program and does not pay for vulnerability reports. Reporters are credited in the advisory, and that is the only reward on offer.

## Scope

In scope: all code in this repository, including the server (`netxmsd`), agent (`nxagentd`) and subagents, management console (`nxmc`), WebAPI, client libraries, and supporting libraries.

Out of scope:

- Vulnerabilities in third-party dependencies that are not caused by how NetXMS uses them. Please report those to the upstream project.
- Issues that require an already fully compromised host or administrator account on the NetXMS server.
- Insecure deployments resulting from configuration choices that the documentation explicitly warns about.

If you are unsure whether something is in scope, report it anyway and we will make the call.
