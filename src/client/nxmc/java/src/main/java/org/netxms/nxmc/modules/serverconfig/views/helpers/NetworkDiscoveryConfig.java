/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import org.netxms.client.InetAddressListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerVariable;

/**
 * Class which holds all elements of network discovery configuration
 */
public class NetworkDiscoveryConfig
{
   public final static int DISCOVERY_TYPE_NONE = 0;
   public final static int DISCOVERY_TYPE_PASSIVE = 1;
   public final static int DISCOVERY_TYPE_ACTIVE = 2;
   public final static int DISCOVERY_TYPE_ACTIVE_PASSIVE = 3;

   public final static int DEFAULT_ACTIVE_INTERVAL = 7200;

   private NXCSession session;
	private int discoveryType;
   private boolean enableSNMPProbing;
   private boolean enableTCPProbing;
	private boolean useSnmpTraps;
	private boolean useSyslog;
	private int filterFlags;
	private String filterScript;
   private int passiveDiscoveryPollInterval;
   private int activeDiscoveryPollInterval;
   private String activeDiscoveryPollSchedule;
	private List<InetAddressListElement> targets;
	private List<InetAddressListElement> addressFilter;

	/**
	 * Create empty object
	 */
   private NetworkDiscoveryConfig(NXCSession session)
	{
      this.session = session;
	}
	
	/**
    * Load discovery configuration from server. This method directly calls communication API, so it should not be called from UI
    * thread.
    * 
    * @param session session to use
    * @return network discovery configuration
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public static NetworkDiscoveryConfig load(NXCSession session) throws NXCException, IOException
	{
      NetworkDiscoveryConfig config = new NetworkDiscoveryConfig(session);

		Map<String, ServerVariable> variables = session.getServerVariables();

		config.discoveryType = getInteger(variables, "NetworkDiscovery.Type", 0);
      config.enableSNMPProbing = getBoolean(variables, "NetworkDiscovery.ActiveDiscovery.EnableSNMPProbing", true);
      config.enableTCPProbing = getBoolean(variables, "NetworkDiscovery.ActiveDiscovery.EnableTCPProbing", false);
      config.useSnmpTraps = getBoolean(variables, "NetworkDiscovery.UseSNMPTraps", false);
      config.useSyslog = getBoolean(variables, "NetworkDiscovery.UseSyslog", false);
      config.filterFlags = getInteger(variables, "NetworkDiscovery.Filter.Flags", 0);
      config.filterScript = getString(variables, "NetworkDiscovery.Filter.Script", "none");
      config.passiveDiscoveryPollInterval = getInteger(variables, "NetworkDiscovery.PassiveDiscovery.Interval", 900);
      config.activeDiscoveryPollInterval = getInteger(variables, "NetworkDiscovery.ActiveDiscovery.Interval", DEFAULT_ACTIVE_INTERVAL);
      config.activeDiscoveryPollSchedule = getString(variables, "NetworkDiscovery.ActiveDiscovery.Schedule", "");

		config.addressFilter = session.getAddressList(NXCSession.ADDRESS_LIST_DISCOVERY_FILTER);
		config.targets = session.getAddressList(NXCSession.ADDRESS_LIST_DISCOVERY_TARGETS);

		return config;
	}

	/**
	 * Get boolean value from server variables
	 * 
	 * @param variables
	 * @param name
	 * @param defVal
	 * @return
	 */
	private static boolean getBoolean(Map<String, ServerVariable> variables, String name, boolean defVal)
	{
		ServerVariable v = variables.get(name);
		if (v == null)
			return defVal;

		try
		{
			return Integer.parseInt(v.getValue()) != 0;
		}
		catch(NumberFormatException e)
		{
			return defVal;
		}
	}

	/**
	 * Get integer value from server variables
	 * 
	 * @param variables
	 * @param name
	 * @param defVal
	 * @return
	 */
	private static int getInteger(Map<String, ServerVariable> variables, String name, int defVal)
	{
		ServerVariable v = variables.get(name);
		if (v == null)
			return defVal;

		try
		{
			return Integer.parseInt(v.getValue());
		}
		catch(NumberFormatException e)
		{
			return defVal;
		}
	}

	/**
	 * Get string value from server variables
	 * 
	 * @param variables
	 * @param name
	 * @param defVal
	 * @return
	 */
	private static String getString(Map<String, ServerVariable> variables, String name, String defVal)
	{
		ServerVariable v = variables.get(name);
      return (v != null) ? v.getValue() : defVal;
	}

	/**
	 * Save discovery configuration on server. This method calls communication
	 * API directly, so it should not be called from UI thread.
	 * 
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void save() throws NXCException, IOException
	{
      session.setServerVariable("NetworkDiscovery.Type", Integer.toString(discoveryType));
      session.setServerVariable("NetworkDiscovery.ActiveDiscovery.EnableSNMPProbing", enableSNMPProbing ? "1" : "0");
      session.setServerVariable("NetworkDiscovery.ActiveDiscovery.EnableTCPProbing", enableTCPProbing ? "1" : "0");
      session.setServerVariable("NetworkDiscovery.UseSNMPTraps", useSnmpTraps ? "1" : "0");
      session.setServerVariable("NetworkDiscovery.UseSyslog", useSyslog ? "1" : "0");
      session.setServerVariable("NetworkDiscovery.Filter.Flags", Integer.toString(filterFlags));
      session.setServerVariable("NetworkDiscovery.Filter.Script", filterScript);
      session.setServerVariable("NetworkDiscovery.PassiveDiscovery.Interval", Integer.toString(passiveDiscoveryPollInterval));
      session.setServerVariable("NetworkDiscovery.ActiveDiscovery.Interval", Integer.toString(activeDiscoveryPollInterval));
      session.setServerVariable("NetworkDiscovery.ActiveDiscovery.Schedule", activeDiscoveryPollSchedule);

		session.setAddressList(NXCSession.ADDRESS_LIST_DISCOVERY_FILTER, addressFilter);
		session.setAddressList(NXCSession.ADDRESS_LIST_DISCOVERY_TARGETS, targets);

		session.resetServerComponent(NXCSession.SERVER_COMPONENT_DISCOVERY_MANAGER);
	}

	/**
	 * @return the filterFlags
	 */
	public int getFilterFlags()
	{
		return filterFlags;
	}

	/**
	 * @param filterFlags the filterFlags to set
	 */
	public void setFilterFlags(int filterFlags)
	{
		this.filterFlags = filterFlags;
	}

	/**
    * Get filter script name.
    *
    * @return filter script name
    */
	public String getFilterScript()
	{
		return filterScript;
	}

	/**
    * Set filter script name.
    *
    * @param filterScript new filter script name
    */
	public void setFilterScript(String filterScript)
	{
		this.filterScript = filterScript;
	}

	/**
	 * @return the targets
	 */
	public List<InetAddressListElement> getTargets()
	{
		return targets;
	}

	/**
	 * @param targets the targets to set
	 */
	public void setTargets(List<InetAddressListElement> targets)
	{
		this.targets = targets;
	}

	/**
	 * @return the addressFilter
	 */
	public List<InetAddressListElement> getAddressFilter()
	{
		return addressFilter;
	}

	/**
	 * @param addressFilter the addressFilter to set
	 */
	public void setAddressFilter(List<InetAddressListElement> addressFilter)
	{
		this.addressFilter = addressFilter;
	}

   /**
    * @return the useSnmpTraps
    */
   public boolean isUseSnmpTraps()
   {
      return useSnmpTraps;
   }

   /**
    * @return the enableSNMPProbing
    */
   public boolean isEnableSNMPProbing()
   {
      return enableSNMPProbing;
   }

   /**
    * @param enableSNMPProbing the enableSNMPProbing to set
    */
   public void setEnableSNMPProbing(boolean enableSNMPProbing)
   {
      this.enableSNMPProbing = enableSNMPProbing;
   }

   /**
    * @param useSnmpTraps the useSnmpTraps to set
    */
   public void setUseSnmpTraps(boolean useSnmpTraps)
   {
      this.useSnmpTraps = useSnmpTraps;
   }

   /**
    * @return the enableTCPProbing
    */
   public boolean isEnableTCPProbing()
   {
      return enableTCPProbing;
   }

   /**
    * @param enableTCPProbing the enableTCPProbing to set
    */
   public void setEnableTCPProbing(boolean enableTCPProbing)
   {
      this.enableTCPProbing = enableTCPProbing;
   }

   /**
    * @return the useSyslog
    */
   public boolean isUseSyslog()
   {
      return useSyslog;
   }

   /**
    * @param useSyslog the useSyslog to set
    */
   public void setUseSyslog(boolean useSyslog)
   {
      this.useSyslog = useSyslog;
   }

   /**
    * @return the discoveryType
    */
   public int getDiscoveryType()
   {
      return discoveryType;
   }

   /**
    * @param discoveryType the discoveryType to set
    */
   public void setDiscoveryType(int discoveryType)
   {
      this.discoveryType = discoveryType;
   }

   /**
    * @return the activeDiscoveryPollInterval
    */
   public int getActiveDiscoveryPollInterval()
   {
      return activeDiscoveryPollInterval;
   }

   /**
    * @param activeDiscoveryPollInterval the activeDiscoveryPollInterval to set
    */
   public void setActiveDiscoveryPollInterval(int activeDiscoveryPollInterval)
   {
      this.activeDiscoveryPollInterval = activeDiscoveryPollInterval;
   }

   /**
    * @return the activeDiscoveryPollSchedule
    */
   public String getActiveDiscoveryPollSchedule()
   {
      return activeDiscoveryPollSchedule;
   }

   /**
    * @param activeDiscoveryPollSchedule the activeDiscoveryPollSchedule to set
    */
   public void setActiveDiscoveryPollSchedule(String activeDiscoveryPollSchedule)
   {
      this.activeDiscoveryPollSchedule = activeDiscoveryPollSchedule;
   }

   /**
    * @return the passiveDiscoveryPollInterval
    */
   public int getPassiveDiscoveryPollInterval()
   {
      return passiveDiscoveryPollInterval;
   }

   /**
    * @param passiveDiscoveryPollInterval the passiveDiscoveryPollInterval to set
    */
   public void setPassiveDiscoveryPollInterval(int passiveDiscoveryPollInterval)
   {
      this.passiveDiscoveryPollInterval = passiveDiscoveryPollInterval;
   }
}
