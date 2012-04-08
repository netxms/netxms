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
package org.netxms.ui.eclipse.eventmanager.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.eventmanager.dialogs.EventSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Event selector widget. Provides uniform way to display selected
 * event and change selection.
 *
 */
public class EventSelector extends AbstractSelector
{
	private static final long serialVersionUID = 1L;

	private long eventCode = 0;
	
	/**
	 * @param parent
	 * @param style
	 */
	public EventSelector(Composite parent, int style)
	{
		super(parent, style, false);
		setText("<none>");
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		EventSelectionDialog dlg = new EventSelectionDialog(getShell());
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			EventTemplate[] events = dlg.getSelectedEvents();
			if (events.length > 0)
			{
				eventCode = events[0].getCode();
				setText(events[0].getName());
				setImage(StatusDisplayInfo.getStatusImage(events[0].getSeverity()));
				getTextControl().setToolTipText(generateToolTipText(events[0]));
			}
			else
			{
				eventCode = 0;
				setText("<none>");
				setImage(null);
				getTextControl().setToolTipText(null);
			}
		}
	}

	/**
	 * Get code of selected event
	 * 
	 * @return Selected event's code
	 */
	public long getEventCode()
	{
		return eventCode;
	}

	/**
	 * Set event code
	 * @param eventCode
	 */
	public void setEventCode(long eventCode)
	{
		this.eventCode = eventCode;
		if (eventCode != 0)
		{
			EventTemplate event = ((NXCSession)ConsoleSharedData.getSession()).findEventTemplateByCode(eventCode);
			if (event != null)
			{
				setText(event.getName());
				setImage(StatusDisplayInfo.getStatusImage(event.getSeverity()));
				getTextControl().setToolTipText(generateToolTipText(event));
			}
			else
			{
				setText("<unknown>");
				setImage(null);
				getTextControl().setToolTipText(null);
			}
		}
		else
		{
			setText("<none>");
			setImage(null);
			getTextControl().setToolTipText(null);
		}
	}

	/**
	 * Generate tooltip text for event
	 * @param event event template
	 * @return tooltip text
	 */
	private String generateToolTipText(EventTemplate event)
	{
		StringBuilder sb = new StringBuilder(event.getName());
		sb.append(" [");
		sb.append(event.getCode());
		sb.append("]\nSeverity: ");
		sb.append(StatusDisplayInfo.getStatusText(event.getSeverity()));
		sb.append("\n\n");
		sb.append(event.getMessage());
		sb.append("\n\n");
		sb.append(event.getDescription().replace("\r", ""));
		return sb.toString();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getButtonToolTip()
	 */
	@Override
	protected String getButtonToolTip()
	{
		return "Select event";
	}
}
