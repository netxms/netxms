/**
 * 
 */
package org.netxms.client.datacollection;

import java.util.ArrayList;


/**
 * Class to hold series of collected DCI data
 * 
 * @author Victor
 *
 */
public class DciData
{
	private long nodeId;
	private long dciId;
	private ArrayList<DciDataRow> values = new ArrayList<DciDataRow>();
	
	
	/**
	 * @param nodeId
	 * @param dciId
	 */
	public DciData(long nodeId, long dciId)
	{
		this.nodeId = nodeId;
		this.dciId = dciId;
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
	 * @return the values
	 */
	public DciDataRow[] getValues()
	{
		return values.toArray(new DciDataRow[values.size()]);
	}
	
	
	/**
	 * Get last added value
	 * 
	 * @return last added value
	 */
	public DciDataRow getLastValue()
	{
		return (values.size() > 0) ? values.get(values.size() - 1) : null;
	}
	
	
	/**
	 * Add new value
	 */
	public void addDataRow(DciDataRow row)
	{
		values.add(row);
	}
}
