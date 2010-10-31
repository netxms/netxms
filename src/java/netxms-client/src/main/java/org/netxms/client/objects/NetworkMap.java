/**
 * 
 */
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapElement;

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
			elements.add(new NetworkMapElement(msg, varId));
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
