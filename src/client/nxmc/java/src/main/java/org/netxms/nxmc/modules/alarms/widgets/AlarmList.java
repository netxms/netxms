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
package org.netxms.nxmc.modules.alarms.widgets;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.util.IPropertyChangeListener;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.AlarmHandle;
import org.netxms.client.events.BulkAlarmStateChangeData;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.dialogs.CustomTimePeriodDialog;
import org.netxms.nxmc.base.helpers.TransformationSelectionProvider;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.CompositeWithMessageArea;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.base.widgets.helpers.MenuContributionItem;
import org.netxms.nxmc.base.widgets.helpers.SearchQueryAttribute;
import org.netxms.nxmc.base.widgets.helpers.SearchQueryAttributeValueProvider;
import org.netxms.nxmc.base.widgets.helpers.TimePeriodFunctions;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.AlarmNotifier;
import org.netxms.nxmc.modules.alarms.dialogs.AlarmStateChangeFailureDialog;
import org.netxms.nxmc.modules.alarms.dialogs.AssistantCommentDialog;
import org.netxms.nxmc.modules.alarms.views.AlarmDetails;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmComparator;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmListFilter;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmListLabelProvider;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmTreeContentProvider;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.ObjectMenuFactory;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ExternalWebBrowser;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.RefreshTimer;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Alarm list widget
 */
public class AlarmList extends CompositeWithMessageArea
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

   private final I18n i18n = LocalizationHelper.getI18n(AlarmList.class);

   private View view;
	private NXCSession session = null;
	private SessionListener clientListener = null;
	private RefreshTimer refreshTimer;
	private SortableTreeViewer alarmViewer;
   private AlarmListLabelProvider labelProvider;
	private AlarmListFilter alarmFilter;
   private TransformationSelectionProvider alarmSelectionProvider;
	private Map<Long, Alarm> alarmList = new HashMap<Long, Alarm>();
   private List<Alarm> newAlarmList = new ArrayList<Alarm>();
   private Set<Long> updateList = new HashSet<Long>();
   private Map<Long, AlarmHandle> displayList = new HashMap<Long, AlarmHandle>();
   private VisibilityValidator visibilityValidator;
   private boolean needInitialRefresh = false;
   private boolean filterRunning = false;
   private boolean filterRunPending = false;
	private Action actionCopy;
	private Action actionCopyMessage;
	private Action actionAcknowledge;
	private Action actionResolve;
	private Action actionStickyAcknowledge;
	private Action actionTerminate;
	private Action actionGoToObject;
	private Action actionGoToDci;
   private Action actionCreateIssue;
   private Action actionShowIssue;
   private Action actionUnlinkIssue;
   private Action actionExportToCsv;
   private Action actionShowAlarmDetails;
   private Action actionQueryAssistant;
   private MenuManager timeAcknowledgeMenu;
   private List<Action> timeAcknowledge;
   private Action timeAcknowledgeOther;
   private Action actionShowColor;
   private boolean initShowfilter;
   private boolean isLocalNotificationsEnabled = false;
   private long rootObject;
   private boolean blinkEnabled = false;
   private int warningMessageId = 0;
   private boolean aiAssistantAvailable;

   private final SearchQueryAttribute[] attributeProposals = {
         new SearchQueryAttribute("AcknowledgedBy:"),
         new SearchQueryAttribute("Event:", new EventAttributeValueProvider()),
         new SearchQueryAttribute("HasComments:", "yes", "no"),
         new SearchQueryAttribute("NOT"),
         new SearchQueryAttribute("RepeatCount:"),
         new SearchQueryAttribute("ResolvedBy:"),
         new SearchQueryAttribute("Severity:", "NORMAL", "WARNING", "MINOR", "MAJOR", "CRITICAL"),
         new SearchQueryAttribute("Source:", new SourceAttributeValueProvider()),
         new SearchQueryAttribute("State:", "Outstanding", "Acknowledged", "Resolved"),
         new SearchQueryAttribute("Zone:", new ZoneAttributeValueProvider())
   };

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
      aiAssistantAvailable = session.isServerComponentRegistered("AI-ASSISTANT");

      getContent().setLayout(new FillLayout());

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
      WidgetHelper.restoreTreeViewerSettings(alarmViewer, configPrefix);

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
            WidgetHelper.saveTreeViewerSettings(alarmViewer, configPrefix);
         }
      });
      alarmViewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionShowAlarmDetails.run();
         }
      });

      // Get filter settings
      final PreferenceStore ps = PreferenceStore.getInstance();
      initShowfilter = ps.getAsBoolean("AlarmList.ShowFilter", true);
		
		createActions();
		createContextMenu();

      if ((visibilityValidator == null) || visibilityValidator.isVisible())
         refresh();
      else
         needInitialRefresh = true;

      // Do not allow less than 500 milliseconds interval between refresh and set minimal delay to 100 milliseconds
      refreshTimer = new RefreshTimer(Math.max(session.getMinViewRefreshInterval(), 500), alarmViewer.getControl(), new Runnable() {
         @Override
         public void run()
         {
            startFilterAndLimit();
         }
      });
      refreshTimer.setMinimalDelay(100);

      // Add client library listener
      clientListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            Alarm oldAlarm;
            boolean changed;
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
                     oldAlarm = alarmList.put(((Alarm)n.getObject()).getId(), (Alarm)n.getObject());
                     updateList.add(((Alarm)n.getObject()).getId());
                  }
                  if (alarmFilter.filter((Alarm)n.getObject()) || ((oldAlarm != null) && alarmFilter.filter(oldAlarm)))
                  {
                     refreshTimer.execute();
                  }
                  break;
               case SessionNotification.ALARM_TERMINATED:
               case SessionNotification.ALARM_DELETED:
                  synchronized(alarmList)
                  {
                     oldAlarm = alarmList.remove(((Alarm)n.getObject()).getId());
                  }
                  if ((oldAlarm != null) && alarmFilter.filter(oldAlarm))
                  {
                     refreshTimer.execute();
                  }
                  break;
               case SessionNotification.MULTIPLE_ALARMS_RESOLVED:
                  changed = false;
                  synchronized(alarmList)
                  {
                     BulkAlarmStateChangeData d = (BulkAlarmStateChangeData)n.getObject();
                     for(Long id : d.getAlarms())
                     {
                        Alarm a = alarmList.get(id);
                        if (a != null)
                        {
                           a.setResolved(d.getUserId(), d.getChangeTime());
                           updateList.add(a.getId());
                           changed = true;
                        }
                     }
                  }
                  if (changed)
                     refreshTimer.execute();
                  break;
               case SessionNotification.MULTIPLE_ALARMS_TERMINATED:
                  changed = false;
                  synchronized(alarmList)
                  {
                     for(Long id : ((BulkAlarmStateChangeData)n.getObject()).getAlarms())
                     {
                        if (alarmList.remove(id) != null)
                           changed = true;
                     }
                  }
                  if (changed)
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

            if (blinkEnabled)
            {
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
            else
            {
               ((AlarmListLabelProvider)alarmViewer.getLabelProvider()).stopBlinking();
               alarmViewer.refresh();               
            }
         }
      };

      if (ps.getAsBoolean("AlarmList.BlinkOutstandingAlarm", false)) 
      {
         blinkEnabled = true;
         getDisplay().timerExec(500, blinkTimer);
      }
      final IPropertyChangeListener propertyChangeListener = new IPropertyChangeListener() {
         @Override
         public void propertyChange(PropertyChangeEvent event)
         {
            if (event.getProperty().equals("AlarmList.BlinkOutstandingAlarm")) 
            {
               if (ps.getAsBoolean("AlarmList.BlinkOutstandingAlarm", false)) 
               {
                  blinkEnabled = true;
                  getDisplay().timerExec(500, blinkTimer);
               }
               else
                  blinkEnabled = false;
            }
            else if (event.getProperty().equals("AlarmList.ShowStatusColor")) 
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

      alarmSelectionProvider = new TransformationSelectionProvider(alarmViewer) {
         @Override
         protected ISelection transformSelection(ISelection selection)
         {
            if (!(selection instanceof IStructuredSelection))
               return selection;
            List<Alarm> alarms = new ArrayList<Alarm>(((IStructuredSelection)selection).size());
            for(Object o : ((IStructuredSelection)selection).toList())
               alarms.add(((AlarmHandle)o).alarm);
            return new StructuredSelection(alarms);
         }
      };

      alarmViewer.setInput(displayList);
   }

   /**
    * Get selection provider of alarm list
    * 
    * @return
    */
   public ISelectionProvider getSelectionProvider()
   {
      return alarmSelectionProvider;
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
               final String newLine = WidgetHelper.getNewLineCharacters();
					StringBuilder sb = new StringBuilder();
					for(int i = 0; i < selection.length; i++)
					{
						if (i > 0)
							sb.append(newLine);
						sb.append('[');
						sb.append(selection[i].getText(WidgetHelper.getColumnIndexById(alarmViewer.getTree(), COLUMN_SEVERITY)));
						sb.append("]\t"); 
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
               final String newLine = WidgetHelper.getNewLineCharacters();
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
      actionCopyMessage.setId("AlarmList.CopyMessage"); 

      actionAcknowledge = new Action(i18n.tr("&Acknowledge"), ResourceManager.getImageDescriptor("icons/alarms/acknowledged.png")) {
			@Override
			public void run()
			{
				acknowledgeAlarms(false, 0);
			}
		};
      actionAcknowledge.setId("AlarmList.Acknowledge"); 

      actionStickyAcknowledge = new Action(i18n.tr("&Sticky acknowledge"), ResourceManager.getImageDescriptor("icons/alarms/acknowledged_sticky.png")) {
			@Override
			public void run()
			{
				acknowledgeAlarms(true, 0);
			}
		};
      actionStickyAcknowledge.setId("AlarmList.StickyAcknowledge"); 

      actionResolve = new Action(i18n.tr("&Resolve"), ResourceManager.getImageDescriptor("icons/alarms/resolved.png")) { 
			@Override
			public void run()
			{
				resolveAlarms();
			}
		};
      actionResolve.setId("AlarmList.Resolve"); 

      actionTerminate = new Action(i18n.tr("&Terminate"), ResourceManager.getImageDescriptor("icons/alarms/terminated.png")) { 
			@Override
			public void run()
			{
				terminateAlarms();
			}
		};
      actionTerminate.setId("AlarmList.Terminate"); 
		
      actionCreateIssue = new Action(i18n.tr("Create ticket in &helpdesk system"),
            ResourceManager.getImageDescriptor("icons/alarms/create-issue.png")) {
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
      
      actionGoToObject = new Action(i18n.tr("&Go to object")) {
			@Override
			public void run()
			{
            IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
            if (selection.size() != 1)
               return;
            
            final long id = ((Alarm)selection.getFirstElement()).getSourceObjectId();
            MainWindow.switchToObject(id, 0);
			}
		};
      actionGoToObject.setId("AlarmList.GoToObject");
      
      actionGoToDci = new Action(i18n.tr("Go to &DCI")) {
         @Override
         public void run()
         {
            IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
            if (selection.size() != 1)
               return;
            
            final long objectId = ((Alarm)selection.getFirstElement()).getSourceObjectId();
            final long dciId = ((Alarm)selection.getFirstElement()).getDciId();
            MainWindow.switchToObject(objectId, dciId);
         }
      };
      actionGoToDci.setId("AlarmList.GoToDci");

      actionExportToCsv = new ExportToCsvAction(view, alarmViewer, true);

		//time based sticky acknowledgement	
      timeAcknowledgeOther = new Action("Other...") {
         @Override
         public void run()
         {
            CustomTimePeriodDialog dlg = new CustomTimePeriodDialog(view.getWindow().getShell());
            if (dlg.open() == Window.OK)
            {
               int time = dlg.getTime();
               if (time > 0)
                  acknowledgeAlarms(true, time);
            }
         }
      };
      timeAcknowledgeOther.setId("AlarmList.TimeAcknowledgeOther"); 

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
      
      actionShowAlarmDetails = new Action(i18n.tr("Show &alarm details")) {
         @Override
         public void run()
         {
            IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
            if (selection.size() != 1)
               return;

            final long alarmId = ((Alarm)selection.getFirstElement()).getId();            
            view.openView(new AlarmDetails(alarmId, rootObject));
         }
      };
      actionShowAlarmDetails.setId("AlarmList.ShowAlarmDetails");

      actionQueryAssistant = new Action(i18n.tr("As&k AI assistant...")) {
         @Override
         public void run()
         {
            queryAssistant();
         }
      };
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
         settings.set("AlarmList.AckMenuSize", 4); 
         timeAcknowledge = new ArrayList<Action>(4);
         createDefaultIntervals();
         settings.set("AlarmList.AckMenuEntry.0", 1 * 60 * 60); 
         settings.set("AlarmList.AckMenuEntry.1", 4 * 60 * 60); 
         settings.set("AlarmList.AckMenuEntry.2", 24 * 60 * 60); 
         settings.set("AlarmList.AckMenuEntry.3", 2 * 24 * 60 * 60); 
         return;
	   }
	   timeAcknowledge = new ArrayList<Action>(menuSize);
      for(int i = 0; i < menuSize; i++)
      {
         final int time = settings.getAsInteger("AlarmList.AckMenuEntry." + Integer.toString(i), 0); 
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
      Action action = new Action(TimePeriodFunctions.timeToString(time)) {
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
    * Create context menu for alarm list
    */
   private void createContextMenu()
   {
      // Create menu manager for underlying object
      final MenuManager objectMenuManager = new ObjectContextMenuManager(view, new TransformationSelectionProvider(alarmViewer) {
         @Override
         protected ISelection transformSelection(ISelection selection)
         {
            List<AbstractObject> objects = new ArrayList<>(((IStructuredSelection)selection).size());
            for(Object element : ((IStructuredSelection)selection).toList())
            {
               if (element instanceof AlarmHandle)
               {
                  AbstractObject object = session.findObjectById(((AlarmHandle)element).alarm.getSourceObjectId());
                  if (object != null)
                     objects.add(object);
               }
            }
            return new StructuredSelection(objects);
         }
      }, alarmViewer) {
         @Override
         public String getMenuText()
         {
            return i18n.tr("&Object");
         }
      };

      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr, objectMenuManager);
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
   protected void fillContextMenu(IMenuManager manager, MenuManager objectMenuManager)
	{
      IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
      if (selection.isEmpty())
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
			manager.add(actionGoToObject);
			if (((Alarm)selection.getFirstElement()).getDciId() != 0)
            manager.add(actionGoToDci);
			manager.add(new Separator());
		}

      manager.add(actionCopy);
      manager.add(actionCopyMessage);
      manager.add(actionExportToCsv);

      if (selection.size() == 1)
      {
         manager.add(new Separator());
         manager.add(actionShowAlarmDetails);
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
         if (aiAssistantAvailable)
         {
            manager.add(actionQueryAssistant);
         }
         manager.add(new Separator());
         manager.add(objectMenuManager);
      }

      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
      final Menu toolsMenu = ObjectMenuFactory.createToolsMenu(selection, contextId, ((MenuManager)manager).getMenu(), null, new ViewPlacement(view));
      if (toolsMenu != null)
      {
         manager.add(new Separator());
         manager.add(new MenuContributionItem(i18n.tr("&Tools"), toolsMenu));
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
      rootObject = objectId;
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
               filterAndLimit(getDisplay());
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
    * configuration parameter <code>AlarmListDisplayLimit</code>, and update list control. This method should be called on
    * background thread with alarm list locked.
    *
    * @param display display for executing UI updates
    */
   private void filterAndLimit(Display display)
   {
      // filter
      final Map<Long, Alarm> selectedAlarms = new HashMap<Long, Alarm>();
      for(Alarm alarm : alarmList.values())
      {
         if (alarmFilter.filter(alarm))
         {
            selectedAlarms.put(alarm.getId(), alarm);
         }
      }

      // limit number of alarms to display
      final Map<Long, Alarm> filteredAlarms;
      if ((session.getAlarmListDisplayLimit() > 0) && (selectedAlarms.size() > session.getAlarmListDisplayLimit()))
      {
         // sort by last change - newest first
         List<Alarm> l = new ArrayList<Alarm>(selectedAlarms.values());
         Collections.sort(l, new Comparator<Alarm>() {
            @Override
            public int compare(Alarm alarm1, Alarm alarm2)
            {
               return -(alarm1.getLastChangeTime().compareTo(alarm2.getLastChangeTime()));
            }
         });

         filteredAlarms = new HashMap<Long, Alarm>(session.getAlarmListDisplayLimit());
         for(Alarm a : l.subList(0, session.getAlarmListDisplayLimit()))
            filteredAlarms.put(a.getId(), a);
      }
      else
      {
         filteredAlarms = selectedAlarms;
      }

      final List<Long> updatedAlarms = new ArrayList<Long>(updateList.size());
      updatedAlarms.addAll(updateList);
      updateList.clear();

      display.asyncExec(() -> {
         if (isDisposed() || alarmViewer.getControl().isDisposed())
            return;

         // Remove from display alarms that are no longer visible
         int initialSize = displayList.size();
         displayList.entrySet().removeIf(e -> (!filteredAlarms.containsKey(e.getKey())));
         boolean structuralChanges = (displayList.size() != initialSize);

         // Add or update alarms in display list
         for(Alarm a : filteredAlarms.values())
         {
            AlarmHandle h = displayList.get(a.getId());
            if (h != null)
            {
               h.alarm = a;
            }
            else
            {
               displayList.put(a.getId(), new AlarmHandle(a));
               structuralChanges = true;
            }
         }

         if (structuralChanges)
         {
            WidgetHelper.setRedraw(alarmViewer.getControl(), false);
            TreeItem topItem = alarmViewer.getTree().getTopItem();
            alarmViewer.refresh();
            if ((topItem != null) && !topItem.isDisposed())
               alarmViewer.getTree().setTopItem(topItem);
            WidgetHelper.setRedraw(alarmViewer.getControl(), true);
         }
         else
         {
            List<AlarmHandle> updatedElements = new ArrayList<AlarmHandle>(updatedAlarms.size());
            for(int i = 0; i < updatedAlarms.size(); i++)
            {
               AlarmHandle h = displayList.get(updatedAlarms.get(i));
               if (h != null)
                  updatedElements.add(h);
            }
            alarmViewer.update(updatedElements.toArray(), new String[] { "message" });
         }

         if ((session.getAlarmListDisplayLimit() > 0) && (selectedAlarms.size() >= session.getAlarmListDisplayLimit()))
         {
            deleteMessage(warningMessageId);
            warningMessageId = addMessage(MessageArea.INFORMATION, String.format(i18n.tr("Only %d most recent alarms shown"), filteredAlarms.size()), true);
         }
         else
         {
            clearMessages();
         }

         // Mark job end and check if another filter run is needed
         filterRunning = false;
         if (filterRunPending)
         {
            filterRunPending = false;
            refreshTimer.execute();
         }
      });

      synchronized(newAlarmList)
      {
         display.syncExec(() -> {
            if (!isDisposed() && (view != null) && view.isVisible() && isLocalNotificationsEnabled)
            {
               for(Alarm a : newAlarmList)
               {
                  if (filteredAlarms.containsKey(a.getId()))
                     AlarmNotifier.playSounOnAlarm(a, getDisplay());
               }
            }
         });
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

      new Job(i18n.tr("Synchronize alarm list"), view, null) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
			   HashMap<Long, Alarm> alarms = session.getAlarms();
            synchronized(alarmList)
            {
               alarmList.clear();
               alarmList.putAll(alarms);
               filterAndLimit(getDisplay());
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
      IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
      if (selection.isEmpty())
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
      IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
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
               runInUIThread(() -> {
                  if (!isDisposed())
                  {
                     AlarmStateChangeFailureDialog dlg = new AlarmStateChangeFailureDialog((view != null) ? view.getWindow().getShell() : null, resolveFails);
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
      IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
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
               runInUIThread(() -> {
                  if (!isDisposed())
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
      IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final long id = ((Alarm)selection.getFirstElement()).getId();
      new Job(i18n.tr("Create helpdesk ticket"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final String issueId = session.openHelpdeskIssue(id);
            runInUIThread(() -> {
               if (isDisposed())
                  return;

               String message = String.format(i18n.tr("Helpdesk issue created successfully (assigned ID is %s)"), issueId);
               if (view != null)
                  view.addMessage(MessageArea.SUCCESS, message);
               else
                  MessageDialogHelper.openInformation(getShell(), i18n.tr("Create Helpdesk Issue"), message);
            });
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
      IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final long id = ((Alarm)selection.getFirstElement()).getId();
      new Job(i18n.tr("Show helpdesk ticket"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final String url = session.getHelpdeskIssueUrl(id);
            runInUIThread(() -> {
               ExternalWebBrowser.open(url);
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
      IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
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
    * Send request to AI assistant for selected alarm
    */
   private void queryAssistant()
   {
      IStructuredSelection selection = alarmSelectionProvider.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final Alarm alarm = (Alarm)selection.getFirstElement();
      new Job(i18n.tr("Querying AI assistant"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final String comments = session.requestAiAssistantComment(alarm.getId());
            runInUIThread(() -> {
               if (isDisposed())
                  return;

               AssistantCommentDialog dlg = new AssistantCommentDialog((view != null) ? view.getWindow().getShell() : null, comments);
               if (dlg.open() == Window.OK)
               {
                  String text = dlg.getText().trim();
                  if (!text.isEmpty())
                     addComment(alarm.getId(), text);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot query AI assistant");
         }
      }.start();
   }

   private void addComment(final long alarmId, final String text)
   {
      new Job(i18n.tr("Adding alarm comment"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateAlarmComment(alarmId, 0, text);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot add alarm comment");
         }
      }.start();
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

   /**
    * Get filter for alarm viewer.
    *
    * @return filter for alarm viewer
    */
   public AbstractViewerFilter getFilter()
   {
      return alarmFilter;
   }

   /**
    * Get attribute proposals for search query autocompletion.
    *
    * @return attribute proposals for search query autocompletion
    */
   public SearchQueryAttribute[] getAttributeProposals()
   {
      return attributeProposals;
   }

   /**
    * Value provider for attribute "event"
    */
   private class EventAttributeValueProvider implements SearchQueryAttributeValueProvider
   {
      /**
       * @see org.netxms.ui.eclipse.widgets.helpers.SearchQueryAttributeValueProvider#getValues()
       */
      @Override
      public String[] getValues()
      {
         Set<Integer> eventCodes = new HashSet<>();
         for(AlarmHandle a : displayList.values())
            eventCodes.add(a.alarm.getSourceEventCode());
         List<EventTemplate> eventTemplates = session.findMultipleEventTemplates(eventCodes);
         if (eventTemplates.isEmpty())
            return null;
         String[] values = new String[eventTemplates.size()];
         for(int i = 0; i < values.length; i++)
            values[i] = eventTemplates.get(i).getName();
         return values;
      }
   }

   /**
    * Value provider for attribute "Source"
    */
   private class SourceAttributeValueProvider implements SearchQueryAttributeValueProvider
   {
      /**
       * @see org.netxms.ui.eclipse.widgets.helpers.SearchQueryAttributeValueProvider#getValues()
       */
      @Override
      public String[] getValues()
      {
         Set<Long> objectIdentifiers = new HashSet<Long>();
         for(AlarmHandle a : displayList.values())
            objectIdentifiers.add(a.alarm.getSourceObjectId());
         List<AbstractObject> objects = session.findMultipleObjects(objectIdentifiers, false);
         if (objects.isEmpty())
            return null;
         String[] values = new String[objects.size()];
         for(int i = 0; i < values.length; i++)
            values[i] = objects.get(i).getObjectName();
         return values;
      }
   }

   /**
    * Value provider for attribute "Zone"
    */
   private static class ZoneAttributeValueProvider implements SearchQueryAttributeValueProvider
   {
      /**
       * @see org.netxms.ui.eclipse.widgets.helpers.SearchQueryAttributeValueProvider#getValues()
       */
      @Override
      public String[] getValues()
      {
         NXCSession session = Registry.getSession();
         if (!session.isZoningEnabled())
            return null;
         List<Zone> zones = session.getAllZones();
         if (zones.isEmpty())
            return null;
         String[] values = new String[zones.size()];
         for(int i = 0; i < values.length; i++)
            values[i] = zones.get(i).getObjectName();
         return values;
      }
   }
}

