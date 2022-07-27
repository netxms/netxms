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
import org.netxms.nxmc.modules.events.views.SyslogMonitor;
import org.netxms.nxmc.services.MonitorPerspectiveElement;

/**
 * Registration for syslog monitor view
 */
public class SyslogMonitorRegistration implements MonitorPerspectiveElement
{
   /**
    * @see org.netxms.nxmc.services.MonitorPerspectiveElement#createView()
    */
   @Override
   public AbstractTraceView createView()
   {
      return new SyslogMonitor();
   }
}
