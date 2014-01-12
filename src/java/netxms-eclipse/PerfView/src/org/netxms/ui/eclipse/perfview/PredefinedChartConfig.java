/**
 * 
 */
package org.netxms.ui.eclipse.perfview;

import java.util.ArrayList;
import java.util.List;
import org.netxms.client.AccessListElement;
import org.netxms.client.datacollection.GraphSettings;

/**
 * Chart config for predefined charts
 */
public class PredefinedChartConfig extends ChartConfig
{
	private long id;
	private long ownerId;
	private String name;
	private List<AccessListElement> accessList;
	
	/**
	 * Create predefined chart configuration from server-side configuration object.
	 * 
	 * @param serverConfig server-side configuration object
	 * @return
	 * @throws Exception
	 */
	public static PredefinedChartConfig createFromServerConfig(GraphSettings serverConfig) throws Exception
	{
		PredefinedChartConfig config = (PredefinedChartConfig)internalCreate(PredefinedChartConfig.class, serverConfig.getConfig());
		config.id = serverConfig.getId();
		config.ownerId = serverConfig.getOwnerId();
		config.name = serverConfig.getName();
		config.accessList.addAll(serverConfig.getAccessList());
		return config;
	}
	
	/**
	 * Default constructor
	 */
	public PredefinedChartConfig()
	{
		id = 0;
		ownerId = 0;
		name = Messages.get().PredefinedChartConfig_Unnamed;
		accessList = new ArrayList<AccessListElement>();
	}

	/**
	 * Create server-side settings object ready to be sent to server.
	 * 
	 * @return
	 * @throws Exception 
	 */
	public GraphSettings createServerSettings() throws Exception
	{
		GraphSettings settings = new GraphSettings(id, ownerId, accessList);
		settings.setName(name);
		settings.setConfig(createXml());
		return settings;
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @param id the id to set
	 */
	public void setId(long id)
	{
		this.id = id;
	}

	/**
	 * @return the ownerId
	 */
	public long getOwnerId()
	{
		return ownerId;
	}

	/**
	 * @param ownerId the ownerId to set
	 */
	public void setOwnerId(long ownerId)
	{
		this.ownerId = ownerId;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the accessList
	 */
	public List<AccessListElement> getAccessList()
	{
		return accessList;
	}
}
