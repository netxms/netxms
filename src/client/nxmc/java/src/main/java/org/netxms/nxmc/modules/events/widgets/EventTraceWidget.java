/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.events.Event;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.AbstractTraceWidget;
import org.netxms.nxmc.base.widgets.helpers.AbstractTraceViewFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.helpers.EventLabelProvider;
import org.netxms.nxmc.modules.events.widgets.helpers.EventMonitorFilter;
import org.netxms.nxmc.modules.events.widgets.helpers.HistoricalEvent;
import org.netxms.nxmc.modules.logviewer.views.AdHocEventLogView;
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
   private EventInterceptor eventInterceptor;

   /**
    * Interface for intercepting real-time events during initial loading.
    */
   public interface EventInterceptor
   {
      /**
       * Called when a real-time event is received.
       *
       * @param event the event object
       * @return true if the event was intercepted and should not be added to widget
       */
      boolean interceptEvent(Object event);
   }

	/**
	 * @param parent
	 * @param style
	 * @param viewPart
	 */
   public EventTraceWidget(Composite parent, int style, View view)
	{
      super(parent, style, view);
      System.out.println("Creating EventTraceWidget");

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
    * @see org.netxms.nxmc.base.widgets.AbstractTraceWidget#fillContextMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillContextMenu(IMenuManager manager)
   {
      super.fillContextMenu(manager);

      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() == 1)
      {
         Object element = selection.getFirstElement();
         if ((element instanceof Event) || (element instanceof HistoricalEvent))
         {
            manager.add(new Separator());
            MenuManager openEventLogMenu = new MenuManager(i18n.tr("Open event log"));
            addOpenEventLogActions(openEventLogMenu, element);
            manager.add(openEventLogMenu);
         }
      }
   }

   /**
    * Add "Open Event Log" submenu actions.
    *
    * @param manager menu manager
    * @param event selected event (Event or HistoricalEvent)
    */
   private void addOpenEventLogActions(IMenuManager manager, Object event)
   {
      final long sourceId;
      final int code;

      if (event instanceof Event)
      {
         sourceId = ((Event)event).getSourceId();
         code = ((Event)event).getCode();
      }
      else
      {
         sourceId = ((HistoricalEvent)event).getSourceId();
         code = ((HistoricalEvent)event).getCode();
      }

      String sourceName = session.getObjectName(sourceId);
      String eventName = session.getEventName(code);

      manager.add(new Action(i18n.tr("Filter by source ({0})", sourceName)) {
         @Override
         public void run()
         {
            openEventLogFiltered(sourceId, null, sourceName);
         }
      });

      manager.add(new Action(i18n.tr("Filter by event type ({0})", eventName)) {
         @Override
         public void run()
         {
            openEventLogFiltered(null, code, eventName);
         }
      });

      manager.add(new Action(i18n.tr("Filter by source and event type")) {
         @Override
         public void run()
         {
            openEventLogFiltered(sourceId, code, sourceName + " / " + eventName);
         }
      });
   }

   /**
    * Open Event Log view with specified filter.
    *
    * @param sourceId source object ID (null for no filter)
    * @param eventCode event code (null for no filter)
    * @param titleSuffix suffix for view title
    */
   private void openEventLogFiltered(Long sourceId, Integer eventCode, String titleSuffix)
   {
      AdHocEventLogView logView = new AdHocEventLogView(sourceId, eventCode, titleSuffix);
      view.openView(logView);
   }

   /**
    * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      if (n.getCode() == SessionNotification.NEW_EVENTLOG_RECORD)
      {
         runInUIThread(() -> {
            if ((eventInterceptor == null) || !eventInterceptor.interceptEvent(n.getObject()))
            {
               addElement(n.getObject());
            }
         });
      }
   }

   /**
    * Set event interceptor for handling real-time events during initial loading.
    *
    * @param interceptor the interceptor to set, or null to disable interception
    */
   public void setEventInterceptor(EventInterceptor interceptor)
   {
      this.eventInterceptor = interceptor;
   }

   /**
    * Add event element to the trace widget.
    *
    * @param event event to add
    */
   public void addEvent(Object event)
   {
      addElement(event);
   }

   /**
    * Set event code filter for client-side filtering.
    *
    * @param codes array of allowed event codes (null or empty for no filter)
    */
   public void setEventCodeFilter(int[] codes)
   {
      ((EventMonitorFilter)filter).setEventCodes(codes);
      refresh();
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
