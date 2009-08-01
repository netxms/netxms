/**
 * 
 */
package org.netxms.client.epp;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * This class represents single rule of event processing policy.
 * 
 * @author Victor
 */
public class EventProcessingPolicyRule
{
	private List<Long> sources;
	private List<Long> events;
	private String script;
	private int flags;
	private String alarmKey;
	private String alarmMessage;
	private int alarmSeverity;
	private int alarmTimeout;
	private long alarmTimeoutEvent;
	private List<Long> actions;
	private long situationId;
	private String situationInstance;
	private Map<String, String> situationAttributes;
	private String comments;
	
	/**
	 * Create rule from NXCP message.
	 * 
	 * @param msg NXCP message
	 */
	public EventProcessingPolicyRule(NXCPMessage msg)
	{
		sources = Arrays.asList(msg.getVariableAsUInt32ArrayEx(NXCPCodes.VID_RULE_SOURCES));
		events = Arrays.asList(msg.getVariableAsUInt32ArrayEx(NXCPCodes.VID_RULE_EVENTS));
		script = msg.getVariableAsString(NXCPCodes.VID_SCRIPT);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		alarmKey = msg.getVariableAsString(NXCPCodes.VID_ALARM_KEY);
		alarmMessage = msg.getVariableAsString(NXCPCodes.VID_ALARM_MESSAGE);
		alarmSeverity = msg.getVariableAsInteger(NXCPCodes.VID_ALARM_SEVERITY);
		alarmTimeout = msg.getVariableAsInteger(NXCPCodes.VID_ALARM_TIMEOUT);
		alarmTimeoutEvent = msg.getVariableAsInt64(NXCPCodes.VID_ALARM_TIMEOUT_EVENT);
		actions = Arrays.asList(msg.getVariableAsUInt32ArrayEx(NXCPCodes.VID_RULE_ACTIONS));
		situationId = msg.getVariableAsInt64(NXCPCodes.VID_SITUATION_ID);
		situationInstance = msg.getVariableAsString(NXCPCodes.VID_SITUATION_INSTANCE);
		comments = msg.getVariableAsString(NXCPCodes.VID_COMMENTS);
		
		int numAttrs = msg.getVariableAsInteger(NXCPCodes.VID_SITUATION_NUM_ATTRS);
		situationAttributes = new HashMap<String, String>(numAttrs);
		long varId = NXCPCodes.VID_SITUATION_ATTR_LIST_BASE;
		for(int i = 0; i < numAttrs; i++)
		{
			final String attr = msg.getVariableAsString(varId++); 
			final String value = msg.getVariableAsString(varId++);
			situationAttributes.put(attr, value);
		}
	}

	/**
	 * Get rule's comments.
	 * 
	 * @return Rule's comments
	 */
	public String getComments()
	{
		return comments;
	}

	/**
	 * Set rule's comments.
	 * 
	 * @param comments New comments
	 */
	public void setComments(String comments)
	{
		this.comments = comments;
	}
}
