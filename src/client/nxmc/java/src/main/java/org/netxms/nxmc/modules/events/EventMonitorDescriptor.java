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
package org.netxms.nxmc.modules.events;

import org.netxms.nxmc.base.views.AbstractTraceView;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.views.EventMonitor;
import org.netxms.nxmc.services.MonitorDescriptor;
import org.xnap.commons.i18n.I18n;

/**
 * Registration for event monitor view
 */
public class EventMonitorDescriptor implements MonitorDescriptor
{
   private final I18n i18n = LocalizationHelper.getI18n(EventMonitorDescriptor.class);

   /**
    * @see org.netxms.nxmc.services.MonitorDescriptor#createView()
    */
   @Override
   public AbstractTraceView createView()
   {
      return new EventMonitor();
   }

   /**
    * @see org.netxms.nxmc.services.MonitorDescriptor#getDisplayName()
    */
   @Override
   public String getDisplayName()
   {
      return i18n.tr("Events");
   }

   /**
    * @see org.netxms.nxmc.services.MonitorDescriptor#getRequiredComponentId()
    */
   @Override
   public String getRequiredComponentId()
   {
      return null;
   }
}
