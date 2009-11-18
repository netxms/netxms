/**
 * 
 */
package org.netxms.client.objects;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * @author Victor
 *
 */
public class AgentPolicyConfig extends AgentPolicy
{
	private String fileName;
	private String fileContent;

	/**
	 * @param msg
	 * @param session
	 */
	public AgentPolicyConfig(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		fileName = msg.getVariableAsString(NXCPCodes.VID_CONFIG_FILE_NAME);
		fileContent = msg.getVariableAsString(NXCPCodes.VID_CONFIG_FILE_DATA);
	}

	/**
	 * @return the fileName
	 */
	public String getFileName()
	{
		return fileName;
	}

	/**
	 * @return the fileContent
	 */
	public String getFileContent()
	{
		return fileContent;
	}
}
