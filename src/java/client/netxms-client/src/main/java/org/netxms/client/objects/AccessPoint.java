/**
 * 
 */
package org.netxms.client.objects;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.MacAddress;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.AccessPointState;
import org.netxms.client.topology.RadioInterface;

/**
 * Access point class
 */
public class AccessPoint extends GenericObject
{
	private long nodeId;
	private MacAddress macAddress;
   private AccessPointState state;
	private String vendor;
	private String model;
	private String serialNumber;
	private RadioInterface[] radios;
	
	/**
	 * @param msg
	 * @param session
	 */
	public AccessPoint(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		nodeId = msg.getFieldAsInt64(NXCPCodes.VID_NODE_ID);
		macAddress = new MacAddress(msg.getFieldAsBinary(NXCPCodes.VID_MAC_ADDR));
		state = AccessPointState.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_STATE));
		vendor = msg.getFieldAsString(NXCPCodes.VID_VENDOR);
		model = msg.getFieldAsString(NXCPCodes.VID_MODEL);
		serialNumber = msg.getFieldAsString(NXCPCodes.VID_SERIAL_NUMBER);
		
		int count = msg.getFieldAsInt32(NXCPCodes.VID_RADIO_COUNT);
		radios = new RadioInterface[count];
		long varId = NXCPCodes.VID_RADIO_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			radios[i] = new RadioInterface(this, msg, varId);
			varId += 10;
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "AccessPoint";
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
	 */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
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

	/**
	 * @return the radios
	 */
	public RadioInterface[] getRadios()
	{
		return radios;
	}

   /**
    * @return the state
    */
   public AccessPointState getState()
   {
      return state;
   }
}
