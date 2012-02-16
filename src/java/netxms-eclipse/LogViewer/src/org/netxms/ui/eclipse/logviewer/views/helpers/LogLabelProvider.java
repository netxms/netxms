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
package org.netxms.ui.eclipse.logviewer.views.helpers;

import java.text.DateFormat;
import java.util.Collection;
import java.util.Date;
import java.util.List;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.logviewer.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for log viewer
 *
 */
public class LogLabelProvider implements ITableLabelProvider
{
	public static final String[] ALARM_STATE_TEXTS = { "Outstanding", "Acknowledged", "Terminated" };
	public static final String[] ALARM_HD_STATE_TEXTS = { "Ignored", "Open", "Closed" };
	
	private LogColumn[] columns;
	private NXCSession session;
	private Image[] alarmStateImages;
	private WorkbenchLabelProvider wbLabelProvider;
	
	public LogLabelProvider(Log logHandle)
	{
		Collection<LogColumn> c = logHandle.getColumns();
		columns = c.toArray(new LogColumn[c.size()]);
		session = (NXCSession)ConsoleSharedData.getSession();
		
		alarmStateImages = new Image[3];
		alarmStateImages[Alarm.STATE_OUTSTANDING] = Activator.getImageDescriptor("icons/outstanding.png").createImage();
		alarmStateImages[Alarm.STATE_ACKNOWLEDGED] = Activator.getImageDescriptor("icons/acknowledged.png").createImage();
		alarmStateImages[Alarm.STATE_TERMINATED] = Activator.getImageDescriptor("icons/terminated.png").createImage();
		
		wbLabelProvider = new WorkbenchLabelProvider();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		final String value = ((List<String>)element).get(columnIndex);
		switch(columns[columnIndex].getType())
		{
			case LogColumn.LC_SEVERITY:
				try
				{
					int severity = Integer.parseInt(value);
					return StatusDisplayInfo.getStatusImage(severity);
				}
				catch(NumberFormatException e)
				{
					return null;
				}
			case LogColumn.LC_ALARM_STATE:
				try
				{
					int state = Integer.parseInt(value);
					return alarmStateImages[state];
				}
				catch(NumberFormatException e)
				{
					return null;
				}
			case LogColumn.LC_OBJECT_ID:
				try
				{
					long id = Long.parseLong(value);
					GenericObject object = session.findObjectById(id);
					return (object != null) ? wbLabelProvider.getImage(object) : null;
				}
				catch(NumberFormatException e)
				{
					return null;
				}
			case LogColumn.LC_USER_ID:
				try
				{
					long id = Long.parseLong(value);
					AbstractUserObject user = session.findUserDBObjectById(id);
					return (user != null) ? wbLabelProvider.getImage(user) : null;
				}
				catch(NumberFormatException e)
				{
					return null;
				}
			default:
				return null;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final String value = ((List<String>)element).get(columnIndex);
		switch(columns[columnIndex].getType())
		{
			case LogColumn.LC_TIMESTAMP:
				try
				{
					long timestamp = Long.parseLong(value);
					Date date = new Date(timestamp * 1000);
					return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.MEDIUM).format(date);
				}
				catch(NumberFormatException e)
				{
					return "<error>";
				}
			case LogColumn.LC_OBJECT_ID:
				try
				{
					long id = Long.parseLong(value);
					if (id == 0)
						return "";
					GenericObject object = session.findObjectById(id);
					return (object != null) ? object.getObjectName() : "<unknown>";
				}
				catch(NumberFormatException e)
				{
					return "<error>";
				}
			case LogColumn.LC_SEVERITY:
				try
				{
					int severity = Integer.parseInt(value);
					return StatusDisplayInfo.getStatusText(severity);
				}
				catch(NumberFormatException e)
				{
					return "<error>";
				}
			case LogColumn.LC_USER_ID:
				try
				{
					long id = Long.parseLong(value);
					AbstractUserObject user = session.findUserDBObjectById(id);
					return (user != null) ? wbLabelProvider.getText(user) : null;
				}
				catch(NumberFormatException e)
				{
					return null;
				}
			case LogColumn.LC_EVENT_CODE:
				try
				{
					long code = Long.parseLong(value);
					EventTemplate evt = session.findEventTemplateByCode(code);
					return (evt != null) ? evt.getName() : null;
				}
				catch(NumberFormatException e)
				{
					return null;
				}
			case LogColumn.LC_ALARM_STATE:
				try
				{
					int state = Integer.parseInt(value);
					return ALARM_STATE_TEXTS[state];
				}
				catch(Exception e)
				{
					return "<error>";
				}
			case LogColumn.LC_ALARM_HD_STATE:
				try
				{
					int state = Integer.parseInt(value);
					return ALARM_HD_STATE_TEXTS[state];
				}
				catch(Exception e)
				{
					return "<error>";
				}
			default:
				return value;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void addListener(ILabelProviderListener listener)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(int i = 0; i < alarmStateImages.length; i++)
			if (alarmStateImages[i] != null)
				alarmStateImages[i].dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
	 */
	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void removeListener(ILabelProviderListener listener)
	{
	}
}
