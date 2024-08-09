/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.netxms.client.events.SyslogRecord;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.SyslogTraceWidget;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for syslog monitor
 */
public class SyslogLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private static final Severity[] severityMap = { Severity.CRITICAL, Severity.CRITICAL, Severity.MAJOR, Severity.MINOR, Severity.WARNING, Severity.WARNING, Severity.NORMAL, Severity.NORMAL };

   private final I18n i18n = LocalizationHelper.getI18n(SyslogLabelProvider.class);
   private final String[] severityText =
      { i18n.tr("Emergency"), i18n.tr("Alert"), i18n.tr("Critical"), i18n.tr("Error"), i18n.tr("Warning"), i18n.tr("Notice"), i18n.tr("Info"), i18n.tr("Debug") };
   private final String[] facilityText =
		{ i18n.tr("Kernel"), i18n.tr("User"), i18n.tr("Mail"), i18n.tr("System"), i18n.tr("Auth"), i18n.tr("Syslog"), i18n.tr("Lpr"), i18n.tr("News"), i18n.tr("UUCP"),
		  i18n.tr("Cron"), i18n.tr("Security"), i18n.tr("FTPD"), i18n.tr("NTP"), i18n.tr("Log Audit"), i18n.tr("Log Alert"), i18n.tr("Clock"), i18n.tr("Local0"),
		  i18n.tr("Local1"), i18n.tr("Local2"), i18n.tr("Local3"), i18n.tr("Local4"), i18n.tr("Local5"), i18n.tr("Local6"), i18n.tr("Local7")
		};

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
      return showColor ? StatusDisplayInfo.getStatusBackgroundColor(severityMap[((SyslogRecord)element).getSeverity()]) : null;
	}

   /**
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

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final SyslogRecord record = (SyslogRecord)element;
		switch(columnIndex)
		{
			case SyslogTraceWidget.COLUMN_TIMESTAMP:
            return DateFormatFactory.getDateTimeFormat().format(record.getTimestamp());
			case SyslogTraceWidget.COLUMN_SOURCE:
				final AbstractObject object = session.findObjectById(record.getSourceObjectId());
            return (object != null) ? object.getObjectName() : i18n.tr("<unknown>");
			case SyslogTraceWidget.COLUMN_SEVERITY:
				try
				{
					return severityText[record.getSeverity()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
               return "<" + Integer.toString(record.getSeverity()) + ">";
				}
			case SyslogTraceWidget.COLUMN_FACILITY:
				try
				{
					return facilityText[record.getFacility()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
               return "<" + Integer.toString(record.getFacility()) + ">";
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
