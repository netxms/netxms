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

import java.io.InputStream;
import java.io.OutputStream;

/**
 * Transport for interactive terminal session: raw PTY byte stream in both directions plus window size control.
 */
public interface TerminalTransport
{
   /**
    * Establish session. Blocks until session is established or fails.
    *
    * @param columns initial terminal width in columns
    * @param rows initial terminal height in rows
    * @throws Exception on any failure
    */
   public void connect(int columns, int rows) throws Exception;

   /**
    * Get stream for reading terminal output (data from remote system). Read returns -1 when session is closed.
    *
    * @return input stream
    */
   public InputStream getInputStream();

   /**
    * Get stream for writing terminal input (keystrokes sent to remote system).
    *
    * @return output stream
    */
   public OutputStream getOutputStream();

   /**
    * Notify remote system about terminal window size change.
    *
    * @param columns new width in columns
    * @param rows new height in rows
    */
   public void resize(int columns, int rows);

   /**
    * Close session and release all resources. Safe to call multiple times.
    */
   public void close();
}
