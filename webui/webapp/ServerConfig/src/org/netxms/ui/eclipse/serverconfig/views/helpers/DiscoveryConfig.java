/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import org.netxms.client.InetAddressListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerVariable;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Class which holds all elements of network discovery configuration
 */
public class DiscoveryConfig
{
   public final static int DISCOVERY_TYPE_NONE = 0;
   public final static int DISCOVERY_TYPE_PASIVE = 1;
   public final static int DISCOVERY_TYPE_ACTIVE = 2;
   public final static int DISCOVERY_TYPE_ACTIVE_PASIVE = 3;
   
   
	private int discoveryType;
	private boolean useSnmpTraps;
	private boolean useSyslog;
	private int filterFlags;
	private String filter;
	private List<InetAddressListElement> targets;
	private List<InetAddressListElement> addressFilter;
	
	/**
	 * Create empty object
	 */
	private DiscoveryConfig()
	{
	}
	
	/**
	 * Load discovery configuration from server. This method directly calls
	 * communication API, so it should not be called from UI thread.
	 * @param session 
	 * 
	 * @return network discovery configuration
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public static DiscoveryConfig load(NXCSession session) throws NXCException, IOException
	{
		DiscoveryConfig config = new DiscoveryConfig();
		
		Map<String, ServerVariable> variables = session.getServerVariables();
		
		config.discoveryType = getInteger(variables, "NetworkDiscovery.Type", 0);
		config.useSnmpTraps = getBoolean(variables, "UseSNMPTrapsForDiscovery", false); //$NON-NLS-1$
      config.useSyslog = getBoolean(variables, "UseSyslogForDiscovery", false); //$NON-NLS-1$
		config.filterFlags = getInteger(variables, "DiscoveryFilterFlags", 0); //$NON-NLS-1$
		config.filter = getString(variables, "DiscoveryFilter", "none"); //$NON-NLS-1$ //$NON-NLS-2$
		
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
		if (v == null)
			return defVal;
		return v.getValue();
	}
	
	/**
	 * Save discovery configuration on server. This method calls communication
	 * API directly, so it should not be called from UI thread.
	 * 
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void save(NXCSession session) throws NXCException, IOException
	{
		session.setServerVariable("NetworkDiscovery.Type", Integer.toString(discoveryType)); //$NON-NLS-1$ 	
      session.setServerVariable("UseSNMPTrapsForDiscovery", useSnmpTraps ? "1" : "0"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
      session.setServerVariable("UseSyslogForDiscovery", useSyslog ? "1" : "0"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
		session.setServerVariable("DiscoveryFilterFlags", Integer.toString(filterFlags)); //$NON-NLS-1$
		session.setServerVariable("DiscoveryFilter", filter); //$NON-NLS-1$
		
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
	 * @return the filter
	 */
	public String getFilter()
	{
		return filter;
	}

	/**
	 * @param filter the filter to set
	 */
	public void setFilter(String filter)
	{
		this.filter = filter;
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
    * @param useSnmpTraps the useSnmpTraps to set
    */
   public void setUseSnmpTraps(boolean useSnmpTraps)
   {
      this.useSnmpTraps = useSnmpTraps;
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
}
