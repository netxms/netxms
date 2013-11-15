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
import org.netxms.client.constants.Severity;
import org.netxms.client.events.SyslogRecord;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.console.tools.RegionalSettings;
import org.netxms.ui.eclipse.eventmanager.Messages;
import org.netxms.ui.eclipse.eventmanager.widgets.SyslogTraceWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for syslog monitor
 */
public class SyslogLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
	private static final Color FOREGROUND_COLOR_DARK = new Color(Display.getCurrent(), 0, 0, 0);
	private static final Color FOREGROUND_COLOR_LIGHT = new Color(Display.getCurrent(), 255, 255, 255);
	private static final Color[] FOREGROUND_COLORS =
		{ FOREGROUND_COLOR_LIGHT, FOREGROUND_COLOR_DARK, FOREGROUND_COLOR_DARK, FOREGROUND_COLOR_LIGHT, FOREGROUND_COLOR_LIGHT };
	private static final int[] severityMap = 
		{ Severity.CRITICAL, Severity.CRITICAL, Severity.MAJOR, Severity.MINOR, Severity.WARNING, Severity.WARNING, Severity.NORMAL, Severity.NORMAL };  
	private static final String[] severityText = 
		{ Messages.get().SyslogLabelProvider_SevEmergency, Messages.get().SyslogLabelProvider_SevAlert, Messages.get().SyslogLabelProvider_SevCritical, Messages.get().SyslogLabelProvider_SevError, Messages.get().SyslogLabelProvider_SevWarning, Messages.get().SyslogLabelProvider_SevNotice, Messages.get().SyslogLabelProvider_SevInfo, Messages.get().SyslogLabelProvider_SevDebug };
	private static final String[] facilityText =
		{ Messages.get().SyslogLabelProvider_FacKernel, Messages.get().SyslogLabelProvider_FacUser, Messages.get().SyslogLabelProvider_FacMail, Messages.get().SyslogLabelProvider_FacSystem, Messages.get().SyslogLabelProvider_FacAuth, Messages.get().SyslogLabelProvider_FacSyslog, Messages.get().SyslogLabelProvider_FacLpr, Messages.get().SyslogLabelProvider_FacNews, Messages.get().SyslogLabelProvider_FacUUCP, Messages.get().SyslogLabelProvider_FacCron, Messages.get().SyslogLabelProvider_FacSecurity,
		  Messages.get().SyslogLabelProvider_FacFTPD, Messages.get().SyslogLabelProvider_FacNTP, Messages.get().SyslogLabelProvider_FacLogAudit, Messages.get().SyslogLabelProvider_FacLogAlert, Messages.get().SyslogLabelProvider_FacClock, Messages.get().SyslogLabelProvider_FacLocal0, Messages.get().SyslogLabelProvider_FacLocal1, Messages.get().SyslogLabelProvider_FacLocal2, Messages.get().SyslogLabelProvider_FacLocal3, Messages.get().SyslogLabelProvider_FacLocal4,
		  Messages.get().SyslogLabelProvider_FacLocal5, Messages.get().SyslogLabelProvider_FacLocal6, Messages.get().SyslogLabelProvider_FacLocal7
		};
	
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private boolean showColor = true;
	private boolean showIcons = false;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
	 */
	@Override
	public Color getForeground(Object element)
	{
		return showColor ? FOREGROUND_COLORS[severityMap[((SyslogRecord)element).getSeverity()]] : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
	 */
	@Override
	public Color getBackground(Object element)
	{
		return showColor ? StatusDisplayInfo.getStatusColor(severityMap[((SyslogRecord)element).getSeverity()]) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (showIcons && (columnIndex == 0))
		{
			return StatusDisplayInfo.getStatusImage(severityMap[((SyslogRecord)element).getSeverity()]);
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final SyslogRecord record = (SyslogRecord)element;
		switch(columnIndex)
		{
			case SyslogTraceWidget.COLUMN_TIMESTAMP:
				return RegionalSettings.getDateTimeFormat().format(record.getTimestamp());
			case SyslogTraceWidget.COLUMN_SOURCE:
				final AbstractObject object = session.findObjectById(record.getSourceObjectId());
				return (object != null) ? object.getObjectName() : Messages.get().SyslogLabelProvider_Unknown;
			case SyslogTraceWidget.COLUMN_SEVERITY:
				try
				{
					return severityText[record.getSeverity()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
					return "<" + Integer.toString(record.getSeverity()) + ">"; //$NON-NLS-1$ //$NON-NLS-2$
				}
			case SyslogTraceWidget.COLUMN_FACILITY:
				try
				{
					return facilityText[record.getFacility()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
					return "<" + Integer.toString(record.getFacility()) + ">"; //$NON-NLS-1$ //$NON-NLS-2$
				}
			case SyslogTraceWidget.COLUMN_MESSAGE:
				return record.getMessage();
			case SyslogTraceWidget.COLUMN_TAG:
				return record.getTag();
			case SyslogTraceWidget.COLUMN_HOSTNAME:
				return record.getHostname();
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
