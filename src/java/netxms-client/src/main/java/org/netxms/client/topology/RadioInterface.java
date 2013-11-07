/**
 * 
 */
package org.netxms.client.topology;

import org.netxms.base.NXCPMessage;
import org.netxms.client.MacAddress;
import org.netxms.client.objects.AccessPoint;

/**
 * Radio interface information
 */
public class RadioInterface
{
	private AccessPoint accessPoint;
	private int index;
	private String name;
	private MacAddress macAddress;
	private int channel;
	private int powerDBm;
	private int powerMW;
	
	/**
	 * Create radio interface object from NXCP message
	 * 
	 * @param msg
	 * @param baseId
	 */
	public RadioInterface(AccessPoint ap, NXCPMessage msg, long baseId)
	{
		accessPoint = ap;
		index = msg.getVariableAsInteger(baseId);
		name = msg.getVariableAsString(baseId + 1);
		macAddress = new MacAddress(msg.getVariableAsBinary(baseId + 2));
		channel = msg.getVariableAsInteger(baseId + 3);
		powerDBm = msg.getVariableAsInteger(baseId + 4);
		powerMW = msg.getVariableAsInteger(baseId + 5);
	}

	/**
	 * @return the index
	 */
	public int getIndex()
	{
		return index;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the macAddress
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

	/**
	 * @return the channel
	 */
	public int getChannel()
	{
		return channel;
	}

	/**
	 * Get transmitting power in dBm
	 * 
	 * @return the powerDBm
	 */
	public int getPowerDBm()
	{
		return powerDBm;
	}

	/**
	 * Get transmitting power in milliwatts
	 * 
	 * @return the powerMW
	 */
	public int getPowerMW()
	{
		return powerMW;
	}

	/**
	 * @return the accessPoint
	 */
	public AccessPoint getAccessPoint()
	{
		return accessPoint;
	}
}
