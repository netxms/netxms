/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.client.log;

import org.netxms.base.NXCPMessage;

/**
 * Log column definition
 */
public class LogColumn
{
	// Column types
	public static final int LC_TEXT              = 0;
	public static final int LC_SEVERITY          = 1;
	public static final int LC_OBJECT_ID         = 2;
	public static final int LC_USER_ID           = 3;
	public static final int LC_EVENT_CODE        = 4;
	public static final int LC_TIMESTAMP         = 5;
	public static final int LC_INTEGER           = 6;
	public static final int LC_ALARM_STATE       = 7;
	public static final int LC_ALARM_HD_STATE    = 8;
   public static final int LC_ZONE_UIN          = 9;
   public static final int LC_EVENT_ORIGIN      = 10;
   public static final int LC_TEXT_DETAILS      = 11;
   public static final int LC_JSON_DETAILS      = 12;
   public static final int LC_COMPLETION_STATUS = 13;
   public static final int LC_ACTION_CODE       = 14;
   public static final int LC_ATM_TXN_CODE      = 15;
   public static final int LC_ASSET_OPERATION   = 16;

   public static final int LCF_TSDB_TIMESTAMPTZ = 0x0001;   /* Column is of timestamptz data type in TimescaleDB */
   public static final int LCF_CHAR_COLUMN      = 0x0002;   /* Column is of char type */
   public static final int LCF_RECORD_ID        = 0x0004;   /* Column is a sequential record ID */

	private String name;
	private String description;
	private int type;
   private int flags;

	/**
	 * Create log column object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	protected LogColumn(NXCPMessage msg, long baseId)
	{
		name = msg.getFieldAsString(baseId);
		type = msg.getFieldAsInt32(baseId + 1);
		description = msg.getFieldAsString(baseId + 2);
      flags = msg.getFieldAsInt32(baseId + 3);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

   /**
    * Get column flags.
    *
    * @return column flags
    */
   public int getFlags()
   {
      return flags;
   }
}
