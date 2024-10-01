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
package org.netxms.nxmc.modules.events.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.dialogs.EventSelectionDialog;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Event selector widget. Provides uniform way to display selected
 * event and change selection.
 */
public class EventSelector extends AbstractSelector
{
   private final I18n i18n = LocalizationHelper.getI18n(EventSelector.class);

   private int eventCode = 0;
   private String eventName = null;

	/**
	 * @param parent
	 * @param style
	 */
	public EventSelector(Composite parent, int style)
	{
      this(parent, style, new SelectorConfigurator());
	}

	/**
	 * @param parent
	 * @param style
	 */
   public EventSelector(Composite parent, int style, SelectorConfigurator configurator)
	{
      super(parent, style, configurator.setSelectionButtonToolTip(LocalizationHelper.getI18n(EventSelector.class).tr("Select event")));
      setText(i18n.tr("None"));
	}

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
    */
	@Override
	protected void selectionButtonHandler()
	{
      EventSelectionDialog dlg = new EventSelectionDialog(getShell());
      dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
         int prevEventCode = eventCode;
			EventTemplate[] events = dlg.getSelectedEvents();
			if (events.length > 0)
			{
				eventCode = events[0].getCode();
				eventName = events[0].getName();
				setText(events[0].getName());
            setImage(StatusDisplayInfo.getStatusImage(events[0].getSeverity()));
            getTextControl().setToolTipText(generateToolTipText(events[0]));
			}
			else
			{
				eventCode = 0;
				eventName = null;
            setText(i18n.tr("None"));
				setImage(null);
				getTextControl().setToolTipText(null);
			}
			if (prevEventCode != eventCode)
				fireModifyListeners();
		}
	}

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#clearButtonHandler()
    */
	@Override
	protected void clearButtonHandler()
	{
		if (eventCode == 0)
			return;
		
		eventCode = 0;
		eventName = null;
      setText(i18n.tr("None"));
		setImage(null);
		getTextControl().setToolTipText(null);
		fireModifyListeners();
	}

	/**
	 * Get code of selected event
	 * 
	 * @return Selected event's code
	 */
   public int getEventCode()
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
   public void setEventCode(int eventCode)
	{
		if (this.eventCode == eventCode)
			return;	// nothing to change

		this.eventCode = eventCode;
		if (eventCode != 0)
		{
         EventTemplate evt = Registry.getSession().findEventTemplateByCode(eventCode);
			if (evt != null)
			{
			   eventName = evt.getName();
				setText(eventName);
			   setImage(StatusDisplayInfo.getStatusImage(((EventTemplate)evt).getSeverity()));
				getTextControl().setToolTipText(generateToolTipText(evt));
			}
			else
			{
            setText("[" + eventCode + "]");
            setImage(StatusDisplayInfo.getStatusImage(ObjectStatus.UNKNOWN));
				getTextControl().setToolTipText(null);
			}
		}
		else
		{
         eventName = null;
         setText(i18n.tr("None"));
			setImage(null);
			getTextControl().setToolTipText(null);
		}
		fireModifyListeners();
	}

	/**
    * Generate tooltip text for event
    *
    * @param event event template
    * @return tooltip text
    */
	private String generateToolTipText(EventTemplate evt)
	{
		StringBuilder sb = new StringBuilder(evt.getName());
      sb.append(" [");
		sb.append(evt.getCode());
      sb.append("] ");
		sb.append(StatusDisplayInfo.getStatusText(((EventTemplate)evt).getSeverity()));
      sb.append("\n\n");
      sb.append(evt.getMessage());
      sb.append("\n\n");
      sb.append(evt.getDescription().replace("\r", ""));
		return sb.toString();
	}
}
