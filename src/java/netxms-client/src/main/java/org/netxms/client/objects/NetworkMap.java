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
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.elements.NetworkMapElement;

/**
 * Network map object
 *
 */
public class NetworkMap extends GenericObject
{
	private int mapType;
	private int layout;
	private int background;
	private long seedObjectId;
	private List<NetworkMapElement> elements;
	
	/**
	 * @param msg
	 * @param session
	 */
	public NetworkMap(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		mapType = msg.getVariableAsInteger(NXCPCodes.VID_MAP_TYPE);
		layout = msg.getVariableAsInteger(NXCPCodes.VID_LAYOUT);
		background = msg.getVariableAsInteger(NXCPCodes.VID_BACKGROUND);
		seedObjectId = msg.getVariableAsInt64(NXCPCodes.VID_SEED_OBJECT);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_ELEMENTS);
		elements = new ArrayList<NetworkMapElement>(count);
		long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			elements.add(NetworkMapElement.createMapElement(msg, varId));
			varId += 100;
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "NetworkMap";
	}

	/**
	 * @return the mapType
	 */
	public int getMapType()
	{
		return mapType;
	}

	/**
	 * @return the layout
	 */
	public int getLayout()
	{
		return layout;
	}

	/**
	 * @return the background
	 */
	public int getBackground()
	{
		return background;
	}

	/**
	 * @return the seedObjectId
	 */
	public long getSeedObjectId()
	{
		return seedObjectId;
	}
}
