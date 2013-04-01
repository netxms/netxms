/**
 * 
 */
package org.netxms.client.objects;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.MacAddress;
import org.netxms.client.NXCSession;

/**
 * Access point class
 */
public class AccessPoint extends GenericObject
{
	private long nodeId;
	private MacAddress macAddress;
	private String vendor;
	private String model;
	private String serialNumber;
	
	/**
	 * @param msg
	 * @param session
	 */
	public AccessPoint(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		nodeId = msg.getVariableAsInt64(NXCPCodes.VID_NODE_ID);
		macAddress = new MacAddress(msg.getVariableAsBinary(NXCPCodes.VID_MAC_ADDR));
		vendor = msg.getVariableAsString(NXCPCodes.VID_VENDOR);
		model = msg.getVariableAsString(NXCPCodes.VID_MODEL);
		serialNumber = msg.getVariableAsString(NXCPCodes.VID_SERIAL_NUMBER);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "AccessPoint";
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the macAddress
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

	/**
	 * @return the vendor
	 */
	public String getVendor()
	{
		return vendor;
	}

	/**
	 * @return the serialNumber
	 */
	public String getSerialNumber()
	{
		return serialNumber;
	}

	/**
	 * @return the model
	 */
	public String getModel()
	{
		return model;
	}
}
