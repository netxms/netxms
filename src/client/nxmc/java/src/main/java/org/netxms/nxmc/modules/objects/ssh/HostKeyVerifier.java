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
package org.netxms.nxmc.modules.objects.ssh;

/**
 * Callback used by direct SSH transport to confirm host key of a remote system that is not yet in known hosts list.
 * Called from a non-UI thread.
 */
public interface HostKeyVerifier
{
   /**
    * Ask user to accept unknown host key.
    *
    * @param message message describing the host and key fingerprint
    * @return true if key should be accepted and remembered
    */
   public boolean confirmUnknownHostKey(String message);
}
