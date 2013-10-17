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
package org.netxms.ui.eclipse.eventmanager.widgets.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Event;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.console.tools.RegionalSettings;
import org.netxms.ui.eclipse.eventmanager.Messages;
import org.netxms.ui.eclipse.eventmanager.widgets.EventTraceWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for event monitor
 */
public class EventLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
	private static final Color FOREGROUND_COLOR_DARK = new Color(Display.getCurrent(), 0, 0, 0);
	private static final Color FOREGROUND_COLOR_LIGHT = new Color(Display.getCurrent(), 255, 255, 255);
	private static final Color[] FOREGROUND_COLORS =
		{ FOREGROUND_COLOR_LIGHT, FOREGROUND_COLOR_DARK, FOREGROUND_COLOR_DARK, FOREGROUND_COLOR_LIGHT, FOREGROUND_COLOR_LIGHT };
	
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private boolean showColor = true;
	private boolean showIcons = false;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
	 */
	@Override
	public Color getForeground(Object element)
	{
		return showColor ? FOREGROUND_COLORS[((Event)element).getSeverity()] : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
	 */
	@Override
	public Color getBackground(Object element)
	{
		return showColor ? StatusDisplayInfo.getStatusColor(((Event)element).getSeverity()) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (showIcons && (columnIndex == 0))
		{
			return StatusDisplayInfo.getStatusImage(((Event)element).getSeverity());
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final Event event = (Event)element;
		switch(columnIndex)
		{
			case EventTraceWidget.COLUMN_TIMESTAMP:
				return RegionalSettings.getDateTimeFormat().format(event.getTimeStamp());
			case EventTraceWidget.COLUMN_SOURCE:
				final AbstractObject object = session.findObjectById(event.getSourceId());
				return (object != null) ? object.getObjectName() : Messages.EventLabelProvider_Unknown;
			case EventTraceWidget.COLUMN_SEVERITY:
				return StatusDisplayInfo.getStatusText(event.getSeverity());
			case EventTraceWidget.COLUMN_EVENT:
				EventTemplate tmpl = session.findEventTemplateByCode(event.getCode());
				return (tmpl != null) ? tmpl.getName() : Integer.toString(event.getCode());
			case EventTraceWidget.COLUMN_MESSAGE:
				return event.getMessage();
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
