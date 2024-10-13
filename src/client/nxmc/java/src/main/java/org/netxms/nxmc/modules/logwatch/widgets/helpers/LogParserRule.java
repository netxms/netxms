/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.modules.logwatch.widgets.helpers;

import java.util.ArrayList;
import java.util.List;
import org.netxms.nxmc.modules.logwatch.widgets.LogParserRuleEditor;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;

/**
 * Log parser rule
 */
@Root(name="rule", strict=false)
public class LogParserRule
{
   @Attribute(required=false)
   private String name = null;

	@Attribute(required=false)
	private String context = null;

	@Attribute(name="break", required=false)
	private String breakProcessing = null;

   @Attribute(required=false)
   private String doNotSaveToDatabase = null;
	
	@Element(required=true)
	private LogParserMatch match = new LogParserMatch();
	
	@Element(required=false)
	private LogParserEvent event = null;

   //severity == level
   //severity in syslog\level for other logs	
	@Element(required=false)
	private Integer severity = null;

   @Element(required=false)
   private Integer level = null;

	//facility == id
   //facility in syslog\id for other logs
	@Element(required=false)
	private Integer facility = null;

   // Could be single ID or ID range in form nn-nn
	@Element(required=false)
   private String id = null;

   //source == tag
   //tag in syslog\source for other logs
	@Element(required = false)
	private String source = null;

	@Element(required=false)
	private String tag = null;
   
   @Element(required=false)
   private String logName = null;

	@Element(required=false)
	private String description = null;

	//Deprecated
   @Element(name="push", required=false)
   private LogParserPushDci pushDci = null;	

   @ElementList(required = false)
   private List<LogParserMetric> metrics = new ArrayList<LogParserMetric>(0);   
	
	@Element(name="context", required=false)
	private LogParserContext contextDefinition = null;

	@Element(required=false)
	private LogParserAgentAction agentAction = null;

	private LogParserRuleEditor editor;

	/**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
	 * @return the context
	 */
	public String getContext()
	{
		return context;
	}

	/**
	 * @param context the context to set
	 */
	public void setContext(String context)
	{
		this.context = context;
	}

	/**
	 * @return the breakProcessing
	 */
	public boolean isBreakProcessing()
	{
		return LogParser.stringToBoolean(breakProcessing);
	}

	/**
	 * @param breakProcessing the breakProcessing to set
	 */
	public void setBreakProcessing(boolean breakProcessing)
	{
		this.breakProcessing = LogParser.booleanToString(breakProcessing);
	}

   /**
    * @return the doNotSaveToDatabase
    */
   public boolean isDoNotSaveToDatabase()
   {
      return LogParser.stringToBoolean(doNotSaveToDatabase);
   }

   /**
    * @param doNotSave the doNotSaveToDatabase to set
    */
   public void setDoNotSaveToDatabase(boolean doNotSave)
   {
      this.doNotSaveToDatabase = LogParser.booleanToString(doNotSave);
   }

	/**
    * @return the matcher
    */
   public LogParserMatch getMatch()
   {
      return match;
   }

   /**
    * @param matcher the matcher to set
    */
   public void setMatch(LogParserMatch matcher)
   {
      this.match = matcher;
   }

   /**
	 * @return the event
	 */
	public LogParserEvent getEvent()
	{
		return event;
	}

	/**
	 * @param event the event to set
	 */
	public void setEvent(LogParserEvent event)
	{
		this.event = event;
	}

	/**
	 * @return the severity
	 */
	public String getSeverityOrLevel(boolean isSyslogParser)
	{
      return isSyslogParser ? integerToString(severity) : integerToString(level);
	}

	/**
	 * @param severity the severity to set
	 */
	public void setSeverityOrLevel(Integer severity)
	{
      if (editor.getParserType() == LogParserType.SYSLOG)
      {
         this.severity = severity;
         this.level = null;
      }
      else
      {
         this.level = severity;
         this.severity = null;
      }
	}

	/**
	 * @return the facility
	 */
	public String getFacilityOrId(boolean isSyslogParser)
	{
      return isSyslogParser ? integerToString(facility) : id;
	}

	/**
	 * @param facility the facility to set
	 */
   public void setFacility(Integer facility)
	{
      if (editor.getParserType() == LogParserType.SYSLOG)
      {
	      this.facility = facility;
	      this.id = null;
      }
	   else
	   {
         this.id = Integer.toString(facility);
	      this.facility = null;
	   }
	}

   /**
    * Set event ID.
    *
    * @param idDefinition event ID or range
    */
   public void setEventId(String idDefinition)
   {
      if (editor.getParserType() == LogParserType.SYSLOG)
      {
         try
         {
            facility = Integer.parseInt(idDefinition);
         }
         catch(NumberFormatException e)
         {
            facility = null;
         }
         id = null;
      }
      else
      {
         id = idDefinition;
         facility = null;
      }
   }

	/**
	 * @return the tag
	 */
	public String getTagOrSource(boolean isSyslogParser)
	{
      if(isSyslogParser)
      {
         return tag == null ? "" : tag;
      }
      else
         return source == null ? "" : source;
	}

	/**
	 * @param tag the tag to set
	 */
	public void setTagOrSource(String tag)
	{
      if(editor.getParserType() == LogParserType.SYSLOG)
      {
         this.tag = tag != null && !tag.isEmpty() ? tag : null;
         this.source = null;
      }
      else
      {
         this.tag = null;
         this.source = tag != null && !tag.isEmpty() ? tag : null;
      }
	}

	/**
    * @return the logName
    */
   public String getLogName()
   {
      return logName;
   }

   /**
    * @param logName the logName to set
    */
   public void setLogName(String logName)
   {
      this.logName = ((logName == null) || logName.trim().isEmpty()) ? null : logName;
   }

   /**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
      this.description = ((description == null) || description.trim().isEmpty()) ? null : description;
	}

	/**
	 * @return the contextDefinition
	 */
	public LogParserContext getContextDefinition()
	{
		return contextDefinition;
	}

	/**
	 * @param contextDefinition the contextDefinition to set
	 */
	public void setContextDefinition(LogParserContext contextDefinition)
	{
		this.contextDefinition = contextDefinition;
	}

	/**
	 * @return the editor
	 */
	public LogParserRuleEditor getEditor()
	{
		return editor;
	}

	/**
	 * @param editor the editor to set
	 */
	public void setEditor(LogParserRuleEditor editor)
	{
		this.editor = editor;
	}

	/**
	 * @return agent action
	 */
	public LogParserAgentAction getAgentAction()
	{
		return agentAction;
	}
	
	/**
	 * @param agentAction the agent action to set
	 */
	public void setAgentAction(String agentAction)
	{
		this.agentAction = new LogParserAgentAction(agentAction);
	}

   /**
    * Fix possible inconsistencies in field values according to parser type
    *
    * @param parserType
    */
   public void fixFieldsForType(LogParserType parserType)
   {
      if (parserType == LogParserType.SYSLOG)
      {
         if (((facility == null) || (facility == 0)) && (id != null) && !id.isEmpty())
         {
            try
            {
               facility = Integer.parseInt(id);
            }
            catch(NumberFormatException e)
            {
            }
         }
         if (tag == null || tag.isEmpty())
            tag = source;
         if (severity == null || severity == 0)
            severity = level;
         id = null;
         source = null;
         level = null;
      }
      else
      {
         if (((id == null) || id.isEmpty()) && (facility != null) && (facility != 0))
            id = facility.toString();
         if (source == null || source.isEmpty())
            source = tag;
         if (level == null || level == 0)
            level = severity;
         facility = null;
         tag = null;
         severity = null;
         
         if (pushDci != null && !pushDci.getData().isEmpty())
         {
            LogParserMetric metric = new LogParserMetric();
            metric.setMetric(pushDci.getData());
            metric.setPush(true);
            metric.setGroup(pushDci.getGroup());
            metrics.add(metric);
            pushDci = null;
         }
      }
   }
   
   /**
    * @return the pushDci
    */
   public LogParserPushDci getPushDci()
   {
      return pushDci;
   }

   /**
    * @return the metrics
    */
   public List<LogParserMetric> getMetrics()
   {
      return metrics;
   }

   /**
    * @param metrics the metrics to set
    */
   public void setMetrics(List<LogParserMetric> metrics)
   {
      this.metrics = metrics;
   }

   /**
    * Null-safe Integer.toString()
    *
    * @param n integer value or null
    * @return string value
    */
   private static String integerToString(Integer n)
   {
      return (n != null) ? n.toString() : "";
   }
}
