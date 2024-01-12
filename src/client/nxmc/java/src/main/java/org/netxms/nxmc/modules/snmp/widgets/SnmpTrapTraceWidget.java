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
package org.netxms.nxmc.modules.snmp.widgets;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.AbstractTraceWidget;
import org.netxms.nxmc.base.widgets.helpers.AbstractTraceViewFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.snmp.widgets.helpers.SnmpTrapMonitorFilter;
import org.netxms.nxmc.modules.snmp.widgets.helpers.SnmpTrapMonitorLabelProvider;
import org.xnap.commons.i18n.I18n;

/**
 * SNMP trap trace widget
 */
public class SnmpTrapTraceWidget extends AbstractTraceWidget implements SessionListener
{
	public static final int COLUMN_TIMESTAMP = 0;
	public static final int COLUMN_SOURCE_IP = 1;
	public static final int COLUMN_SOURCE_NODE = 2;
	public static final int COLUMN_OID = 3;
	public static final int COLUMN_VARBINDS = 4;

   private I18n i18n;
   private NXCSession session = Registry.getSession();

	/**
    * @param parent
    * @param style
    * @param view
    */
   public SnmpTrapTraceWidget(Composite parent, int style, View view)
	{
      super(parent, style, view);

		session.addListener(this);
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				session.removeListener(SnmpTrapTraceWidget.this);
			}
		});
	}

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractTraceWidget#setupLocalization()
    */
   @Override
   protected void setupLocalization()
   {
      i18n = LocalizationHelper.getI18n(SnmpTrapTraceWidget.class);
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#setupViewer(org.eclipse.jface.viewers.TableViewer)
    */
	@Override
	protected void setupViewer(TableViewer viewer)
	{
      addColumn(i18n.tr("Timestamp"), 150);
      addColumn(i18n.tr("Source IP"), 120);
      addColumn(i18n.tr("Source node"), 200);
      addColumn(i18n.tr("OID"), 200);
      addColumn(i18n.tr("Varbinds"), 600);

		viewer.setLabelProvider(new SnmpTrapMonitorLabelProvider());
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractTraceWidget#createFilter()
    */
   @Override
   protected AbstractTraceViewFilter createFilter()
   {
      return new SnmpTrapMonitorFilter();
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#getConfigPrefix()
    */
	@Override
	protected String getConfigPrefix()
	{
      return "SnmpTrapMonitor";
	}

   /**
    * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
    */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
		if (n.getCode() == SessionNotification.NEW_SNMP_TRAP)
		{
         runInUIThread(() -> addElement(n.getObject()));
		}
	}
}
