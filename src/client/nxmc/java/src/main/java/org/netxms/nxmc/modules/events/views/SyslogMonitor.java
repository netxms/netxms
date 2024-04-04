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
package org.netxms.nxmc.modules.events.views;

import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.base.views.AbstractTraceView;
import org.netxms.nxmc.base.widgets.AbstractTraceWidget;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.SyslogTraceWidget;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Syslog monitor
 */
public class SyslogMonitor extends AbstractTraceView
{
   /**
    * Create view
    */
   public SyslogMonitor()
   {
      super(LocalizationHelper.getI18n(SyslogMonitor.class).tr("Syslog"), ResourceManager.getImageDescriptor("icons/monitor-views/syslog-monitor.png"), "monitor.syslog");
   }

   /**
    * @see org.netxms.nxmc.base.views.AbstractTraceView#fillLocalMenu(IMenuManager)
    */
	@Override
   protected void fillLocalMenu(IMenuManager manager)
	{
      super.fillLocalMenu(manager);
		manager.add(new Separator());
      manager.add(((SyslogTraceWidget)getTraceWidget()).getActionShowColor());
      manager.add(((SyslogTraceWidget)getTraceWidget()).getActionShowIcons());
	}

   /**
    * @see org.netxms.nxmc.base.views.AbstractTraceView#createTraceWidget(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected AbstractTraceWidget createTraceWidget(Composite parent)
	{
      return new SyslogTraceWidget(parent, SWT.NONE, this);
	}

   /**
    * @see org.netxms.nxmc.base.views.AbstractTraceView#getChannelName()
    */
   @Override
   protected String getChannelName()
   {
      return NXCSession.CHANNEL_SYSLOG;
   }
}
