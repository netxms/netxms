/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.client.maps;

import org.netxms.base.NXCPMessage;

/**
 * Represents link between two elements on map
 */
public class NetworkMapLink
{
	// Link types
	public static final int NORMAL = 0;
	public static final int VPN = 1;
	public static final int MULTILINK = 2;
	
	// Routing types
	public static final int ROUTING_DEFAULT = 0;
	public static final int ROUTING_DIRECT = 1;
	public static final int ROUTING_MANHATTAN = 2;
	public static final int ROUTING_BENDPOINTS = 3;
	
	private String name;
	private int type;
	private long element1;
	private long element2;
	private String connectorName1;
	private String connectorName2;
	private int color;
	private long statusObject;
	private int routing;
	private long[] bendPoints;

	/**
	 * @param name
	 * @param type
	 * @param element1
	 * @param element2
	 * @param connectorName1
	 * @param connectorName2
	 */
	public NetworkMapLink(String name, int type, long element1, long element2, String connectorName1, String connectorName2)
	{
		this.name = name;
		this.type = type;
		this.element1 = element1;
		this.element2 = element2;
		this.connectorName1 = connectorName1;
		this.connectorName2 = connectorName2;
		this.color = -1;
		this.statusObject = 0;
		this.routing = ROUTING_DEFAULT;
		bendPoints = null;
	}

	/**
	 * @param linkType
	 * @param element1
	 * @param element2
	 * @param connectorName1
	 * @param connectorName2
	 */
	public NetworkMapLink(int type, long element1, long element2)
	{
		this.name = "";
		this.type = type;
		this.element1 = element1;
		this.element2 = element2;
		this.connectorName1 = "";
		this.connectorName2 = "";
		this.color = -1;
		this.statusObject = 0;
		this.routing = ROUTING_DEFAULT;
		bendPoints = null;
	}
	
	/**
	 * Create link object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public NetworkMapLink(NXCPMessage msg, long baseId)
	{
		name = msg.getVariableAsString(baseId + 1);
		type = msg.getVariableAsInteger(baseId);
		element1 = msg.getVariableAsInt64(baseId + 4);
		element2 = msg.getVariableAsInt64(baseId + 5);
		connectorName1 = msg.getVariableAsString(baseId + 2);
		connectorName2 = msg.getVariableAsString(baseId + 3);
		color = msg.getVariableAsInteger(baseId + 6);
		statusObject = msg.getVariableAsInt64(baseId + 7);
		routing = msg.getVariableAsInteger(baseId + 8);
		bendPoints = msg.getVariableAsUInt32Array(baseId + 9);
	}
	
	/**
	 * Fill NXCP message with link data
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{
		msg.setVariableInt16(baseId, type);
		msg.setVariable(baseId + 1, name);
		msg.setVariable(baseId + 2, connectorName1);
		msg.setVariable(baseId + 3, connectorName2);
		msg.setVariableInt32(baseId + 4, (int)element1);
		msg.setVariableInt32(baseId + 5, (int)element2);
		msg.setVariableInt32(baseId + 6, color);
		msg.setVariableInt32(baseId + 7, (int)statusObject);
		msg.setVariableInt16(baseId + 8, routing);
		msg.setVariable(baseId + 9, (bendPoints != null) ? bendPoints : new long[] { 0x7FFFFFFF, 0x7FFFFFFF });
	}

	/**
	 * @return the linkType
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return first (left) element
	 */
	public long getElement1()
	{
		return element1;
	}

	/**
	 * @return second (right) element
	 */
	public long getElement2()
	{
		return element2;
	}

	/**
	 * @return first (left) connector name
	 */
	public String getConnectorName1()
	{
		return connectorName1;
	}

	/**
	 * @return second (right) connector name
	 */
	public String getConnectorName2()
	{
		return connectorName2;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
	
	/**
	 * Get label for display. If name is not null and not empty, label will have form
	 * name (connector1 - connector2)
	 * otherwise it will have form
	 * connector1 - connector2
	 * If any of connector names is null or empty, it will be replaced with string "<noname>".
	 * 
	 * @return display label or null for unnamed link 
	 */
	public String getLabel()
	{
		if (isUnnamed())
			return null;
		
		StringBuilder sb = new StringBuilder();
		if ((name != null) && !name.isEmpty())
		{
			sb.append(name);
			sb.append(" (");
		}
		
		sb.append(((connectorName1 != null) && !connectorName1.isEmpty()) ? connectorName1 : "<noname>");
		sb.append(" - ");
		sb.append(((connectorName2 != null) && !connectorName2.isEmpty()) ? connectorName2 : "<noname>");

		if ((name != null) && !name.isEmpty())
		{
			sb.append(')');
		}
		return sb.toString();
	}
	
	/**
	 * Check if link has non-empty name
	 * 
	 * @return true if link has non-empty name
	 */
	public boolean hasName()
	{
		return (name != null) && !name.isEmpty();
	}
	
	/**
	 * Check if link has non-empty name for connector 1
	 * 
	 * @return true if link has non-empty name for connector 1
	 */
	public boolean hasConnectorName1()
	{
		return (connectorName1 != null) && !connectorName1.isEmpty();
	}

	/**
	 * Check if link has non-empty name for connector 2
	 * 
	 * @return true if link has non-empty name for connector 2
	 */
	public boolean hasConnectorName2()
	{
		return (connectorName2 != null) && !connectorName2.isEmpty();
	}
	
	/**
	 * Check if this link is unnamed.
	 * 
	 * @return true if all names (link and both connectors) are null or empty
	 */
	public boolean isUnnamed()
	{
		return ((name == null) || name.isEmpty()) &&
		       ((connectorName1 == null) || connectorName1.isEmpty()) &&
		       ((connectorName2 == null) || connectorName2.isEmpty());
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object obj)
	{
		if (obj instanceof NetworkMapLink)
			return (((NetworkMapLink)obj).element1 == this.element1) &&
			       (((NetworkMapLink)obj).element2 == this.element2);
		return super.equals(obj);
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode()
	{
		return (int)((element1 << 16) | (element2 & 0xFFFF));
	}

	/**
	 * @return the color
	 */
	public int getColor()
	{
		return color;
	}

	/**
	 * @param color the color to set
	 */
	public void setColor(int color)
	{
		this.color = color;
	}

	/**
	 * @return the statusObject
	 */
	public long getStatusObject()
	{
		return statusObject;
	}

	/**
	 * @param statusObject the statusObject to set
	 */
	public void setStatusObject(long statusObject)
	{
		this.statusObject = statusObject;
	}

	/**
	 * @return the routing
	 */
	public int getRouting()
	{
		return routing;
	}

	/**
	 * @param routing the routing to set
	 */
	public void setRouting(int routing)
	{
		this.routing = routing;
	}

	/**
	 * @return the bendPoints
	 */
	public long[] getBendPoints()
	{
		return bendPoints;
	}

	/**
	 * @param bendPoints the bendPoints to set
	 */
	public void setBendPoints(long[] bendPoints)
	{
		this.bendPoints = bendPoints;
	}
}
