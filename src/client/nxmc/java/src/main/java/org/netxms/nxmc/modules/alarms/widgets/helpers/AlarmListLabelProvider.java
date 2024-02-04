/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.alarms.widgets.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.AlarmHandle;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.interfaces.ZoneMember;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.widgets.AlarmList;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for alarm list
 */
public class AlarmListLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private static final Color FOREGROUND_COLOR_DARK = new Color(Display.getCurrent(), 0, 0, 0);
   private static final Color FOREGROUND_COLOR_LIGHT = new Color(Display.getCurrent(), 255, 255, 255);
   private static final Color[] FOREGROUND_COLORS = 
         { 
            FOREGROUND_COLOR_LIGHT, 
            FOREGROUND_COLOR_DARK, 
            FOREGROUND_COLOR_DARK,
            FOREGROUND_COLOR_LIGHT, 
            FOREGROUND_COLOR_LIGHT 
         };

   private final I18n i18n = LocalizationHelper.getI18n(AlarmListLabelProvider.class);
   private static final String[] stateText = 
         { 
            LocalizationHelper.getI18n(AlarmListLabelProvider.class).tr("Outstanding"),
            LocalizationHelper.getI18n(AlarmListLabelProvider.class).tr("Acknowledged"),
            LocalizationHelper.getI18n(AlarmListLabelProvider.class).tr("Resolved"),
            LocalizationHelper.getI18n(AlarmListLabelProvider.class).tr("Terminated") 
         };

   private NXCSession session;
   private Image[] stateImages = new Image[5];
   private boolean blinkState = true;
   private boolean showColor = true;
   private TreeViewer viewer;
   private BaseObjectLabelProvider objectLabelProvider;

   /**
    * Default constructor
    */
   public AlarmListLabelProvider(TreeViewer viewer)
   {
      this.viewer = viewer;
      session = Registry.getSession();
      objectLabelProvider = new BaseObjectLabelProvider();

      stateImages[0] = ResourceManager.getImage("icons/alarms/outstanding.png");
      stateImages[1] = ResourceManager.getImage("icons/alarms/acknowledged.png");
      stateImages[2] = ResourceManager.getImage("icons/alarms/resolved.png");
      stateImages[3] = ResourceManager.getImage("icons/alarms/terminated.png");
      stateImages[4] = ResourceManager.getImage("icons/alarms/acknowledged_sticky.png");
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      Alarm alarm = ((AlarmHandle)element).alarm;
      switch((Integer)viewer.getTree().getColumn(columnIndex).getData("ID"))
      {
         case AlarmList.COLUMN_SEVERITY:
            return StatusDisplayInfo.getStatusImage(alarm.getCurrentSeverity());
         case AlarmList.COLUMN_STATE:
            if (alarm.getState() == Alarm.STATE_OUTSTANDING)
               return blinkState ? stateImages[Alarm.STATE_OUTSTANDING] : SharedIcons.IMG_EMPTY;
            if ((alarm.getState() == Alarm.STATE_ACKNOWLEDGED) && alarm.isSticky())
               return stateImages[4];
            return stateImages[alarm.getState()];
         case AlarmList.COLUMN_SOURCE:
            AbstractObject object = session.findObjectById(alarm.getSourceObjectId());
            return (object != null) ? objectLabelProvider.getImage(object) : null;
         case AlarmList.COLUMN_COMMENTS:
            return (alarm.getCommentsCount() > 0) ? SharedIcons.IMG_COMMENTS : null;
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      Alarm alarm = ((AlarmHandle)element).alarm;
      switch((Integer)viewer.getTree().getColumn(columnIndex).getData("ID"))
      {
         case AlarmList.COLUMN_SEVERITY:
            return StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity());
         case AlarmList.COLUMN_STATE:
            int time = alarm.getAckTime();
            String timeString = time > 0
                  ? " (" + DateFormatFactory.getDateTimeFormat().format(System.currentTimeMillis() + time * 1000) + ")" //$NON-NLS-1$ //$NON-NLS-2$
                  : ""; //$NON-NLS-1$
            return stateText[alarm.getState()] + timeString;
         case AlarmList.COLUMN_SOURCE:
            AbstractObject object = session.findObjectById(alarm.getSourceObjectId());
            return (object != null) ? object.getObjectName() : ("[" + Long.toString(alarm.getSourceObjectId()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
         case AlarmList.COLUMN_ZONE:
            if (session.isZoningEnabled())
            {
               ZoneMember zm = session.findObjectById(alarm.getSourceObjectId(), ZoneMember.class);
               return (zm != null) ? zm.getZoneName() : "";
            }
            return "";
         case AlarmList.COLUMN_MESSAGE:
            return alarm.getMessage();
         case AlarmList.COLUMN_COUNT:
            return Integer.toString(alarm.getRepeatCount());
         case AlarmList.COLUMN_COMMENTS:
            return (alarm.getCommentsCount() > 0) ? Integer.toString(alarm.getCommentsCount()) : null;
         case AlarmList.COLUMN_ACK_BY:
            if (alarm.getState() == Alarm.STATE_OUTSTANDING)
               return null;
            int userId = (alarm.getState() == Alarm.STATE_ACKNOWLEDGED) ? alarm.getAcknowledgedByUser()
                  : alarm.getResolvedByUser();
            AbstractUserObject user = session.findUserDBObjectById(userId, new ViewerElementUpdater(viewer, element));
            return (user != null) ? user.getName() : ("[" + Long.toString(userId) + "]");
         case AlarmList.COLUMN_CREATED:
            return DateFormatFactory.getDateTimeFormat().format(alarm.getCreationTime());
         case AlarmList.COLUMN_LASTCHANGE:
            return DateFormatFactory.getDateTimeFormat().format(alarm.getLastChangeTime());
         case AlarmList.COLUMN_HELPDESK_REF:
            switch(alarm.getHelpdeskState())
            {
               case Alarm.HELPDESK_STATE_OPEN:
                  return alarm.getHelpdeskReference();
               case Alarm.HELPDESK_STATE_CLOSED:
                  return alarm.getHelpdeskReference() + i18n.tr(" (closed)");
            }
            return null;
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      for(int i = 0; i < stateImages.length; i++)
         stateImages[i].dispose();
      objectLabelProvider.dispose();
      super.dispose();
   }

   /**
    * Toggle blink state
    */
   public void toggleBlinkState()
   {
      blinkState = !blinkState;
   }

   /**
    * Toggle blink state
    */
   public void stopBlinking()
   {
      blinkState = true;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      return showColor ? FOREGROUND_COLORS[((AlarmHandle)element).alarm.getCurrentSeverity().getValue()] : null;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return showColor ? StatusDisplayInfo.getStatusColor(((AlarmHandle)element).alarm.getCurrentSeverity()) : null;
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
}
