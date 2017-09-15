/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.constants;

/**
 * This class holds constants related to network discovery
 */
public final class NetworkDiscovery
{
	public static final String FILTER_NONE = "none";
	public static final String FILTER_AUTO = "auto";
	
	public static final int FILTER_ALLOW_AGENT = 0x0001;
	public static final int FILTER_ALLOW_SNMP = 0x0002;
	public static final int FILTER_LIMIT_BY_RANGE = 0x0004;
}
