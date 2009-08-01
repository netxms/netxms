/**
 * 
 */
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

/**
 * Represents data collection item's threshold
 * 
 * @author Victor Kirhenshtein
 *
 */
public class Threshold
{
	private long id;
	private int fireEvent;
	private int rearmEvent;
	private int arg1;
	private int arg2;
	private int function;
	private int operation;
	private int repeatInterval;
	private String value;
	
	protected Threshold(final NXCPMessage msg, final long baseId)
	{
		
	}
}
