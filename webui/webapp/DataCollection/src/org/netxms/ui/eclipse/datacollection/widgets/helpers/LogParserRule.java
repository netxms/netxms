/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
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
	
	@Element(required=false)
	private Integer id = null;

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
	   if(isSyslogParser)
      {
	      return LogParser.integerToString(severity);
      }
	   else
	      return LogParser.integerToString(level);
	}

	/**
	 * @param severity the severity to set
	 */
	public void setSeverityOrLevel(Integer severity)
	{
      if(editor.getParserType() == LogParserType.SYSLOG)
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
	   if(isSyslogParser)
	   {
	      return LogParser.integerToString(facility);
	   }
	   else
	      return LogParser.integerToString(id);
	}

	/**
	 * @param facility the facility to set
	 */
	public void setFacilityOrId(Integer facility)
	{
	   if(editor.getParserType() == LogParserType.SYSLOG)
      {
	      this.facility = facility;
	      this.id = null;
      }
	   else
	   {
	      this.id = facility;
	      this.facility = null;
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
      this.logName = logName;
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
		this.description = description == null || description.trim().isEmpty() ? null : description;
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

   public void updateFieldsCorrectly(LogParserType parserType)
   {
      if (parserType != LogParserType.POLICY)
      {
         if(facility == null || facility == 0)
            facility = id;
         if(tag == null || tag.isEmpty())
            tag = source;
         if(severity == null || severity == 0)
            severity = level;         
      }
      else
      {
         if(id == null || id == 0)
            id = facility;
         if(source == null || source.isEmpty())
            source = tag;
         if(level == null || level == 0)
            level = severity;          
      }
      
   }
}
