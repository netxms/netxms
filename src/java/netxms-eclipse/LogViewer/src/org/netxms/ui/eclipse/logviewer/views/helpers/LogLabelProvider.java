/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.TableRow;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.events.Alarm;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Zone;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.console.ViewerElementUpdater;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.logviewer.Activator;
import org.netxms.ui.eclipse.logviewer.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for log viewer
 */
public class LogLabelProvider extends LabelProvider implements ITableLabelProvider
{
	public static final String[] ALARM_STATE_TEXTS = { Messages.get().LogLabelProvider_Outstanding, Messages.get().LogLabelProvider_Acknowledged, Messages.get().LogLabelProvider_Resolved, Messages.get().LogLabelProvider_Terminated };
	public static final String[] ALARM_HD_STATE_TEXTS = { Messages.get().LogLabelProvider_Ignored, Messages.get().LogLabelProvider_Open, Messages.get().LogLabelProvider_Closed };
   public static final String[] EVENT_ORIGIN_TEXTS = { "System", "Agent", "Client", "Syslog", "SNMP", "Script", "Remote Server", "Windows Event" };

	private LogColumn[] columns;
	private NXCSession session;
	private Image[] alarmStateImages;
	private WorkbenchLabelProvider wbLabelProvider;
   private TableViewer viewer;

	/**
	 * @param logHandle
	 */
	public LogLabelProvider(Log logHandle, TableViewer viewer)
	{
	   this.viewer = viewer;
		Collection<LogColumn> c = logHandle.getColumns();
		columns = c.toArray(new LogColumn[c.size()]);
		session = ConsoleSharedData.getSession();

		alarmStateImages = new Image[4];
		alarmStateImages[Alarm.STATE_OUTSTANDING] = Activator.getImageDescriptor("icons/outstanding.png").createImage(); //$NON-NLS-1$
		alarmStateImages[Alarm.STATE_ACKNOWLEDGED] = Activator.getImageDescriptor("icons/acknowledged.png").createImage(); //$NON-NLS-1$
		alarmStateImages[Alarm.STATE_RESOLVED] = Activator.getImageDescriptor("icons/resolved.png").createImage(); //$NON-NLS-1$
		alarmStateImages[Alarm.STATE_TERMINATED] = Activator.getImageDescriptor("icons/terminated.png").createImage(); //$NON-NLS-1$

		wbLabelProvider = new WorkbenchLabelProvider();
	}

	/**
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
      final String value = ((TableRow)element).get(columnIndex).getValue();
		switch(columns[columnIndex].getType())
		{
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
			case LogColumn.LC_COMPLETION_STATUS:
				try
				{
				   int status = Integer.parseInt(value);
				   return StatusDisplayInfo.getStatusImage(status > 0 ? ObjectStatus.NORMAL.getValue() : ObjectStatus.MAJOR.getValue());
				}
				catch(NumberFormatException e)
				{
				   return null;
				}
			case LogColumn.LC_USER_ID:
				try
				{
               int id = Integer.parseInt(value);
					AbstractUserObject user = session.findUserDBObjectById(id, new ViewerElementUpdater(viewer, element));
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

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final String value = ((TableRow)element).get(columnIndex).getValue();
		switch(columns[columnIndex].getType())
		{
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
         case LogColumn.LC_EVENT_CODE:
            try
            {
               return session.getEventName(Integer.parseInt(value));
            }
            catch(NumberFormatException e)
            {
               return null;
            }
         case LogColumn.LC_EVENT_ORIGIN:
            try
            {
               int origin = Integer.parseInt(value);
               return EVENT_ORIGIN_TEXTS[origin];
            }
            catch(Exception e)
            {
               return value;
            }
			case LogColumn.LC_OBJECT_ID:
				try
				{
					long id = Long.parseLong(value);
					if (id == 0)
						return ""; //$NON-NLS-1$
               return session.getObjectName(id);
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
         case LogColumn.LC_COMPLETION_STATUS:
            try
            {
               int status = Integer.parseInt(value);
               return status > 0 ? "Success" : "Failure";
            }
            catch(NumberFormatException e)
            {
               return Messages.get().LogLabelProvider_Error;
            }
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
			case LogColumn.LC_USER_ID:
				try
				{
               int id = Integer.parseInt(value);
					AbstractUserObject user = session.findUserDBObjectById(id, new ViewerElementUpdater(viewer, element));
					return (user != null) ? wbLabelProvider.getText(user) : null;
				}
				catch(NumberFormatException e)
				{
					return null;
				}
         case LogColumn.LC_ZONE_UIN:
            try
            {
               int uin = Integer.parseInt(value);
               Zone zone = session.findZone(uin);
               return (zone != null) ? zone.getObjectName() : "[" + uin + "]";
            }
            catch(NumberFormatException e)
            {
               return null;
            }
			default:
				return value;
		}
	}

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
	@Override
	public void dispose()
	{
		for(int i = 0; i < alarmStateImages.length; i++)
			if (alarmStateImages[i] != null)
				alarmStateImages[i].dispose();
      wbLabelProvider.dispose();
	}
}
