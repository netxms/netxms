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
import org.netxms.ui.eclipse.eventmanager.Messages;
import org.netxms.ui.eclipse.eventmanager.dialogs.EventSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Event selector widget. Provides uniform way to display selected
 * event and change selection.
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
		super(parent, style, 0);
		setText(Messages.EventSelector_None);
	}

	/**
	 * @param parent
	 * @param style
	 */
	public EventSelector(Composite parent, int style, boolean useHyperlink)
	{
		super(parent, style, (useHyperlink ? USE_HYPERLINK : 0) | SHOW_CLEAR_BUTTON);
		setText(Messages.EventSelector_None);
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
			long prevEventCode = eventCode;
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
				setText(Messages.EventSelector_None);
				setImage(null);
				getTextControl().setToolTipText(null);
			}
			if (prevEventCode != eventCode)
				fireModifyListeners();
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#clearButtonHandler()
	 */
	@Override
	protected void clearButtonHandler()
	{
		if (eventCode == 0)
			return;
		
		eventCode = 0;
		setText(Messages.EventSelector_None);
		setImage(null);
		getTextControl().setToolTipText(null);
		fireModifyListeners();
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
		if (this.eventCode == eventCode)
			return;	// nothing to change
		
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
				setText(Messages.EventSelector_Unknown);
				setImage(null);
				getTextControl().setToolTipText(null);
			}
		}
		else
		{
			setText(Messages.EventSelector_None);
			setImage(null);
			getTextControl().setToolTipText(null);
		}
		fireModifyListeners();
	}

	/**
	 * Generate tooltip text for event
	 * @param event event template
	 * @return tooltip text
	 */
	private String generateToolTipText(EventTemplate event)
	{
		StringBuilder sb = new StringBuilder(event.getName());
		sb.append(" ["); //$NON-NLS-1$
		sb.append(event.getCode());
		sb.append(Messages.EventSelector_Severity);
		sb.append(StatusDisplayInfo.getStatusText(event.getSeverity()));
		sb.append("\n\n"); //$NON-NLS-1$
		sb.append(event.getMessage());
		sb.append("\n\n"); //$NON-NLS-1$
		sb.append(event.getDescription().replace("\r", "")); //$NON-NLS-1$ //$NON-NLS-2$
		return sb.toString();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getSelectionButtonToolTip()
	 */
	@Override
	protected String getSelectionButtonToolTip()
	{
		return Messages.EventSelector_Tooltip;
	}
}
