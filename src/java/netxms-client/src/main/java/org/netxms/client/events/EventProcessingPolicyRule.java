/**
 * 
 */
package org.netxms.client.events;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * This class represents single rule of event processing policy.
 * 
 * @author Victor
 */
public class EventProcessingPolicyRule
{
	// Rule flags (options)
	public static final int STOP_PROCESSING     = 0x0001;
	public static final int NEGATED_SOURCE      = 0x0002;
	public static final int NEGATED_EVENTS      = 0x0004;
	public static final int GENERATE_ALARM      = 0x0008;
	public static final int DISABLED            = 0x0010;
	public static final int TERMINATE_BY_REGEXP = 0x0020;
	public static final int SEVERITY_NORMAL     = 0x0100;
	public static final int SEVERITY_WARNING    = 0x0200;
	public static final int SEVERITY_MINOR      = 0x0400;
	public static final int SEVERITY_MAJOR      = 0x0800;
	public static final int SEVERITY_CRITICAL   = 0x1000;

	public static final int SEVERITY_ANY = SEVERITY_NORMAL | SEVERITY_WARNING | SEVERITY_MINOR | SEVERITY_MAJOR | SEVERITY_CRITICAL;
	
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
	 * Fill NXCP message with rule's data
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		msg.setVariableInt32(NXCPCodes.VID_FLAGS, flags);
		msg.setVariable(NXCPCodes.VID_COMMENTS, comments);
		msg.setVariable(NXCPCodes.VID_SCRIPT, script);
		
		msg.setVariableInt32(NXCPCodes.VID_NUM_ACTIONS, actions.size());
		msg.setVariable(NXCPCodes.VID_RULE_ACTIONS, actions.toArray(new Long[actions.size()]));
		
		msg.setVariableInt32(NXCPCodes.VID_NUM_EVENTS, events.size());
		msg.setVariable(NXCPCodes.VID_RULE_EVENTS, events.toArray(new Long[events.size()]));
		
		msg.setVariableInt32(NXCPCodes.VID_NUM_SOURCES, sources.size());
		msg.setVariable(NXCPCodes.VID_RULE_SOURCES, sources.toArray(new Long[sources.size()]));
		
		msg.setVariable(NXCPCodes.VID_ALARM_KEY, alarmKey);
		msg.setVariable(NXCPCodes.VID_ALARM_MESSAGE, alarmMessage);
		msg.setVariableInt16(NXCPCodes.VID_ALARM_SEVERITY, alarmSeverity);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_TIMEOUT, alarmTimeout);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_TIMEOUT_EVENT, (int)alarmTimeoutEvent);

		msg.setVariableInt32(NXCPCodes.VID_SITUATION_ID, (int)situationId);
		msg.setVariable(NXCPCodes.VID_SITUATION_INSTANCE, situationInstance);
		msg.setVariableInt32(NXCPCodes.VID_SITUATION_NUM_ATTRS, situationAttributes.size());
		long varId = NXCPCodes.VID_SITUATION_ATTR_LIST_BASE;
		for(Entry<String, String> e : situationAttributes.entrySet())
		{
			msg.setVariable(varId++, e.getKey());
			msg.setVariable(varId++, e.getValue());
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

	/**
	 * @return the script
	 */
	public String getScript()
	{
		return script;
	}

	/**
	 * @param script the script to set
	 */
	public void setScript(String script)
	{
		this.script = script;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
	}

	/**
	 * @return the alarmKey
	 */
	public String getAlarmKey()
	{
		return alarmKey;
	}

	/**
	 * @param alarmKey the alarmKey to set
	 */
	public void setAlarmKey(String alarmKey)
	{
		this.alarmKey = alarmKey;
	}

	/**
	 * @return the alarmMessage
	 */
	public String getAlarmMessage()
	{
		return alarmMessage;
	}

	/**
	 * @param alarmMessage the alarmMessage to set
	 */
	public void setAlarmMessage(String alarmMessage)
	{
		this.alarmMessage = alarmMessage;
	}

	/**
	 * @return the alarmSeverity
	 */
	public int getAlarmSeverity()
	{
		return alarmSeverity;
	}

	/**
	 * @param alarmSeverity the alarmSeverity to set
	 */
	public void setAlarmSeverity(int alarmSeverity)
	{
		this.alarmSeverity = alarmSeverity;
	}

	/**
	 * @return the alarmTimeout
	 */
	public int getAlarmTimeout()
	{
		return alarmTimeout;
	}

	/**
	 * @param alarmTimeout the alarmTimeout to set
	 */
	public void setAlarmTimeout(int alarmTimeout)
	{
		this.alarmTimeout = alarmTimeout;
	}

	/**
	 * @return the alarmTimeoutEvent
	 */
	public long getAlarmTimeoutEvent()
	{
		return alarmTimeoutEvent;
	}

	/**
	 * @param alarmTimeoutEvent the alarmTimeoutEvent to set
	 */
	public void setAlarmTimeoutEvent(long alarmTimeoutEvent)
	{
		this.alarmTimeoutEvent = alarmTimeoutEvent;
	}

	/**
	 * @return the situationId
	 */
	public long getSituationId()
	{
		return situationId;
	}

	/**
	 * @param situationId the situationId to set
	 */
	public void setSituationId(long situationId)
	{
		this.situationId = situationId;
	}

	/**
	 * @return the situationInstance
	 */
	public String getSituationInstance()
	{
		return situationInstance;
	}

	/**
	 * @param situationInstance the situationInstance to set
	 */
	public void setSituationInstance(String situationInstance)
	{
		this.situationInstance = situationInstance;
	}

	/**
	 * @return the sources
	 */
	public List<Long> getSources()
	{
		return sources;
	}

	/**
	 * @return the events
	 */
	public List<Long> getEvents()
	{
		return events;
	}

	/**
	 * @return the actions
	 */
	public List<Long> getActions()
	{
		return actions;
	}

	/**
	 * @return the situationAttributes
	 */
	public Map<String, String> getSituationAttributes()
	{
		return situationAttributes;
	}

	/**
	 * @param sources the sources to set
	 */
	public void setSources(List<Long> sources)
	{
		this.sources = sources;
	}

	/**
	 * @param events the events to set
	 */
	public void setEvents(List<Long> events)
	{
		this.events = events;
	}

	/**
	 * @param actions the actions to set
	 */
	public void setActions(List<Long> actions)
	{
		this.actions = actions;
	}

	/**
	 * @param situationAttributes the situationAttributes to set
	 */
	public void setSituationAttributes(Map<String, String> situationAttributes)
	{
		this.situationAttributes = situationAttributes;
	}
}
