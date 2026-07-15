/*
** NetXMS - Network Management System
** Drivers for HPE (Hewlett Packard Enterprise) devices
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: hpe.cpp
**
**/

#include "hpe.h"

/**
 * Get SSH driver hints for interactive CLI sessions on Comware-based devices
 */
void ComwareDeviceDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // Comware prompt patterns:
   // - User view: <hostname>
   // - System view: [hostname]
   // There is no separate privileged mode; SSH users get their configured privilege level at login
   hints->promptPattern = "^[<\\[][\\w.-]+[>\\]]\\s*$";
   hints->enabledPromptPattern = "^[<\\[][\\w.-]+[>\\]]\\s*$";

   // No privilege escalation ("super" requires interactive password entry and is normally not needed)
   hints->enableCommand = nullptr;
   hints->enablePromptPattern = nullptr;

   // Pagination control
   hints->paginationDisableCmd = "screen-length disable";
   hints->paginationPrompt = "---- More ----|--More--";
   hints->paginationContinue = " ";

   // Exit command
   hints->exitCommand = "quit";

   // Test command for verifying command mode support
   hints->testCommand = "display version";
   hints->testCommandPattern = "Comware";

   // Timeouts
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}

/**
 * Check if config backup is supported
 */
bool ComwareDeviceDriver::isConfigBackupSupported()
{
   return true;
}

/**
 * Get running configuration via interactive SSH
 */
bool ComwareDeviceDriver::getRunningConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("display current-configuration", output);
}

/**
 * Get startup configuration via interactive SSH
 */
bool ComwareDeviceDriver::getStartupConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("display saved-configuration", output);
}
