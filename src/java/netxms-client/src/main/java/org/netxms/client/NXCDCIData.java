/**
 * 
 */
package org.netxms.client;

import java.util.ArrayList;

/**
 * Class to hold series of collected DCI data
 * 
 * @author Victor
 *
 */
public class NXCDCIData
{
	private long nodeId;
	private long dciId;
	private ArrayList<NXCDCIDataRow> values = new ArrayList<NXCDCIDataRow>();
	
	
	/**
	 * @param nodeId
	 * @param dciId
	 */
	public NXCDCIData(long nodeId, long dciId)
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
	public NXCDCIDataRow[] getValues()
	{
		return values.toArray(new NXCDCIDataRow[values.size()]);
	}
	
	
	/**
	 * Get last added value
	 * 
	 * @return last added value
	 */
	public NXCDCIDataRow getLastValue()
	{
		return (values.size() > 0) ? values.get(values.size() - 1) : null;
	}
	
	
	/**
	 * Add new value
	 */
	public void addDataRow(NXCDCIDataRow row)
	{
		values.add(row);
	}
}
