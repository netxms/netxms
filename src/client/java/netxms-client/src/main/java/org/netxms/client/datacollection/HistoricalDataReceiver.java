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
package org.netxms.client.datacollection;

/**
 * Callback interface for incremental (streaming) historical data retrieval. Allows the caller to
 * receive collected DCI data page by page (newest to oldest) instead of waiting for the entire data
 * set to be loaded, and to abort loading early.
 */
public interface HistoricalDataReceiver
{
   /**
    * Called on the calling thread after each page of data has been appended to the series. The same
    * cumulative {@link DataSeries} object is passed on every call and keeps growing between calls, so
    * implementations that retain it across threads must take a snapshot (via the {@link DataSeries}
    * copy constructor).
    *
    * @param data cumulative data received so far
    * @return true to continue loading, false to abort loading
    */
   public boolean dataReceived(DataSeries data);
}
