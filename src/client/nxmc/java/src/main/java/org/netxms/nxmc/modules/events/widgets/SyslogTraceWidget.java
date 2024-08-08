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
package org.netxms.nxmc.modules.events.widgets;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.AbstractTraceWidget;
import org.netxms.nxmc.base.widgets.helpers.AbstractTraceViewFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.helpers.SyslogLabelProvider;
import org.netxms.nxmc.modules.events.widgets.helpers.SyslogMonitorFilter;
import org.xnap.commons.i18n.I18n;

/**
 * Syslog trace widget
 */
public class SyslogTraceWidget extends AbstractTraceWidget implements SessionListener
{
	public static final int COLUMN_TIMESTAMP = 0;
	public static final int COLUMN_SOURCE = 1;
	public static final int COLUMN_SEVERITY = 2;
	public static final int COLUMN_FACILITY = 3;
	public static final int COLUMN_HOSTNAME = 4;
	public static final int COLUMN_TAG = 5;
	public static final int COLUMN_MESSAGE = 6;

   private I18n i18n;
	private NXCSession session;
	private Action actionShowColor;
	private Action actionShowIcons;
	private SyslogLabelProvider labelProvider;

   /**
    * Create syslog trace widget.
    *
    * @param parent parent composite
    * @param style composite style
    * @param view owning view
    */
   public SyslogTraceWidget(Composite parent, int style, View view)
	{
      super(parent, style, view);

      session = Registry.getSession();
		session.addListener(this);
      addDisposeListener((e) -> session.removeListener(SyslogTraceWidget.this));
	}

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractTraceWidget#setupLocalization()
    */
   @Override
   protected void setupLocalization()
   {
      i18n = LocalizationHelper.getI18n(SyslogTraceWidget.class);
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#setupViewer(org.eclipse.jface.viewers.TableViewer)
    */
	@Override
	protected void setupViewer(TableViewer viewer)
	{
		labelProvider = new SyslogLabelProvider();
		viewer.setLabelProvider(labelProvider);

      final PreferenceStore ps = PreferenceStore.getInstance();
      labelProvider.setShowColor(ps.getAsBoolean("SyslogMonitor.showColor", labelProvider.isShowColor()));
      labelProvider.setShowIcons(ps.getAsBoolean("SyslogMonitor.showIcons", labelProvider.isShowIcons()));

      addColumn(i18n.tr("Timestamp"), 150);
      addColumn(i18n.tr("Source"), 200);
      addColumn(i18n.tr("Severity"), 90);
      addColumn(i18n.tr("Facility"), 90);
      addColumn(i18n.tr("Host name"), 130);
      addColumn(i18n.tr("Tag"), 90);
      addColumn(i18n.tr("Message"), 600);
	}

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractTraceWidget#createFilter()
    */
   @Override
   protected AbstractTraceViewFilter createFilter()
   {
      return new SyslogMonitorFilter();
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#getConfigPrefix()
    */
	@Override
	protected String getConfigPrefix()
	{
      return "SyslogMonitor";
	}

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#saveConfig()
    */
	@Override
	protected void saveConfig()
	{
		super.saveConfig();

      PreferenceStore ps = PreferenceStore.getInstance();
      ps.set("SyslogMonitor.showColor", labelProvider.isShowColor());
      ps.set("SyslogMonitor.showIcons", labelProvider.isShowIcons());
	}

	/**
	 * @see org.netxms.nxmc.base.widgets.AbstractTraceWidget#createActions()
	 */
	@Override
	protected void createActions()
	{
		super.createActions();

      actionShowColor = new Action(i18n.tr("Show status &colors"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowColor(actionShowColor.isChecked());
				refresh();
			}
		};
		actionShowColor.setChecked(labelProvider.isShowColor());

      actionShowIcons = new Action(i18n.tr("Show status &icons"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowIcons(actionShowIcons.isChecked());
				refresh();
			}
		};
		actionShowIcons.setChecked(labelProvider.isShowIcons());
	}

   /**
    * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
    */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
		if (n.getCode() == SessionNotification.NEW_SYSLOG_RECORD)
		{
         runInUIThread(() -> addElement(n.getObject()));
		}
	}

	/**
	 * @return the actionShowColor
	 */
	public Action getActionShowColor()
	{
		return actionShowColor;
	}

	/**
	 * @return the actionShowIcons
	 */
	public Action getActionShowIcons()
	{
		return actionShowIcons;
	}
}
