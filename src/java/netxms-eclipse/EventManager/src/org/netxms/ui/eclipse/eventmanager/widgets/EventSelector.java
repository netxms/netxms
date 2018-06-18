/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.netxms.client.events.EventObject;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
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
	private long eventCode = 0;
   private String eventName = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public EventSelector(Composite parent, int style)
	{
		super(parent, style, 0);
		setText(Messages.get().EventSelector_None);
	}

	/**
	 * @param parent
	 * @param style
	 */
	public EventSelector(Composite parent, int style, boolean useHyperlink)
	{
		super(parent, style, (useHyperlink ? USE_HYPERLINK : 0) | SHOW_CLEAR_BUTTON);
		setText(Messages.get().EventSelector_None);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		EventSelectionDialog dlg = new EventSelectionDialog(getShell(), false);
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			long prevEventCode = eventCode;
			EventObject[] events = dlg.getSelectedEvents();
			if (events.length > 0)
			{
				eventCode = events[0].getCode();
				eventName = events[0].getName();
				setText(events[0].getName());
				if (events[0] instanceof EventTemplate)
   				setImage(StatusDisplayInfo.getStatusImage(((EventTemplate)events[0]).getSeverity()));
				else
				   setImage(SharedIcons.IMG_CONTAINER);

            getTextControl().setToolTipText(generateToolTipText(events[0]));
			}
			else
			{
				eventCode = 0;
				eventName = null;
				setText(Messages.get().EventSelector_None);
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
		eventName = null;
		setText(Messages.get().EventSelector_None);
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
    * Get name of selected event
    * 
    * @return Selected event's name
    */
   public String getEventName()
   {
      return eventName;
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
			EventObject object = ((NXCSession)ConsoleSharedData.getSession()).findEventObjectByCode(eventCode);
			if (object != null)
			{
			   eventName = object.getName();
				setText(eventName);
				if (object instanceof EventTemplate)
				   setImage(StatusDisplayInfo.getStatusImage(((EventTemplate)object).getSeverity()));
				else
				   setImage(SharedIcons.IMG_CONTAINER);
				getTextControl().setToolTipText(generateToolTipText(object));
			}
			else
			{
				setText(Messages.get().EventSelector_Unknown);
				setImage(null);
				getTextControl().setToolTipText(null);
			}
		}
		else
		{
			setText(Messages.get().EventSelector_None);
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
	private String generateToolTipText(EventObject object)
	{
		StringBuilder sb = new StringBuilder(object.getName());
		sb.append(" ["); //$NON-NLS-1$
		sb.append(object.getCode());
		if (object instanceof EventTemplate)
		{
   		sb.append(Messages.get().EventSelector_Severity);
   		sb.append(StatusDisplayInfo.getStatusText(((EventTemplate)object).getSeverity()));
         sb.append("\n\n"); //$NON-NLS-1$
         sb.append(((EventTemplate)object).getMessage());
		}
		sb.append("\n\n"); //$NON-NLS-1$
		sb.append(object.getDescription().replace("\r", "")); //$NON-NLS-1$ //$NON-NLS-2$
		return sb.toString();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getSelectionButtonToolTip()
	 */
	@Override
	protected String getSelectionButtonToolTip()
	{
		return Messages.get().EventSelector_Tooltip;
	}
}
