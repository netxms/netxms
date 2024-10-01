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
package org.netxms.nxmc.modules.logviewer.views.helpers;

import java.util.Collection;
import java.util.Date;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.TableRow;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.events.Alarm;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Zone;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.modules.users.views.helpers.BaseUserLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for log viewer
 */
public class LogLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(LogLabelProvider.class);
   private final String[] ALARM_STATE_TEXTS = getAlarmStateTexts(i18n);
   private final String[] ALARM_HD_STATE_TEXTS = getAlarmHDStateTexts(i18n);
   private final String[] EVENT_ORIGIN_TEXTS = getEventOriginTexts(i18n);
   private final String[] ASSET_OPERATION_TEXTS = getAssetOperationTexts(i18n);

	private LogColumn[] columns;
	private NXCSession session;
	private Image[] alarmStateImages;
   private BaseObjectLabelProvider objectLabelProvider;
   private BaseUserLabelProvider userLabelProvider;
   private TableViewer viewer;

	/**
	 * @param logHandle
	 */
	public LogLabelProvider(Log logHandle, TableViewer viewer)
	{
	   this.viewer = viewer;
		Collection<LogColumn> c = logHandle.getColumns();
		columns = c.toArray(new LogColumn[c.size()]);
      session = Registry.getSession();

		alarmStateImages = new Image[4];
      alarmStateImages[Alarm.STATE_OUTSTANDING] = ResourceManager.getImage("icons/alarms/outstanding.png");
      alarmStateImages[Alarm.STATE_ACKNOWLEDGED] = ResourceManager.getImage("icons/alarms/acknowledged.png");
      alarmStateImages[Alarm.STATE_RESOLVED] = ResourceManager.getImage("icons/alarms/resolved.png");
      alarmStateImages[Alarm.STATE_TERMINATED] = ResourceManager.getImage("icons/alarms/terminated.png");

      objectLabelProvider = new BaseObjectLabelProvider();
      userLabelProvider = new BaseUserLabelProvider();
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
               return (object != null) ? objectLabelProvider.getImage(object) : null;
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
               return (user != null) ? userLabelProvider.getImage(user) : null;
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
               return i18n.tr("<error>");
            }
         case LogColumn.LC_ALARM_STATE:
            try
            {
               int state = Integer.parseInt(value);
               return ALARM_STATE_TEXTS[state & Alarm.STATE_MASK];
            }
            catch(Exception e)
            {
               return i18n.tr("<error>");
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
               return (id != 0) ? session.getObjectName(id) : "";
				}
				catch(NumberFormatException e)
				{
               return i18n.tr("<error>");
				}
			case LogColumn.LC_SEVERITY:
				try
				{
					int severity = Integer.parseInt(value);
					return StatusDisplayInfo.getStatusText(severity);
				}
				catch(NumberFormatException e)
				{
               return i18n.tr("<error>");
				}
         case LogColumn.LC_COMPLETION_STATUS:
            try
            {
               int status = Integer.parseInt(value);
               return status > 0 ? "Success" : "Failure";
            }
            catch(NumberFormatException e)
            {
               return i18n.tr("<error>");
            }
         case LogColumn.LC_TIMESTAMP:
            try
            {
               long timestamp = Long.parseLong(value);
               Date date = new Date(timestamp * 1000);
               return DateFormatFactory.getDateTimeFormat().format(date);
            }
            catch(NumberFormatException e)
            {
               return i18n.tr("<error>");
            }
			case LogColumn.LC_USER_ID:
				try
				{
               int id = Integer.parseInt(value);
					AbstractUserObject user = session.findUserDBObjectById(id, new ViewerElementUpdater(viewer, element));
               return (user != null) ? userLabelProvider.getText(user) : null;
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
         case LogColumn.LC_ASSET_OPERATION:
            try
            {
               int operation = Integer.parseInt(value);
               return ASSET_OPERATION_TEXTS[operation];               
            }
            catch(Exception e)
            {
               return value;
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
      objectLabelProvider.dispose();
      userLabelProvider.dispose();
	}

   /**
    * Get alarm state texts
    *
    * @param i18n i18n object or null
    * @return alarm state texts
    */
   public static final String[] getAlarmStateTexts(I18n i18n)
   {
      if (i18n == null)
         i18n = LocalizationHelper.getI18n(LogLabelProvider.class);
      return new String[] { i18n.tr("Outstanding"), i18n.tr("Acknowledged"), i18n.tr("Resolved"), i18n.tr("Terminated") };
   }

   /**
    * Get alarm helpdesk state texts
    *
    * @param i18n i18n object or null
    * @return alarm helpdesk state texts
    */
   public static final String[] getAlarmHDStateTexts(I18n i18n)
   {
      if (i18n == null)
         i18n = LocalizationHelper.getI18n(LogLabelProvider.class);
      return new String[] { i18n.tr("Ignored"), i18n.tr("Open"), i18n.tr("Closed") };
   }

   /**
    * Get event origin texts
    *
    * @param i18n i18n object or null
    * @return event origin texts
    */
   public static final String[] getEventOriginTexts(I18n i18n)
   {
      if (i18n == null)
         i18n = LocalizationHelper.getI18n(LogLabelProvider.class);
      return new String[] { i18n.tr("System"), i18n.tr("Agent"), i18n.tr("Client"), i18n.tr("Syslog"), i18n.tr("SNMP"), i18n.tr("Script"), i18n.tr("Remote Server"), i18n.tr("Windows Event") };
   }

   /**
    * Get event origin texts
    *
    * @param i18n i18n object or null
    * @return event origin texts
    */
   public static final String[] getAssetOperationTexts(I18n i18n)
   {
      if (i18n == null)
         i18n = LocalizationHelper.getI18n(LogLabelProvider.class);
      return new String[] { i18n.tr("Create"), i18n.tr("Delete"), i18n.tr("Update"), i18n.tr("Link"), i18n.tr("Unlink") };
   }
}
