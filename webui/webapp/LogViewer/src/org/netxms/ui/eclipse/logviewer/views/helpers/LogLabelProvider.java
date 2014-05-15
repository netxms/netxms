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

import java.util.Collection;
import java.util.Date;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.client.NXCSession;
import org.netxms.client.TableRow;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.logviewer.Activator;
import org.netxms.ui.eclipse.logviewer.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for log viewer
 */
public class LogLabelProvider implements ITableLabelProvider
{
	public final String[] ALARM_STATE_TEXTS = { Messages.get().LogLabelProvider_Outstanding, Messages.get().LogLabelProvider_Acknowledged, Messages.get().LogLabelProvider_Resolved, Messages.get().LogLabelProvider_Terminated };
	public final String[] ALARM_HD_STATE_TEXTS = { Messages.get().LogLabelProvider_Ignored, Messages.get().LogLabelProvider_Open, Messages.get().LogLabelProvider_Closed };
	
	private LogColumn[] columns;
	private NXCSession session;
	private Image[] alarmStateImages;
	private WorkbenchLabelProvider wbLabelProvider;
	
	/**
	 * Get empty instance (not suitable to be real label provider - needed only to provide access to localized texts)
	 * 
	 * @return instance of log label provider
	 */
	public static LogLabelProvider getEmptyInstance()
	{
	   return new LogLabelProvider();
	}
	
	/**
	 * Private empty constructor 
	 */
	private LogLabelProvider()
	{
	}
	
	/**
	 * @param logHandle
	 */
	public LogLabelProvider(Log logHandle)
	{
		Collection<LogColumn> c = logHandle.getColumns();
		columns = c.toArray(new LogColumn[c.size()]);
		session = (NXCSession)ConsoleSharedData.getSession();
		
		alarmStateImages = new Image[4];
		alarmStateImages[Alarm.STATE_OUTSTANDING] = Activator.getImageDescriptor("icons/outstanding.png").createImage(); //$NON-NLS-1$
		alarmStateImages[Alarm.STATE_ACKNOWLEDGED] = Activator.getImageDescriptor("icons/acknowledged.png").createImage(); //$NON-NLS-1$
		alarmStateImages[Alarm.STATE_RESOLVED] = Activator.getImageDescriptor("icons/resolved.png").createImage(); //$NON-NLS-1$
		alarmStateImages[Alarm.STATE_TERMINATED] = Activator.getImageDescriptor("icons/terminated.png").createImage(); //$NON-NLS-1$
		
		wbLabelProvider = new WorkbenchLabelProvider();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		final String value = ((TableRow)element).get(columnIndex).getValue();
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
					return alarmStateImages[state & Alarm.STATE_MASK];
				}
				catch(NumberFormatException e)
				{
					return null;
				}
			case LogColumn.LC_OBJECT_ID:
				try
				{
					long id = Long.parseLong(value);
					AbstractObject object = session.findObjectById(id);
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
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final String value = ((TableRow)element).get(columnIndex).getValue();
		switch(columns[columnIndex].getType())
		{
			case LogColumn.LC_TIMESTAMP:
				try
				{
					long timestamp = Long.parseLong(value);
					Date date = new Date(timestamp * 1000);
					return RegionalSettings.getDateTimeFormat().format(date);
				}
				catch(NumberFormatException e)
				{
					return Messages.get().LogLabelProvider_Error;
				}
			case LogColumn.LC_OBJECT_ID:
				try
				{
					long id = Long.parseLong(value);
					if (id == 0)
						return ""; //$NON-NLS-1$
					AbstractObject object = session.findObjectById(id);
					return (object != null) ? object.getObjectName() : ("[" + id + "]"); //$NON-NLS-1$ //$NON-NLS-2$
				}
				catch(NumberFormatException e)
				{
					return Messages.get().LogLabelProvider_Error;
				}
			case LogColumn.LC_SEVERITY:
				try
				{
					int severity = Integer.parseInt(value);
					return StatusDisplayInfo.getStatusText(severity);
				}
				catch(NumberFormatException e)
				{
					return Messages.get().LogLabelProvider_Error;
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
					return (evt != null) ? evt.getName() : ("[" + code + "]"); //$NON-NLS-1$ //$NON-NLS-2$
				}
				catch(NumberFormatException e)
				{
					return null;
				}
			case LogColumn.LC_ALARM_STATE:
				try
				{
					int state = Integer.parseInt(value);
					return ALARM_STATE_TEXTS[state & Alarm.STATE_MASK];
				}
				catch(Exception e)
				{
					return Messages.get().LogLabelProvider_Error;
				}
			case LogColumn.LC_ALARM_HD_STATE:
				try
				{
					int state = Integer.parseInt(value);
					return ALARM_HD_STATE_TEXTS[state];
				}
				catch(Exception e)
				{
					return Messages.get().LogLabelProvider_Error;
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
