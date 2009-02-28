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
	
	// Data type
	public static final int DT_INT = 0;
	public static final int DT_UINT = 1;
	public static final int DT_INT64 = 2;
	public static final int DT_UINT64 = 3;
	public static final int DT_STRING = 4;
	public static final int DT_FLOAT = 5;
	public static final int DT_NULL = 6;
}
