/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.alarms.widgets;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.apache.commons.lang3.SystemUtils;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.util.IPropertyChangeListener;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseTrackAdapter;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.program.Program;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.BulkAlarmStateChangeData;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.CompositeWithMessageBar;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.MessageBar;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.AlarmNotifier;
import org.netxms.nxmc.modules.alarms.dialogs.AcknowledgeCustomTimeDialog;
import org.netxms.nxmc.modules.alarms.dialogs.AlarmStateChangeFailureDialog;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmAcknowledgeTimeFunctions;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmComparator;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmListFilter;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmListLabelProvider;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmToolTip;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmTreeContentProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.RefreshTimer;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Alarm list widget
 */
public class AlarmList extends CompositeWithMessageBar
{
   // Columns
   public static final int COLUMN_SEVERITY = 0;
   public static final int COLUMN_STATE = 1;
   public static final int COLUMN_SOURCE = 2;
   public static final int COLUMN_ZONE = 3;
   public static final int COLUMN_MESSAGE = 4;
   public static final int COLUMN_COUNT = 5;
   public static final int COLUMN_COMMENTS = 6;
   public static final int COLUMN_HELPDESK_REF = 7;
	public static final int COLUMN_ACK_BY = 8;
	public static final int COLUMN_CREATED = 9;
	public static final int COLUMN_LASTCHANGE = 10;
	
   private static Logger logger = LoggerFactory.getLogger(AlarmList.class);

   private I18n i18n = LocalizationHelper.getI18n(AlarmList.class);
   private View view;
	private NXCSession session = null;
	private SessionListener clientListener = null;
	private RefreshTimer refreshTimer;
	private SortableTreeViewer alarmViewer;
   private AlarmListLabelProvider labelProvider;
	private AlarmListFilter alarmFilter;
   private FilterText filterText;
	private Point toolTipLocation;
	private Alarm toolTipObject;
	private Map<Long, Alarm> alarmList = new HashMap<Long, Alarm>();
   private List<Alarm> newAlarmList = new ArrayList<Alarm>();
   private VisibilityValidator visibilityValidator;
   private boolean needInitialRefresh = false;
   private boolean filterRunning = false;
   private boolean filterRunPending = false;
	private Action actionCopy;
	private Action actionCopyMessage;
	private Action actionComments;
	private Action actionAcknowledge;
	private Action actionResolve;
	private Action actionStickyAcknowledge;
	private Action actionTerminate;
	private Action actionShowObjectDetails;
   private Action actionCreateIssue;
   private Action actionShowIssue;
   private Action actionUnlinkIssue;
   private Action actionExportToCsv;
   private Action actionShowFilter;
   private MenuManager timeAcknowledgeMenu;
   private List<Action> timeAcknowledge;
   private Action timeAcknowledgeOther;
   private Action actionShowColor;
   private boolean initShowfilter;
   private boolean isLocalNotificationsEnabled = false;

   /**
    * Create alarm list widget
    * 
    * @param viewPart owning view part
    * @param parent parent composite
    * @param style widget style
    * @param configPrefix prefix for saving/loading widget configuration
    */
   public AlarmList(View view, Composite parent, int style, final String configPrefix, VisibilityValidator visibilityValidator)
	{
		super(parent, style);

      session = Registry.getSession();
      this.view = view;
		this.visibilityValidator = visibilityValidator;

      getContent().setLayout(new FormLayout());

      // Create filter area
      filterText = new FilterText(getContent(), SWT.NONE, null, true);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            actionShowFilter.setChecked(false);
         }
      });
		
		// Setup table columns
		final String[] names = { 
		      i18n.tr("Severity"), 
		      i18n.tr("State"),
		      i18n.tr("Source"),
		      i18n.tr("Zone"),
		      i18n.tr("Message"),
		      i18n.tr("Count"),
		      i18n.tr("Comments"),
		      i18n.tr("Helpdesk ID"), 
		      i18n.tr("Ack/Resolve By"),
		      i18n.tr("Created"), 
		      i18n.tr("Last Change")
		   };
		final int[] widths = { 100, 100, 150, 130, 300, 70, 70, 120, 100, 100, 100 };
		alarmViewer = new SortableTreeViewer(getContent(), names, widths, 0, SWT.DOWN, SortableTreeViewer.DEFAULT_STYLE);
      if (!session.isZoningEnabled())
         alarmViewer.removeColumnById(COLUMN_ZONE);
      WidgetHelper.restoreTreeViewerSettings(alarmViewer, PreferenceStore.getInstance(), configPrefix);

      labelProvider = new AlarmListLabelProvider(alarmViewer);
      labelProvider.setShowColor(PreferenceStore.getInstance().getAsBoolean("AlarmList.ShowStatusColor", false));
      alarmViewer.setLabelProvider(labelProvider);
      alarmViewer.setContentProvider(new AlarmTreeContentProvider());
      alarmViewer.setComparator(new AlarmComparator());
      alarmFilter = new AlarmListFilter();
      alarmViewer.addFilter(alarmFilter);
      alarmViewer.getTree().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTreeViewerSettings(alarmViewer, PreferenceStore.getInstance(), configPrefix);
         }
      });

      final Runnable toolTipTimer = new Runnable() {
         @Override
         public void run()
         {
            AlarmToolTip t = new AlarmToolTip(alarmViewer.getTree(), toolTipObject);
            t.show(toolTipLocation);
         }
      };

      alarmViewer.getTree().addMouseTrackListener(new MouseTrackAdapter() {
         @Override
         public void mouseHover(MouseEvent e)
         {
            if (!PreferenceStore.getInstance().getAsBoolean("AlarmList.ShowTooltips", false)) //$NON-NLS-1$
               return;

            Point p = new Point(e.x, e.y);
            TreeItem item = alarmViewer.getTree().getItem(p);
            if (item == null)
               return;
            toolTipObject = (Alarm)item.getData();
            p.x -= 10;
            p.y -= 10;
            toolTipLocation = p;
            getDisplay().timerExec(300, toolTipTimer);
         }

         @Override
         public void mouseExit(MouseEvent e)
         {
            getDisplay().timerExec(-1, toolTipTimer);
         }
      });

      // Get filter settings
      final PreferenceStore ps = PreferenceStore.getInstance();
      initShowfilter = ps.getAsBoolean("AlarmList.ShowFilter", true);
		
		createActions();
		createPopupMenu();

      if ((visibilityValidator == null) || visibilityValidator.isVisible())
         refresh();
      else
         needInitialRefresh = true;

      refreshTimer = new RefreshTimer(session.getMinViewRefreshInterval(), alarmViewer.getControl(), new Runnable() {
         @Override
         public void run()
         {
            startFilterAndLimit();
         }
      });

      // Add client library listener
      clientListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            switch(n.getCode())
            {
               case SessionNotification.NEW_ALARM:
                  synchronized(newAlarmList)
                  {
                     newAlarmList.add((Alarm)n.getObject()); // Add to this list only new alarms to be able to notify with sound
                  }
               case SessionNotification.ALARM_CHANGED:
                  synchronized(alarmList)
                  {
                     alarmList.put(((Alarm)n.getObject()).getId(), (Alarm)n.getObject());
                  }
                  if (alarmFilter.filter((Alarm)n.getObject()))
                     refreshTimer.execute();
                  break;
               case SessionNotification.ALARM_TERMINATED:
               case SessionNotification.ALARM_DELETED:
                  synchronized(alarmList)
                  {
                     alarmList.remove(((Alarm)n.getObject()).getId());
                  }
                  refreshTimer.execute();
                  break;
               case SessionNotification.MULTIPLE_ALARMS_RESOLVED:
                  synchronized(alarmList)
                  {
                     BulkAlarmStateChangeData d = (BulkAlarmStateChangeData)n.getObject();
                     for(Long id : d.getAlarms())
                     {
                        Alarm a = alarmList.get(id);
                        if (a != null)
                        {
                           a.setResolved(d.getUserId(), d.getChangeTime());
                        }
                     }
                  }
                  refreshTimer.execute();
                  break;
               case SessionNotification.MULTIPLE_ALARMS_TERMINATED:
                  synchronized(alarmList)
                  {
                     for(Long id : ((BulkAlarmStateChangeData)n.getObject()).getAlarms())
                     {
                        alarmList.remove(id);
                     }
                  }
                  refreshTimer.execute();
                  break;
               default:
                  break;
            }
         }
      };
      session.addListener(clientListener);

      final Runnable blinkTimer = new Runnable() {
         @Override
         public void run()
         {
            if (isDisposed())
               return;

            int count = 0;
            synchronized(alarmList)
            {
               for(Alarm a : alarmList.values())
                  if (a.getState() == Alarm.STATE_OUTSTANDING)
                     count++;
            }

            if (count > 0)
            {
               ((AlarmListLabelProvider)alarmViewer.getLabelProvider()).toggleBlinkState();
               alarmViewer.refresh();
            }
            getDisplay().timerExec(500, this);
         }
      };

      if (ps.getAsBoolean("AlarmList.BlinkOutstandingAlarm", false)) //$NON-NLS-1$
         getDisplay().timerExec(500, blinkTimer);
      final IPropertyChangeListener propertyChangeListener = new IPropertyChangeListener() {
         @Override
         public void propertyChange(PropertyChangeEvent event)
         {
            if (event.getProperty().equals("AlarmList.BlinkOutstandingAlarm")) //$NON-NLS-1$
            {
               if (ps.getAsBoolean("AlarmList.BlinkOutstandingAlarm", false)) //$NON-NLS-1$
                  getDisplay().timerExec(500, blinkTimer);
               else
                  getDisplay().timerExec(-1, blinkTimer);
            }
            else if (event.getProperty().equals("AlarmList.ShowStatusColor")) //$NON-NLS-1$
            {
               boolean showColors = ps.getAsBoolean("AlarmList.ShowStatusColor", false);
               if (labelProvider.isShowColor() != showColors)
               {
                  labelProvider.setShowColor(showColors);
                  actionShowColor.setChecked(showColors);
                  alarmViewer.refresh();
               }
            }
         }
      };
      ps.addPropertyChangeListener(propertyChangeListener);

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            ps.removePropertyChangeListener(propertyChangeListener);
            if ((session != null) && (clientListener != null))
               session.removeListener(clientListener);
            ps.set("AlarmList.ShowFilter", initShowfilter);
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      alarmViewer.getControl().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      // Set initial focus to filter input line
      if (initShowfilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly*/
   }
	
   /**
    * Get selection provider of alarm list
    * 
    * @return
    */
   public ISelectionProvider getSelectionProvider()
   {
      return alarmViewer;
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCopy = new Action(i18n.tr("&Copy to clipboard")) {
			@Override
			public void run()
			{
				TreeItem[] selection = alarmViewer.getTree().getSelection();
				if (selection.length > 0)
				{
               final String newLine = SystemUtils.IS_OS_WINDOWS ? "\r\n" : "\n"; //$NON-NLS-1$ //$NON-NLS-2$
					StringBuilder sb = new StringBuilder();
					for(int i = 0; i < selection.length; i++)
					{
						if (i > 0)
							sb.append(newLine);
						sb.append('[');
						sb.append(selection[i].getText(WidgetHelper.getColumnIndexById(alarmViewer.getTree(), COLUMN_SEVERITY)));
						sb.append("]\t"); //$NON-NLS-1$
						sb.append(selection[i].getText(WidgetHelper.getColumnIndexById(alarmViewer.getTree(), COLUMN_SOURCE)));
						sb.append('\t');
						sb.append(selection[i].getText(WidgetHelper.getColumnIndexById(alarmViewer.getTree(), COLUMN_MESSAGE)));
					}
					WidgetHelper.copyToClipboard(sb.toString());
				}
			}
		};
      actionCopy.setId("AlarmList.Copy");

      actionCopyMessage = new Action(i18n.tr("Copy &message to clipboard")) {
			@Override
			public void run()
			{
				TreeItem[] selection = alarmViewer.getTree().getSelection();
				if (selection.length > 0)
				{
               final String newLine = SystemUtils.IS_OS_WINDOWS ? "\r\n" : "\n";
					StringBuilder sb = new StringBuilder();
					for(int i = 0; i < selection.length; i++)
					{
						if (i > 0)
							sb.append(newLine);
						sb.append(selection[i].getText(WidgetHelper.getColumnIndexById(alarmViewer.getTree(), COLUMN_MESSAGE)));
					}
					WidgetHelper.copyToClipboard(sb.toString());
				}
			}
		};
      actionCopyMessage.setId("AlarmList.CopyMessage"); //$NON-NLS-1$

      actionComments = new Action(i18n.tr("Comments"), SharedIcons.COMMENTS) {
         @Override
         public void run()
         {
         }
      };
      actionComments.setId("AlarmList.Comments");

      actionAcknowledge = new Action(i18n.tr("&Acknowledge"), ResourceManager.getImageDescriptor("icons/alarms/acknowledged.png")) {
			@Override
			public void run()
			{
				acknowledgeAlarms(false, 0);
			}
		};
      actionAcknowledge.setId("AlarmList.Acknowledge"); //$NON-NLS-1$

      actionStickyAcknowledge = new Action(i18n.tr("&Sticky acknowledge"), ResourceManager.getImageDescriptor("icons/alarms/acknowledged_sticky.png")) {
			@Override
			public void run()
			{
				acknowledgeAlarms(true, 0);
			}
		};
      actionStickyAcknowledge.setId("AlarmList.StickyAcknowledge"); //$NON-NLS-1$

      actionResolve = new Action(i18n.tr("&Resolve"), ResourceManager.getImageDescriptor("icons/alarms/resolved.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				resolveAlarms();
			}
		};
      actionResolve.setId("AlarmList.Resolve"); //$NON-NLS-1$

      actionTerminate = new Action(i18n.tr("&Terminate"), ResourceManager.getImageDescriptor("icons/alarms/terminated.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				terminateAlarms();
			}
		};
      actionTerminate.setId("AlarmList.Terminate"); //$NON-NLS-1$
		
      actionCreateIssue = new Action(i18n.tr("Create ticket in &helpdesk system"),
            ResourceManager.getImageDescriptor("icons/helpdesk_ticket.png")) {
         @Override
         public void run()
         {
            createIssue();
         }
      };

      actionShowIssue = new Action(i18n.tr("Show helpdesk ticket in &web browser"), SharedIcons.BROWSER) {
         @Override
         public void run()
         {
            showIssue();
         }
      };

      actionUnlinkIssue = new Action(i18n.tr("Unlink from helpdesk ticket")) {
         @Override
         public void run()
         {
            unlinkIssue();
         }
      };
      
      actionShowObjectDetails = new Action(i18n.tr("Show &object details")) {
			@Override
			public void run()
			{
			}
		};
      actionShowObjectDetails.setId("AlarmList.ShowObjectDetails");

      actionExportToCsv = new ExportToCsvAction(view, alarmViewer, true);

		//time based sticky acknowledgement	
      timeAcknowledgeOther = new Action("Other...") {
         @Override
         public void run()
         {
            AcknowledgeCustomTimeDialog dlg = new AcknowledgeCustomTimeDialog(view.getWindow().getShell());
            if (dlg.open() == Window.OK)
            {
               int time = dlg.getTime();
               if (time > 0)
                  acknowledgeAlarms(true, time);
            }
         }
      };
      timeAcknowledgeOther.setId("AlarmList.TimeAcknowledgeOther"); //$NON-NLS-1$

      actionShowColor = new Action(i18n.tr("Show status &colors"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            labelProvider.setShowColor(actionShowColor.isChecked());
            alarmViewer.refresh();
            PreferenceStore.getInstance().set("AlarmList.ShowStatusColor", actionShowColor.isChecked());
         }
      };
      actionShowColor.setChecked(labelProvider.isShowColor());
      
      actionShowFilter = new Action(i18n.tr("Show filter")) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(initShowfilter);
      actionShowFilter.setActionDefinitionId("AlarmList.ShowFilter");
	}

	/**
    * Initialize timed acknowledge actions and configuration
    */
	private void initializeTimeAcknowledge()
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      int menuSize = settings.getAsInteger("AlarmList.AckMenuSize", 0);
      if (menuSize < 1)
      {
         settings.set("AlarmList.AckMenuSize", 4); //$NON-NLS-1$
         timeAcknowledge = new ArrayList<Action>(4);
         createDefaultIntervals();
         settings.set("AlarmList.AckMenuEntry.0", 1 * 60 * 60); //$NON-NLS-1$
         settings.set("AlarmList.AckMenuEntry.1", 4 * 60 * 60); //$NON-NLS-1$
         settings.set("AlarmList.AckMenuEntry.2", 24 * 60 * 60); //$NON-NLS-1$
         settings.set("AlarmList.AckMenuEntry.3", 2 * 24 * 60 * 60); //$NON-NLS-1$
         return;
	   }
	   timeAcknowledge = new ArrayList<Action>(menuSize);
      for(int i = 0; i < menuSize; i++)
      {
         final int time = settings.getAsInteger("AlarmList.AckMenuEntry." + Integer.toString(i), 0); //$NON-NLS-1$
	      if (time == 0)
	         continue;
	      timeAcknowledge.add(createTimeAcknowledgeAction(time, i));
	   }
   }

   /**
    * Create default intervals for timed sticky acknowledge
    */
   private void createDefaultIntervals()
   {
      timeAcknowledge.add(createTimeAcknowledgeAction(1 * 60 * 60, 0));
      timeAcknowledge.add(createTimeAcknowledgeAction(4 * 60 * 60, 1));
      timeAcknowledge.add(createTimeAcknowledgeAction(24 * 60 * 60, 2));
      timeAcknowledge.add(createTimeAcknowledgeAction(48 * 60 * 60, 2));
   }

   /**
    * Create action for time acknowledge.
    * 
    * @param time time in seconds
    * @param index action index
    * @return new action
    */
   private Action createTimeAcknowledgeAction(int time, int index)
   {
      Action action = new Action(AlarmAcknowledgeTimeFunctions.timeToString(time)) {
         @Override
         public void run()
         {
            acknowledgeAlarms(true, time);
         }
      };
      action.setId("AlarmList.TimeAcknowledge." + Integer.toString(index));
      return action;
   }

   /**
    * Create pop-up menu for alarm list
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

		// Create menu.
		Menu menu = menuMgr.createContextMenu(alarmViewer.getControl());
		alarmViewer.getControl().setMenu(menu);
   }

	/**
	 * Fill context menu
    * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
		if (selection.size() == 0)
			return;
		
		int states = getSelectionType(selection.toArray());
		
		if (states == 2)
		{
   		manager.add(actionAcknowledge);
		   manager.add(actionStickyAcknowledge);

		   if (session.isTimedAlarmAckEnabled())
		   {
      		initializeTimeAcknowledge();
            timeAcknowledgeMenu = new MenuManager(i18n.tr("Sticky acknowledge for"), "timeAcknowledge");
            for(Action act : timeAcknowledge)
            {
               timeAcknowledgeMenu.add(act);
            }
            timeAcknowledgeMenu.add(new Separator());   
            timeAcknowledgeMenu.add(timeAcknowledgeOther);
      		manager.add(timeAcknowledgeMenu);
		   }
		}
		
		if (states < 4)
		   manager.add(actionResolve);
		if (states == 4 || !session.isStrictAlarmStatusFlow())
		   manager.add(actionTerminate);
		
		manager.add(new Separator());

		if (selection.size() == 1)
		{
         // manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_TOOLS));
			manager.add(new Separator());
			manager.add(actionShowObjectDetails);
			manager.add(new Separator());
		}

      manager.add(actionCopy);
      manager.add(actionCopyMessage);
      manager.add(actionExportToCsv);

      if (selection.size() == 1)
      {
         manager.add(new Separator());
         manager.add(actionComments);
         if (session.isHelpdeskLinkActive())
         {
            manager.add(new Separator());
            if (((Alarm)selection.getFirstElement()).getHelpdeskState() == Alarm.HELPDESK_STATE_IGNORED)
            {
               manager.add(actionCreateIssue);
            }
            else
            {
               manager.add(actionShowIssue);
               if ((session.getUserSystemRights() & UserAccessRights.SYSTEM_ACCESS_UNLINK_ISSUES) != 0)
                  manager.add(actionUnlinkIssue);
            }
         }
      }
   }

   /**
    * We add 2 to status to give to outstanding status not zero meaning: 
    * STATE_OUTSTANDING + 2 = 2 
    * STATE_ACKNOWLEDGED + 2 = 3
    * STATE_RESOLVED + 2 = 4 
    * It is needed as we can't move STATE_OUTSTANDING to STATE_TERMINATED in strict flow mode. Number of status should be meaningful.
    * 
    * Then we sum all statuses with or command.
    * To STATE_ACKNOWLEDGED only from STATE_OUTSTANDING = 2, STATE_ACKNOWLEDGED = 2
    * To STATE_RESOLVED from STATE_OUTSTANDING and STATE_ACKNOWLEDGED = 2 | 3 = 3, STATE_RESOLVED <=3
    * To STATE_TERMINATED(not strict mode) from any mode(always active)
    * To STATE_TERMINATED(strict mode) only from STATE_RESOLVED = 4, STATE_TERMINATED = 4
    * More results after logical or operation
    * STATE_OUTSTANDING | STATE_RESOLVED = 6
    * STATE_ACKNOWLEDGED | STATE_RESOLVED = 7
    * STATE_OUTSTANDING | STATE_ACKNOWLEDGED | STATE_RESOLVED = 7
    * 
    * @param array selected objects array
    */
   private int getSelectionType(Object[] array)
   {
      int type = 0;
      for(int i = 0; i < array.length; i++)
      {
         type |= ((Alarm)array[i]).getState() + 2;
      }
      return type;
   }

   /**
    * Change root object for alarm list
    * 
    * @param objectId ID of new root object
    */
   public void setRootObject(long objectId)
   {
      alarmFilter.setRootObject(objectId);
      filterRunPending = true;
      doPendingUpdates();
   }

   /**
    * Change root objects for alarm list. List is refreshed after change.
    * 
    * @param List of objectId
    */
   public void setRootObjects(List<Long> selectedObjects) 
   {
      alarmFilter.setRootObjects(selectedObjects);
      filterRunPending = true;
      doPendingUpdates();
   }

   /**
    * Execute pending content updates if any
    */
   public void doPendingUpdates()
   {
      if ((visibilityValidator != null) && !visibilityValidator.isVisible())
         return;

      if (needInitialRefresh)
      {
         needInitialRefresh = false;
         refresh();
      }
      else if (filterRunPending)
      {
         startFilterAndLimit();
      }
   }

   /**
    * Call filterAndLimit() method on background thread. Should be called on UI thread.
    */
   private void startFilterAndLimit()
   {
      // Check if filtering job already running
      if (filterRunning || ((visibilityValidator != null) && !visibilityValidator.isVisible()))
      {
         filterRunPending = true;
         return;
      }

      filterRunning = true;
      filterRunPending = false;

      Job job = new Job(i18n.tr("Filter alarms"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            synchronized(alarmList)
            {
               filterAndLimit();
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot filter alartm list");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Filter all alarms (e.g. by chosen object), sort them by last change and reduce the size to maximum as it is set in
    * configuration parameter <code>AlarmListDisplayLimit</code>, and update list control.
    * This method should be called on background thread with alarm list locked.
    */
   private void filterAndLimit()
   {
      // filter
      final List<Alarm> selectedAlarms = new ArrayList<Alarm>();
      for(Alarm alarm : alarmList.values())
      {
         if (alarmFilter.filter(alarm))
         {
            selectedAlarms.add(alarm);
         }
      }

      // limit number of alarms to display
      final List<Alarm> filteredAlarms;
      if ((session.getAlarmListDisplayLimit() > 0) && (selectedAlarms.size() > session.getAlarmListDisplayLimit()))
      {
         // sort by last change - newest first
         Collections.sort(selectedAlarms, new Comparator<Alarm>() {
            @Override
            public int compare(Alarm alarm1, Alarm alarm2)
            {
               return -(alarm1.getLastChangeTime().compareTo(alarm2.getLastChangeTime()));
            }
         });

         filteredAlarms = selectedAlarms.subList(0, session.getAlarmListDisplayLimit());
      }
      else
      {
         filteredAlarms = selectedAlarms;
      }

      alarmViewer.getControl().getDisplay().asyncExec(new Runnable() {
         @Override
         public void run()
         {
            if (alarmViewer.getControl().isDisposed())
               return;

            alarmViewer.setInput(filteredAlarms);

            // For some unknown reason setInput may remove sort indicator from column - this call will restore it
            alarmViewer.getTree().setSortColumn(alarmViewer.getTree().getSortColumn());

            if ((session.getAlarmListDisplayLimit() > 0) && (selectedAlarms.size() >= session.getAlarmListDisplayLimit()))
            {
               showMessage(MessageBar.INFORMATION,
                     String.format(i18n.tr("Only %d most recent alarms shown"), filteredAlarms.size()));
            }
            else
            {
               hideMessage();
            }

            // Mark job end and check if another filter run is needed
            filterRunning = false;
            if (filterRunPending)
            {
               filterRunPending = false;
               refreshTimer.execute();
            }
         }
      });

      synchronized(newAlarmList)
      {
         if (!AlarmNotifier.isGlobalSoundEnabled() && (view != null) && view.isVisible() && isLocalNotificationsEnabled)
         {
            for(Alarm a : newAlarmList)
            {
               if (filteredAlarms.contains(a))
                  AlarmNotifier.playSounOnAlarm(a);
            }
         }
         newAlarmList.clear();
      }
   }

   /**
    * Refresh alarm list
    */
   public void refresh()
   {
      if ((visibilityValidator != null) && !visibilityValidator.isVisible())
	      return;

      filterRunning = true;
      filterRunPending = false;

      new Job(i18n.tr("Synchronize alarm list"), view, this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
			   HashMap<Long, Alarm> alarms = session.getAlarms();
            synchronized(alarmList)
            {
      		   alarmList.clear();
      		   alarmList.putAll(alarms);
               filterAndLimit();
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot synchronize alarm list");
         }
      }.start();
   }

	/**
	 * @param filter
	 */
	public void setStateFilter(int filter)
	{
		alarmFilter.setStateFilter(filter);
	}
	
	/**
	 * @param filter
	 */
	public void setSeverityFilter(int filter)
	{
		alarmFilter.setSeverityFilter(filter);
	}
	
	/**
    * Acknowledge selected alarms
    *
    * @param sticky
    */
	private void acknowledgeAlarms(final boolean sticky, final int time)
	{
		IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
		if (selection.size() == 0)
			return;

		final Object[] alarms = selection.toArray();
      new Job(i18n.tr("Acknowledge alarms"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            monitor.beginTask(i18n.tr("Acknowledging alarms..."), alarms.length);
				for(Object o : alarms)
				{
					if (monitor.isCanceled())
						break;
					if (o instanceof Alarm)
						session.acknowledgeAlarm(((Alarm)o).getId(), sticky, time);
					monitor.worked(1);
				}
				monitor.done();
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot acknowledge alarm");
			}
		}.start();
	}
		
	/**
	 * Resolve selected alarms
	 */
	private void resolveAlarms()
	{
		IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
      if (selection.isEmpty())
			return;
		
      final List<Long> alarmIds = new ArrayList<Long>(selection.size());
      for(Object o : selection.toList())
         alarmIds.add(((Alarm)o).getId());
      new Job(i18n.tr("Resolving alarms"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
			   final Map<Long, Integer> resolveFails = session.bulkResolveAlarms(alarmIds);
            if (!resolveFails.isEmpty())
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     AlarmStateChangeFailureDialog dlg = new AlarmStateChangeFailureDialog(
                           (view != null) ? view.getWindow().getShell() : null, resolveFails);
                     dlg.open();
                  }
               });
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot resolve alarm");
         }
      }.start();
   }

	/**
    * Terminate selected alarms
    */
   private void terminateAlarms()
   {
      IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
      if (selection.isEmpty())
         return;     

		final List<Long> alarmIds = new ArrayList<Long>(selection.size());
		for(Object o : selection.toList())
		   alarmIds.add(((Alarm)o).getId());
      new Job(i18n.tr("Terminate alarms"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{		      
		      final Map<Long, Integer> terminationFails = session.bulkTerminateAlarms(alarmIds);
				if (!terminationFails.isEmpty())
				{
				   runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     AlarmStateChangeFailureDialog dlg = new AlarmStateChangeFailureDialog(
                           (view != null) ? view.getWindow().getShell() : null, terminationFails);
                     dlg.open();
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot terminate alarm");
         }
      }.start();
   }

   /**
    * Create helpdesk ticket (issue) from selected alarms
    */
   private void createIssue()
   {
      IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
      if (selection.size() != 1)
         return;
      
      final long id = ((Alarm)selection.getFirstElement()).getId();
      new Job(i18n.tr("Create helpdesk ticket"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.openHelpdeskIssue(id);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create helpdesk ticket from alarm");
         }
      }.start();
   }

   /**
    * Show in web browser helpdesk ticket (issue) linked to selected alarm
    */
   private void showIssue()
   {
      IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
      if (selection.size() != 1)
         return;
      
      final long id = ((Alarm)selection.getFirstElement()).getId();
      new Job(i18n.tr("Show helpdesk ticket"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final String url = session.getHelpdeskIssueUrl(id);
            runInUIThread(new Runnable() { 
               @Override
               public void run()
               {
                  try
                  {
                     Program browser = Program.findProgram("html");
                     if (browser != null)
                     {
                        browser.execute(url);
                     }
                     else
                     {
                        logger.error("Program.findProgram returned null in AlarmList.showIssue");
                        MessageDialogHelper.openError(getShell(), i18n.tr("Error"),
                              i18n.tr("Internal error: unable to open web browser"));
                     }
                  }
                  catch(Exception e)
                  {
                     logger.error("Exception in AlarmList.showIssue (url=\"" + url + "\")", e);
                     MessageDialogHelper.openError(getShell(), i18n.tr("Error"),
                           i18n.tr("Internal error: unable to open web browser"));
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get URL for helpdesk ticket");
         }
      }.start();
   }

   /**
    * Unlink helpdesk ticket (issue) from selected alarm
    */
   private void unlinkIssue()
   {
      IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
      if (selection.size() != 1)
         return;

      final long id = ((Alarm)selection.getFirstElement()).getId();
      new Job(i18n.tr("Unlink alarm from helpdesk ticket"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.unlinkHelpdeskIssue(id);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot unlink alarm from helpdesk ticket");
         }
      }.start();
   }

   /**
    * Get underlying table viewer.
    * 
    * @return
    */
   public TreeViewer getViewer()
   {
      return alarmViewer;
   }

   /**
    * Get action to toggle status color display
    * 
    * @return
    */
   public IAction getActionShowColors()
   {
      return actionShowColor;
   }

   /**
    * Enable/disable status color background
    * 
    * @param show
    */
   public void setShowColors(boolean show)
   {
      labelProvider.setShowColor(show);
      actionShowColor.setChecked(show);
      alarmViewer.refresh();
      PreferenceStore.getInstance().set("AlarmList.ShowStatusColor", show);
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      alarmFilter.setFilterString(text);
      alarmViewer.refresh(false);
   }
   
   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      initShowfilter = enable;
      filterText.setVisible(initShowfilter);
      FormData fd = (FormData)alarmViewer.getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText, 0, SWT.BOTTOM) : new FormAttachment(0, 0);
      getContent().layout();
      if (enable)
         filterText.setFocus();
      else
         setFilter(""); //$NON-NLS-1$
   }
   
   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   public void setFilter(final String text)
   {
      filterText.setText(text);
      onFilterModify();
   }
   
   /**
    * @return action to show filter
    */
   public Action getActionShowFilter()
   {
      return actionShowFilter;
   }
   
   /**
    * Set action to be executed when user press "Close" button in object filter.
    * Default implementation will hide filter area without notifying parent.
    * 
    * @param action
    */
   public void setFilterCloseAction(Action action)
   {
      filterText.setCloseAction(action);
   }
   
   /**
    * @return true if filter is enabled
    */
   public boolean isFilterEnabled()
   {
      return initShowfilter;
   }

   public void setIsLocalSoundEnabled(boolean isLocalSoundEnabled)
   {
      this.isLocalNotificationsEnabled = isLocalSoundEnabled;
   }
}
