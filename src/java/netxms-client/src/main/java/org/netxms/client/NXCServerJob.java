/**
 * 
 */
package org.netxms.client;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Represents server-side job
 * 
 * @author Victor
 *
 */
public class NXCServerJob
{
	// Job status codes
	public static final int PENDING = 0;
	public static final int ACTIVE = 1;
	public static final int ON_HOLD = 2;
	public static final int COMPLETED = 3;
	public static final int FAILED = 4;	
	public static final int CANCELLED = 5;
	public static final int CANCEL_PENDING = 6;
	
	private long id;
	private long nodeId;
	private String jobType;
	private String description;
	private int status;
	private int progress;
	private String failureMessage;
	
	/**
	 * Create job object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseVarId Base variable ID for job's data
	 */
	NXCServerJob(final NXCPMessage msg, final long baseVarId)
	{
		long var = baseVarId;
		
		id = msg.getVariableAsInt64(var++);
		jobType = msg.getVariableAsString(var++);
		description = msg.getVariableAsString(var++);
		nodeId = msg.getVariableAsInt64(var++);
		status = msg.getVariableAsInteger(var++);
		progress = msg.getVariableAsInteger(var++);
		failureMessage = msg.getVariableAsString(var++);
	}
	
	/**
	 * Create job object from notification message
	 */
	NXCServerJob(final NXCPMessage msg)
	{
		id = msg.getVariableAsInt64(NXCPCodes.VID_JOB_ID);
		jobType = msg.getVariableAsString(NXCPCodes.VID_JOB_TYPE);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
		nodeId = msg.getVariableAsInt64(NXCPCodes.VID_OBJECT_ID);
		status = msg.getVariableAsInteger(NXCPCodes.VID_JOB_STATUS);
		progress = msg.getVariableAsInteger(NXCPCodes.VID_JOB_PROGRESS);
		failureMessage = msg.getVariableAsString(NXCPCodes.VID_FAILURE_MESSAGE);
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the jobType
	 */
	public String getJobType()
	{
		return jobType;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the status
	 */
	public int getStatus()
	{
		return status;
	}

	/**
	 * @return the progress
	 */
	public int getProgress()
	{
		return progress;
	}

	/**
	 * @return the failureMessage
	 */
	public String getFailureMessage()
	{
		return failureMessage;
	}
}
