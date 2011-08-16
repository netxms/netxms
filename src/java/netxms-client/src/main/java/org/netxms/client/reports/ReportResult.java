/**
 * 
 */
package org.netxms.client.reports;

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Result of report execution
 */
public class ReportResult
{
	private long jobId;
	private Date executionTime;
	
	/**
	 * Create report result object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public ReportResult(NXCPMessage msg, long baseId)
	{
		jobId = msg.getVariableAsInt64(baseId);
		executionTime = msg.getVariableAsDate(baseId + 1);
	}

	/**
	 * @return the jobId
	 */
	public long getJobId()
	{
		return jobId;
	}

	/**
	 * @return the executionTime
	 */
	public Date getExecutionTime()
	{
		return executionTime;
	}
}
