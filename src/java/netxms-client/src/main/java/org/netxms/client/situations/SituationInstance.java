/**
 * 
 */
package org.netxms.client.situations;

import java.util.HashMap;
import java.util.Map;
import org.netxms.base.NXCPMessage;

/**
 * Situation instance object
 *
 */
public class SituationInstance
{
	private String name;
	private Map<String, String> attributes;
	
	/**
	 * Create situation instance from message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable id
	 */
	protected SituationInstance(NXCPMessage msg, long baseId)
	{
		name = msg.getVariableAsString(baseId);
		
		int count = msg.getVariableAsInteger(baseId + 1);
		attributes = new HashMap<String, String>(count);
		long varId = baseId + 2;
		for(int i = 0; i < count; i++)
		{
			final String name = msg.getVariableAsString(varId++);
			final String value = msg.getVariableAsString(varId++);
			attributes.put(name, value);
		}
	}
	
	/**
	 * Get number of attributes
	 * 
	 * @return
	 */
	public int getAttributeCount()
	{
		return attributes.size();
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
	
	/**
	 * Get attribute's value
	 * 
	 * @param name attribute's name
	 * @return attribute's value
	 */
	public String getAttribute(String name)
	{
		return attributes.get(name);
	}

	/**
	 * @return the attributes
	 */
	public Map<String, String> getAttributes()
	{
		return attributes;
	}
}
