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
package org.netxms.nxmc.modules.events.widgets.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.Event;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.EventTraceWidget;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for event monitor
 */
public class EventLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(EventLabelProvider.class);

   private NXCSession session = Registry.getSession();
	private boolean showColor = true;
	private boolean showIcons = false;

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
	@Override
	public Color getForeground(Object element)
	{
      return null;
	}

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
	@Override
	public Color getBackground(Object element)
	{
      if (!showColor)
         return null;

      Severity severity;
      if (element instanceof Event)
         severity = ((Event)element).getSeverity();
      else if (element instanceof HistoricalEvent)
         severity = ((HistoricalEvent)element).getSeverity();
      else
         return null;

      return StatusDisplayInfo.getStatusBackgroundColor(severity);
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (showIcons && (columnIndex == 0))
		{
         Severity severity;
         if (element instanceof Event)
            severity = ((Event)element).getSeverity();
         else if (element instanceof HistoricalEvent)
            severity = ((HistoricalEvent)element).getSeverity();
         else
            return null;
			return StatusDisplayInfo.getStatusImage(severity);
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
      long sourceId;
      java.util.Date timeStamp;
      Severity severity;
      int code;
      String message;

      if (element instanceof Event)
      {
         Event event = (Event)element;
         sourceId = event.getSourceId();
         timeStamp = event.getTimeStamp();
         severity = event.getSeverity();
         code = event.getCode();
         message = event.getMessage();
      }
      else if (element instanceof HistoricalEvent)
      {
         HistoricalEvent event = (HistoricalEvent)element;
         sourceId = event.getSourceId();
         timeStamp = event.getTimeStamp();
         severity = event.getSeverity();
         code = event.getCode();
         message = event.getMessage();
      }
      else
      {
         return null;
      }

		switch(columnIndex)
		{
			case EventTraceWidget.COLUMN_TIMESTAMP:
            return DateFormatFactory.getDateTimeFormat().format(timeStamp);
			case EventTraceWidget.COLUMN_SOURCE:
				final AbstractObject object = session.findObjectById(sourceId);
            return (object != null) ? object.getObjectName() : i18n.tr("Unknown");
			case EventTraceWidget.COLUMN_SEVERITY:
				return StatusDisplayInfo.getStatusText(severity);
			case EventTraceWidget.COLUMN_EVENT:
			   return session.getEventName(code);
			case EventTraceWidget.COLUMN_MESSAGE:
				return message;
		}
		return null;
	}

	/**
	 * @return the showColor
	 */
	public boolean isShowColor()
	{
		return showColor;
	}

	/**
	 * @param showColor the showColor to set
	 */
	public void setShowColor(boolean showColor)
	{
		this.showColor = showColor;
	}

	/**
	 * @return the showIcons
	 */
	public boolean isShowIcons()
	{
		return showIcons;
	}

	/**
	 * @param showIcons the showIcons to set
	 */
	public void setShowIcons(boolean showIcons)
	{
		this.showIcons = showIcons;
	}
}
