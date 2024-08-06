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
package org.netxms.nxmc.modules.alarms.views;

import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.localization.LocalizationHelper;

/**
 * Ad-hoc alarms view - display alarms for object X in context of object Y
 */
public class AdHocAlarmsView extends AlarmsView
{
   private long contextObjectId;
   private long alarmSourceId;

   /**
    * Create new ad-hoc alarms view.
    *
    * @param contextObjectId context object ID
    * @param alarmSource alarm source (object to display alarms for)
    */
   public AdHocAlarmsView(long contextObjectId, AbstractObject alarmSource)
   {
      super(LocalizationHelper.getI18n(AdHocAlarmsView.class).tr("Alarms - {0}", alarmSource.getObjectName()), "objects.alarms." + contextObjectId + "." + alarmSource.getObjectId());
      this.contextObjectId = contextObjectId;
      this.alarmSourceId = alarmSource.getObjectId();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject) && (((AbstractObject)context).getObjectId() == contextObjectId);
   }

   /**
    * @see org.netxms.nxmc.modules.alarms.views.AlarmsView#getPriority()
    */
   @Override
   public int getPriority()
   {
      return DEFAULT_PRIORITY;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }

   /**
    * @see org.netxms.nxmc.modules.alarms.views.AlarmsView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      // Ignore object change
   }

   /**
    * @see org.netxms.nxmc.modules.alarms.views.AlarmsView#getContextName()
    */
   @Override
   protected String getContextName()
   {
      return session.getObjectName(alarmSourceId);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      alarmList.setRootObject(alarmSourceId);
   }
}
