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

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Helpers for interacting with the host environment when the client itself runs inside a sandbox (currently Flatpak).
 */
public final class SandboxHelper
{
   private static final String DOCUMENTATION_URL = "https://go.netxms.org/flatpak";

   private static final boolean flatpak = new File("/.flatpak-info").exists();
   private static final boolean hostAccessAvailable;
   private static final String applicationId;

   static
   {
      if (flatpak)
      {
         FlatpakInfo info = parseFlatpakInfo(new File("/.flatpak-info"));
         hostAccessAvailable = info.hostAccessAvailable;
         applicationId = info.applicationId;
      }
      else
      {
         hostAccessAvailable = true;
         applicationId = null;
      }
   }

   /**
    * Private constructor to forbid instantiation
    */
   private SandboxHelper()
   {
   }

   /**
    * Parsed subset of /.flatpak-info relevant to host command execution.
    */
   static final class FlatpakInfo
   {
      final boolean hostAccessAvailable;
      final String applicationId;

      FlatpakInfo(boolean hostAccessAvailable, String applicationId)
      {
         this.hostAccessAvailable = hostAccessAvailable;
         this.applicationId = applicationId;
      }
   }

   /**
    * Parse the given /.flatpak-info keyfile to determine whether host command execution is permitted and to obtain the Flatpak
    * application ID. Because /.flatpak-info reflects the sandbox's <em>effective</em> permissions, host command execution is
    * considered available when any of the following grants the ability to reach flatpak-spawn's "org.freedesktop.Flatpak" service:
    * a "[Session Bus Policy]" entry of "talk" or "own" for "org.freedesktop.Flatpak" or a matching wildcard (e.g.
    * "org.freedesktop.*"), or unfiltered session bus access via "[Context] sockets=...;session-bus". Anything else, a missing
    * section/key, or a read error is treated as "no host access" (fail closed) so that a restricted Flathub build surfaces an
    * actionable error instead of failing silently.
    *
    * @param file /.flatpak-info file to parse
    * @return parsed host access flag and application ID (application ID is {@code null} if the "[Application] name" key is absent)
    */
   static FlatpakInfo parseFlatpakInfo(File file)
   {
      boolean hostAccess = false;
      String appId = null;
      String section = null;
      try (BufferedReader reader = new BufferedReader(new FileReader(file)))
      {
         String line;
         while ((line = reader.readLine()) != null)
         {
            line = line.trim();
            if (line.isEmpty() || (line.charAt(0) == '#') || (line.charAt(0) == ';'))
               continue;
            if ((line.charAt(0) == '[') && (line.charAt(line.length() - 1) == ']'))
            {
               section = line.substring(1, line.length() - 1).trim();
               continue;
            }
            int eq = line.indexOf('=');
            if (eq < 0)
               continue;
            String key = line.substring(0, eq).trim();
            String value = line.substring(eq + 1).trim();
            if ("Application".equals(section) && "name".equals(key))
            {
               appId = value;
            }
            else if ("Session Bus Policy".equals(section) && sessionBusPolicyGrantsFlatpak(key, value))
            {
               hostAccess = true;
            }
            else if ("Context".equals(section) && "sockets".equals(key) && socketListGrantsSessionBus(value))
            {
               hostAccess = true;
            }
         }
      }
      catch(IOException | SecurityException e)
      {
         hostAccess = false;
      }
      return new FlatpakInfo(hostAccess, appId);
   }

   /**
    * Check whether a "[Session Bus Policy]" entry grants access to flatpak-spawn's "org.freedesktop.Flatpak" service. The policy
    * value must be "talk" or "own", and the name must be "org.freedesktop.Flatpak" exactly or a wildcard prefix that covers it
    * (Flatpak/xdg-dbus-proxy match a trailing ".*", e.g. "org.freedesktop.*" or "org.*").
    *
    * @param key session bus policy name
    * @param value session bus policy value
    * @return {@code true} if the entry grants access to "org.freedesktop.Flatpak"
    */
   private static boolean sessionBusPolicyGrantsFlatpak(String key, String value)
   {
      if (!"talk".equals(value) && !"own".equals(value))
         return false;
      if ("org.freedesktop.Flatpak".equals(key) || "*".equals(key))
         return true;
      return key.endsWith(".*") && "org.freedesktop.Flatpak".startsWith(key.substring(0, key.length() - 1));
   }

   /**
    * Check whether a "[Context] sockets" value list grants unfiltered session bus access, which lets flatpak-spawn reach the host
    * without the "org.freedesktop.Flatpak" session bus policy. The value is a ";"-separated list (e.g. "x11;wayland;session-bus").
    *
    * @param value semicolon-separated socket list
    * @return {@code true} if "session-bus" is present
    */
   private static boolean socketListGrantsSessionBus(String value)
   {
      for(String socket : value.split(";"))
      {
         if ("session-bus".equals(socket.trim()))
            return true;
      }
      return false;
   }

   /**
    * Build argument vector for running a shell command line on the host machine. When the client runs inside a Flatpak sandbox the
    * command is routed to the host via flatpak-spawn so that it can reach host tools (terminal emulators, ssh, etc.); otherwise it is
    * executed directly. Host command execution requires the "org.freedesktop.Flatpak" talk-name permission. The Flathub build ships
    * without it (Flathub rejects that permission), so on a restricted sandbox this method throws an {@link IOException} carrying an
    * actionable message: the exact {@code flatpak override} command to unlock the feature plus a documentation link. Callers already
    * handle {@code IOException} from {@link Runtime#exec(String[])}, so the error surfaces through their existing error path.
    *
    * @param commandLine shell command line to execute
    * @return argument vector suitable for {@link Runtime#exec(String[])}
    * @throws IOException if the client runs in a restricted Flatpak sandbox without host command execution permission
    */
   public static String[] buildHostShellCommand(String commandLine) throws IOException
   {
      if (flatpak && !hostAccessAvailable)
      {
         I18n i18n = LocalizationHelper.getI18n(SandboxHelper.class);
         throw new IOException(i18n.tr(
               "Cannot execute command on host machine: this Flatpak build runs in a restricted sandbox. " +
               "To enable host command execution, run:\nflatpak override --user --talk-name=org.freedesktop.Flatpak {0}\n" +
               "and restart the application. See {1} for details.",
               ((applicationId != null) && !applicationId.isEmpty()) ? applicationId : "<application-id>", DOCUMENTATION_URL));
      }
      return flatpak
            ? new String[] { "flatpak-spawn", "--host", "/bin/sh", "-c", commandLine }
            : new String[] { "/bin/sh", "-c", commandLine };
   }
}
