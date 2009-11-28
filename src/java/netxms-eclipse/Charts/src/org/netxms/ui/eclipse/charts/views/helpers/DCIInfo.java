package org.netxms.ui.eclipse.charts.views.helpers;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

/**
 * Helper class to hold source DCI information
 */
public class DCIInfo
{
	private long nodeId;
	private long dciId;
	private String description;
	
	public DCIInfo(long nodeId, long dciId, String description)
	{
		super();
		this.nodeId = nodeId;
		this.dciId = dciId;
		try
		{
			this.description = URLDecoder.decode(description, "UTF-8");
		}
		catch(UnsupportedEncodingException e)
		{
			this.description = "<internal error>";
		}
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the dciId
	 */
	public long getDciId()
	{
		return dciId;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}
}