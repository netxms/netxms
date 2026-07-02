/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.tools;

import java.io.File;

/**
 * Helpers for interacting with the host environment when the client itself runs inside a sandbox (currently Flatpak).
 */
public final class SandboxHelper
{
   private static final boolean flatpak = new File("/.flatpak-info").exists();

   /**
    * Private constructor to forbid instantiation
    */
   private SandboxHelper()
   {
   }

   /**
    * Build argument vector for running a shell command line on the host machine. When the client runs inside a Flatpak sandbox the
    * command is routed to the host via flatpak-spawn so that it can reach host tools (terminal emulators, ssh, etc.); otherwise it is
    * executed directly. This requires the "org.freedesktop.Flatpak" talk-name permission to be granted in the Flatpak manifest.
    *
    * @param commandLine shell command line to execute
    * @return argument vector suitable for {@link Runtime#exec(String[])}
    */
   public static String[] buildHostShellCommand(String commandLine)
   {
      return flatpak
            ? new String[] { "flatpak-spawn", "--host", "/bin/sh", "-c", commandLine }
            : new String[] { "/bin/sh", "-c", commandLine };
   }
}
