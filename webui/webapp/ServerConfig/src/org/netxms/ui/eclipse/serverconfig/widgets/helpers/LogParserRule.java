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
package org.netxms.ui.eclipse.serverconfig.widgets.helpers;

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
	private String context = null;

	@Attribute(name="break", required=false)
	private Boolean breakProcessing = null;
	
	@Element(required=false)
	private String match = ".*"; //$NON-NLS-1$
	
	@Element(required=false)
	private LogParserEvent event = null;
	
	@Element(required=false)
	private Integer severity = null;
	
	@Element(required=false)
	private Integer facility = null;
	
	@Element(required=false)
	private String tag = null;

	@Element(required=false)
	private String description = null;
	
	@Element(name="context", required=false)
	private LogParserContext contextDefinition = null;
	
	private LogParserRuleEditor editor;

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
		return (breakProcessing != null) ? breakProcessing : false;
	}

	/**
	 * @param breakProcessing the breakProcessing to set
	 */
	public void setBreakProcessing(boolean breakProcessing)
	{
		this.breakProcessing = breakProcessing ? Boolean.TRUE : null;
	}

	/**
	 * @return the match
	 */
	public String getMatch()
	{
		return match;
	}

	/**
	 * @param match the match to set
	 */
	public void setMatch(String match)
	{
		this.match = match;
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
	public Integer getSeverity()
	{
		return severity;
	}

	/**
	 * @param severity the severity to set
	 */
	public void setSeverity(Integer severity)
	{
		this.severity = severity;
	}

	/**
	 * @return the facility
	 */
	public Integer getFacility()
	{
		return facility;
	}

	/**
	 * @param facility the facility to set
	 */
	public void setFacility(Integer facility)
	{
		this.facility = facility;
	}

	/**
	 * @return the tag
	 */
	public String getTag()
	{
		return tag;
	}

	/**
	 * @param tag the tag to set
	 */
	public void setTag(String tag)
	{
		this.tag = tag;
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
		this.description = description;
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
}
