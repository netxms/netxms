/**
 * 
 */
package org.netxms.client;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class NXCAgentPolicyConfig extends NXCAgentPolicy
{
	private String fileName;
	private String fileContent;

	/**
	 * @param msg
	 * @param session
	 */
	public NXCAgentPolicyConfig(NXCPMessage msg, NXCSession session)
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
