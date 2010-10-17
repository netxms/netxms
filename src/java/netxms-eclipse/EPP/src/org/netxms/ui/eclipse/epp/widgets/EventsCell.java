/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.widgets;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.eclipse.jface.action.IMenuManager;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.epp.widgets.helpers.ImageFactory;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Cell containing events
 *
 */
public class EventsCell extends ListCell
{
	private EventProcessingPolicyRule eppRule;
	private NXCSession session;
	
	public EventsCell(Rule rule, Object data)
	{
		super(rule, data);
		eppRule = (EventProcessingPolicyRule)data;
		session = ConsoleSharedData.getInstance().getSession();
		
		List<Long> eventCodes = eppRule.getEvents();
		List<EventTemplate> events = session.findMultipleEventTemplates(eventCodes.toArray(new Long[eventCodes.size()]));
		if (eventCodes.size() > 0)
		{
			Collections.sort(events, new Comparator<EventTemplate>() {
				@Override
				public int compare(EventTemplate arg0, EventTemplate arg1)
				{
					return arg0.getName().compareToIgnoreCase(arg1.getName());
				}
			});
			
			for(EventTemplate e : events)
			{
				addEntry(e.getName(), ImageFactory.getImage(StatusDisplayInfo.getStatusImageDescriptor(e.getSeverity())));
			}
		}
		else
		{
			addEntry("Any", ImageFactory.getImage("icons/any.gif"));
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.epp.widgets.ListCell#fillContextMenu(org.eclipse.jface.action.IMenuManager)
	 */
	@Override
	protected void fillContextMenu(IMenuManager mgr)
	{
	}
}
