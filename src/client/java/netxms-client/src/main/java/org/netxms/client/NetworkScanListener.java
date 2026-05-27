/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.client;

/**
 * Listener for streaming results of an interactive network range scan.
 * The same host may produce multiple callbacks as additional protocols are
 * detected; each delivered result reflects the latest cumulative state for
 * that address.
 */
public interface NetworkScanListener
{
   /**
    * Called for each per-host update received from the server.
    *
    * @param result the host record snapshot
    */
   void resultReceived(NetworkScanResult result);
}
