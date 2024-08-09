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
import org.netxms.nxmc.modules.events.widgets.helpers.EventLabelProvider;
import org.netxms.nxmc.modules.events.widgets.helpers.EventMonitorFilter;
import org.xnap.commons.i18n.I18n;

/**
 * Event trace widget
 */
public class EventTraceWidget extends AbstractTraceWidget implements SessionListener
{
	public static final int COLUMN_TIMESTAMP = 0;
	public static final int COLUMN_SOURCE = 1;
	public static final int COLUMN_SEVERITY = 2;
	public static final int COLUMN_EVENT = 3;
	public static final int COLUMN_MESSAGE = 4;

   private I18n i18n;

	private NXCSession session;
	private Action actionShowColor; 
	private Action actionShowIcons;
	private EventLabelProvider labelProvider;

	/**
	 * @param parent
	 * @param style
	 * @param viewPart
	 */
   public EventTraceWidget(Composite parent, int style, View view)
	{
      super(parent, style, view);

      session = Registry.getSession();
		session.addListener(this);
      addDisposeListener((e) -> session.removeListener(EventTraceWidget.this));
	}

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractTraceWidget#setupLocalization()
    */
   @Override
   protected void setupLocalization()
   {
      i18n = LocalizationHelper.getI18n(EventTraceWidget.class);
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#setupViewer(org.eclipse.jface.viewers.TableViewer)
    */
	@Override
	protected void setupViewer(TableViewer viewer)
	{
		labelProvider = new EventLabelProvider();
		viewer.setLabelProvider(labelProvider);

      final PreferenceStore settings = PreferenceStore.getInstance();
      labelProvider.setShowColor(settings.getAsBoolean("EventMonitor.showColor", true));
      labelProvider.setShowIcons(settings.getAsBoolean("EventMonitor.showIcons", false));

      addColumn(i18n.tr("Timestamp"), 150);
      addColumn(i18n.tr("Source"), 200);
      addColumn(i18n.tr("Severity"), 90);
      addColumn(i18n.tr("Event"), 200);
      addColumn(i18n.tr("Message"), 600);
	}

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractTraceWidget#createFilter()
    */
   @Override
   protected AbstractTraceViewFilter createFilter()
   {
      return new EventMonitorFilter();
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#saveConfig()
    */
	@Override
	protected void saveConfig()
	{
		super.saveConfig();

      final PreferenceStore ps = PreferenceStore.getInstance();
      ps.set("EventMonitor.showColor", labelProvider.isShowColor());
      ps.set("EventMonitor.showIcons", labelProvider.isShowIcons());
	}

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#getConfigPrefix()
    */
	@Override
	protected String getConfigPrefix()
	{
      return "EventMonitor";
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
      if (n.getCode() == SessionNotification.NEW_EVENTLOG_RECORD)
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
