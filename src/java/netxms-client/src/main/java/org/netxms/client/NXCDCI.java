/**
 * 
 */
package org.netxms.client;

/**
 * @author Victor
 *
 */
public class NXCDCI
{
	// Data sources
	public static final int INTERNAL = 0;
	public static final int AGENT = 1;
	public static final int SNMP = 2;
	public static final int CHECKPOINT_SNMP = 3;
	public static final int PUSH = 4;
	
	// Status
	public static final int ACTIVE = 0;
	public static final int DISABLED = 1;
	public static final int NOT_SUPPORTED = 2;
}
