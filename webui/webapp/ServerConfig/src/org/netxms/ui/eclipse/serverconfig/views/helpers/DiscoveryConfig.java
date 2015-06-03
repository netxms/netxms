/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import org.netxms.client.IpAddressListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerVariable;
import org.netxms.client.snmp.SnmpUsmCredential;

/**
 * Class which holds all elements of network discovery configuration
 */
public class DiscoveryConfig
{
	private boolean enabled;
	private boolean active;
	private boolean useSnmpTraps;
	private int filterFlags;
	private String filter;
	private String defaultCommunity;
	private List<IpAddressListElement> targets;
	private List<IpAddressListElement> addressFilter;
	private List<String> communities;
	private List<SnmpUsmCredential> usmCredentials;
	
	/**
	 * Create empty object
	 */
	private DiscoveryConfig()
	{
	}
	
	/**
	 * Load discovery configuration from server. This method directly calls
	 * communication API, so it should not be called from UI thread.
	 * 
	 * @return network discovery configuration
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public static DiscoveryConfig load(NXCSession session) throws NXCException, IOException
	{
		DiscoveryConfig config = new DiscoveryConfig();
		
		Map<String, ServerVariable> variables = session.getServerVariables();
		
		config.enabled = getBoolean(variables, "RunNetworkDiscovery", false); //$NON-NLS-1$
		config.active = getBoolean(variables, "ActiveNetworkDiscovery", false); //$NON-NLS-1$
		config.useSnmpTraps = getBoolean(variables, "UseSNMPTrapsForDiscovery", false); //$NON-NLS-1$
		config.filterFlags = getInteger(variables, "DiscoveryFilterFlags", 0); //$NON-NLS-1$
		config.filter = getString(variables, "DiscoveryFilter", "none"); //$NON-NLS-1$ //$NON-NLS-2$
		config.defaultCommunity = getString(variables, "DefaultCommunityString", "public"); //$NON-NLS-1$ //$NON-NLS-2$
		
		config.addressFilter = session.getAddressList(NXCSession.ADDRESS_LIST_DISCOVERY_FILTER);
		config.targets = session.getAddressList(NXCSession.ADDRESS_LIST_DISCOVERY_TARGETS);
		config.communities = session.getSnmpCommunities();
		config.usmCredentials = session.getSnmpUsmCredentials();
		
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
		session.setServerVariable("RunNetworkDiscovery", enabled ? "1" : "0"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
		session.setServerVariable("ActiveNetworkDiscovery", active ? "1" : "0"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
      session.setServerVariable("UseSNMPTrapsForDiscovery", useSnmpTraps ? "1" : "0"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
		session.setServerVariable("DiscoveryFilterFlags", Integer.toString(filterFlags)); //$NON-NLS-1$
		session.setServerVariable("DiscoveryFilter", filter); //$NON-NLS-1$
		session.setServerVariable("DefaultCommunityString", defaultCommunity); //$NON-NLS-1$
		
		session.setAddressList(NXCSession.ADDRESS_LIST_DISCOVERY_FILTER, addressFilter);
		session.setAddressList(NXCSession.ADDRESS_LIST_DISCOVERY_TARGETS, targets);
		session.updateSnmpCommunities(communities);
		session.updateSnmpUsmCredentials(usmCredentials);
		
		session.resetServerComponent(NXCSession.SERVER_COMPONENT_DISCOVERY_MANAGER);
	}

	/**
	 * @return the enabled
	 */
	public boolean isEnabled()
	{
		return enabled;
	}

	/**
	 * @param enabled the enabled to set
	 */
	public void setEnabled(boolean enabled)
	{
		this.enabled = enabled;
	}

	/**
	 * @return the active
	 */
	public boolean isActive()
	{
		return active;
	}

	/**
	 * @param active the active to set
	 */
	public void setActive(boolean active)
	{
		this.active = active;
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
	 * @return the defaultCommunity
	 */
	public String getDefaultCommunity()
	{
		return defaultCommunity;
	}

	/**
	 * @param defaultCommunity the defaultCommunity to set
	 */
	public void setDefaultCommunity(String defaultCommunity)
	{
		this.defaultCommunity = defaultCommunity;
	}

	/**
	 * @return the targets
	 */
	public List<IpAddressListElement> getTargets()
	{
		return targets;
	}

	/**
	 * @param targets the targets to set
	 */
	public void setTargets(List<IpAddressListElement> targets)
	{
		this.targets = targets;
	}

	/**
	 * @return the addressFilter
	 */
	public List<IpAddressListElement> getAddressFilter()
	{
		return addressFilter;
	}

	/**
	 * @param addressFilter the addressFilter to set
	 */
	public void setAddressFilter(List<IpAddressListElement> addressFilter)
	{
		this.addressFilter = addressFilter;
	}

	/**
	 * @return the communities
	 */
	public List<String> getCommunities()
	{
		return communities;
	}

	/**
	 * @param communities the communities to set
	 */
	public void setCommunities(List<String> communities)
	{
		this.communities = communities;
	}

	/**
	 * @return the usmCredentials
	 */
	public List<SnmpUsmCredential> getUsmCredentials()
	{
		return usmCredentials;
	}

	/**
	 * @param usmCredentials the usmCredentials to set
	 */
	public void setUsmCredentials(List<SnmpUsmCredential> usmCredentials)
	{
		this.usmCredentials = usmCredentials;
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
}
