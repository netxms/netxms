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
package org.netxms.nxmc.modules.snmp.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.base.views.AbstractTraceView;
import org.netxms.nxmc.base.widgets.AbstractTraceWidget;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.snmp.widgets.SnmpTrapTraceWidget;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * SNMP trap monitor
 */
public class SnmpTrapMonitor extends AbstractTraceView
{
   /**
    * Create view
    */
   public SnmpTrapMonitor()
   {
      super(LocalizationHelper.getI18n(SnmpTrapMonitor.class).tr("SNMP Traps"), ResourceManager.getImageDescriptor("icons/monitor-views/snmp-trap-monitor.png"), "monitor.snmp-traps");
   }

   /**
    * @see org.netxms.nxmc.base.views.AbstractTraceView#createTraceWidget(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected AbstractTraceWidget createTraceWidget(Composite parent)
	{
      return new SnmpTrapTraceWidget(parent, SWT.NONE, this);
	}

   /**
    * @see org.netxms.nxmc.base.views.AbstractTraceView#getChannelName()
    */
   @Override
   protected String getChannelName()
   {
      return NXCSession.CHANNEL_SNMP_TRAPS;
   }
}
