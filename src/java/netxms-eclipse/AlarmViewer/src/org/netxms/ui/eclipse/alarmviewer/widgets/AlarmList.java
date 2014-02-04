/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer.widgets;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.util.IPropertyChangeListener;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseTrackAdapter;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.alarmviewer.dialogs.AcknowledgeCustomTimeDialog;
import org.netxms.ui.eclipse.alarmviewer.views.AlarmComments;
import org.netxms.ui.eclipse.alarmviewer.views.AlarmDetails;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmAcknowledgeTimeFunctions;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmComparator;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmListFilter;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmListLabelProvider;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmToolTip;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.views.TabbedObjectView;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.FilteringMenuManager;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Alarm list widget
 */
public class AlarmList extends Composite
{
	public static final String JOB_FAMILY = "AlarmViewJob"; //$NON-NLS-1$
	
	// Columns
	public static final int COLUMN_SEVERITY = 0;
	public static final int COLUMN_STATE = 1;
	public static final int COLUMN_SOURCE = 2;
	public static final int COLUMN_MESSAGE = 3;
	public static final int COLUMN_COUNT = 4;
	public static final int COLUMN_COMMENTS = 5;
	public static final int COLUMN_ACK_BY = 6;
	public static final int COLUMN_CREATED = 7;
	public static final int COLUMN_LASTCHANGE = 8;
	
	private final IViewPart viewPart;
	private NXCSession session = null;
	private NXCListener clientListener = null;
	private SortableTableViewer alarmViewer;
	private AlarmListFilter alarmFilter;
	private Point toolTipLocation;
	private Alarm toolTipObject;
	private Map<Long, Alarm> alarmList = new HashMap<Long, Alarm>();
	private Action actionCopy;
	private Action actionCopyMessage;
	private Action actionComments;
	private Action actionAcknowledge;
	private Action actionResolve;
	private Action actionStickyAcknowledge;
	private Action actionTerminate;
	private Action actionShowAlarmDetails;
	private Action actionShowObjectDetails;
	private Action actionExportToCsv;
	private MenuManager timeAcknowledgeMenu;
	private List<Action> timeAcknowledge;
	private Action timeAcknowledgeOther;
	
	/**
	 * Create alarm list widget
	 *  
	 * @param viewPart owning view part
	 * @param parent parent composite
	 * @param style widget style
	 * @param configPrefix prefix for saving/loading widget configuration
	 */
	public AlarmList(IViewPart viewPart, Composite parent, int style, final String configPrefix)
	{
		super(parent, style);
		session = (NXCSession)ConsoleSharedData.getSession();
		this.viewPart = viewPart;	
		
		// Setup table columns
		final String[] names = { Messages.get().AlarmList_ColumnSeverity, Messages.get().AlarmList_ColumnState, Messages.get().AlarmList_ColumnSource, Messages.get().AlarmList_ColumnMessage, Messages.get().AlarmList_ColumnCount, Messages.get().AlarmList_Comments, Messages.get().AlarmList_AckBy, Messages.get().AlarmList_ColumnCreated, Messages.get().AlarmList_ColumnLastChange };
		final int[] widths = { 100, 100, 150, 300, 70, 70, 100, 100, 100 };
		alarmViewer = new SortableTableViewer(this, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
		WidgetHelper.restoreTableViewerSettings(alarmViewer, Activator.getDefault().getDialogSettings(), configPrefix);
	
		alarmViewer.setLabelProvider(new AlarmListLabelProvider());
		alarmViewer.setContentProvider(new ArrayContentProvider());
		alarmViewer.setComparator(new AlarmComparator());
		alarmFilter = new AlarmListFilter();
		alarmViewer.addFilter(alarmFilter);
		alarmViewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(alarmViewer, Activator.getDefault().getDialogSettings(), configPrefix);
			}
		});
		alarmViewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				actionShowAlarmDetails.run();
			}
		});
		
		final Runnable toolTipTimer = new Runnable() {
         @Override
         public void run()
         {
            AlarmToolTip t = new AlarmToolTip(alarmViewer.getTable(), toolTipObject);
            t.show(toolTipLocation);
         }
      };
		
		alarmViewer.getTable().addMouseTrackListener(new MouseTrackAdapter() {
         @Override
         public void mouseHover(MouseEvent e)
         {
            if (!Activator.getDefault().getPreferenceStore().getBoolean("SHOW_ALARM_TOOLTIPS")) //$NON-NLS-1$
               return;
            
            Point p = new Point(e.x, e.y);
            TableItem item = alarmViewer.getTable().getItem(p);
            if (item == null)
               return;
            toolTipObject = (Alarm)item.getData();
            p.x -= 10;
            p.y -= 10;
            toolTipLocation = p;
            getDisplay().timerExec(300, toolTipTimer);
         }

         /* (non-Javadoc)
          * @see org.eclipse.swt.events.MouseTrackAdapter#mouseExit(org.eclipse.swt.events.MouseEvent)
          */
         @Override
         public void mouseExit(MouseEvent e)
         {
            getDisplay().timerExec(-1, toolTipTimer);
         }
		});
		
		createActions();
		createPopupMenu();

		addListener(SWT.Resize, new Listener() {
			public void handleEvent(Event e)
			{
				alarmViewer.getControl().setBounds(AlarmList.this.getClientArea());
			}
		});
		
		refresh();

		// Add client library listener
		clientListener = new NXCListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				switch(n.getCode())
				{
					case NXCNotification.NEW_ALARM:
					case NXCNotification.ALARM_CHANGED:
						synchronized(alarmList)
						{
							alarmList.put(((Alarm)n.getObject()).getId(), (Alarm)n.getObject());
						}
						scheduleAlarmViewerUpdate();
						break;
					case NXCNotification.ALARM_TERMINATED:
					case NXCNotification.ALARM_DELETED:
						synchronized(alarmList)
						{
							alarmList.remove(((Alarm)n.getObject()).getId());
						}
						scheduleAlarmViewerUpdate();
						break;
					default:
						break;
				}
			}
		};
		session.addListener(clientListener);
		
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				if ((session != null) && (clientListener != null))
					session.removeListener(clientListener);
			}
		});
		
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
		
		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		if (ps.getBoolean("BLINK_OUTSTANDING_ALARMS")) //$NON-NLS-1$
			getDisplay().timerExec(500, blinkTimer);
		ps.addPropertyChangeListener(new IPropertyChangeListener() {
			@Override
			public void propertyChange(PropertyChangeEvent event)
			{
				if (event.getProperty().equals("BLINK_OUTSTANDING_ALARMS")) //$NON-NLS-1$
				{
					if (ps.getBoolean("BLINK_OUTSTANDING_ALARMS")) //$NON-NLS-1$
						getDisplay().timerExec(500, blinkTimer);
					else
						getDisplay().timerExec(-1, blinkTimer);
				}
			}
		});
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
	 * Schedule alarm viewer update
	 */
	private void scheduleAlarmViewerUpdate()
	{
		getDisplay().asyncExec(new Runnable() {
			@Override
			public void run()
			{
				if (!alarmViewer.getControl().isDisposed())
				{
					synchronized(alarmList)
					{
						alarmViewer.refresh();
					}
				}
			}
		});
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionCopy = new Action(Messages.get().AlarmList_CopyToClipboard) {
			@Override
			public void run()
			{
				TableItem[] selection = alarmViewer.getTable().getSelection();
				if (selection.length > 0)
				{
					final String newLine = Platform.getOS().equals(Platform.OS_WIN32) ? "\r\n" : "\n"; //$NON-NLS-1$ //$NON-NLS-2$
					StringBuilder sb = new StringBuilder();
					for(int i = 0; i < selection.length; i++)
					{
						if (i > 0)
							sb.append(newLine);
						sb.append('[');
						sb.append(selection[i].getText(COLUMN_SEVERITY));
						sb.append("]\t"); //$NON-NLS-1$
						sb.append(selection[i].getText(COLUMN_SOURCE));
						sb.append('\t');
						sb.append(selection[i].getText(COLUMN_MESSAGE));
					}
					WidgetHelper.copyToClipboard(sb.toString());
				}
			}
		};
      actionCopy.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.Copy"); //$NON-NLS-1$

		actionCopyMessage = new Action(Messages.get().AlarmList_CopyMsgToClipboard) {
			@Override
			public void run()
			{
				TableItem[] selection = alarmViewer.getTable().getSelection();
				if (selection.length > 0)
				{
					final String newLine = Platform.getOS().equals(Platform.OS_WIN32) ? "\r\n" : "\n"; //$NON-NLS-1$ //$NON-NLS-2$
					StringBuilder sb = new StringBuilder();
					for(int i = 0; i < selection.length; i++)
					{
						if (i > 0)
							sb.append(newLine);
						sb.append(selection[i].getText(COLUMN_MESSAGE));
					}
					WidgetHelper.copyToClipboard(sb.toString());
				}
			}
		};
		actionCopyMessage.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.CopyMessage"); //$NON-NLS-1$
		
		actionComments = new Action(Messages.get().AlarmList_Comments, Activator.getImageDescriptor("icons/comments.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				openAlarmDetailsView(AlarmComments.ID);
			}
		};
		actionComments.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.Comments"); //$NON-NLS-1$

		actionShowAlarmDetails = new Action(Messages.get().AlarmList_ActionAlarmDetails) {
			@Override
			public void run()
			{
				openAlarmDetailsView(AlarmDetails.ID);
			}
		};
		actionShowAlarmDetails.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.AlarmDetails"); //$NON-NLS-1$

		actionAcknowledge = new Action(Messages.get().AlarmList_Acknowledge, Activator.getImageDescriptor("icons/acknowledged.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				acknowledgeAlarms(false, 0);
			}
		};
		actionAcknowledge.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.Acknowledge"); //$NON-NLS-1$

		actionStickyAcknowledge = new Action(Messages.get().AlarmList_StickyAck, Activator.getImageDescriptor("icons/acknowledged_sticky.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				acknowledgeAlarms(true, 0);
			}
		};
		actionStickyAcknowledge.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.StickyAcknowledge"); //$NON-NLS-1$

		actionResolve = new Action(Messages.get().AlarmList_Resolve, Activator.getImageDescriptor("icons/resolved.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				resolveAlarms();
			}
		};
		actionResolve.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.Resolve"); //$NON-NLS-1$

		actionTerminate = new Action(Messages.get().AlarmList_Terminate, Activator.getImageDescriptor("icons/terminated.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				terminateAlarms();
			}
		};
		actionTerminate.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.Terminate"); //$NON-NLS-1$
		
		actionShowObjectDetails = new Action(Messages.get().AlarmList_ActionObjectDetails) {
			@Override
			public void run()
			{
				showObjectDetails();
			}
		};
		actionShowObjectDetails.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.ShowObjectDetails"); //$NON-NLS-1$
		
		actionExportToCsv = new ExportToCsvAction(viewPart, alarmViewer, true);
		
		//time based sticky acknowledgment 	
		timeAcknowledgeOther = new Action("Other...", Activator.getImageDescriptor("icons/acknowledged.png")) { //$NON-NLS-1$ //$NON-NLS-2$
         @Override
         public void run()
         {
            AcknowledgeCustomTimeDialog dlg = new AcknowledgeCustomTimeDialog(viewPart.getSite().getShell());//PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell()
            if (dlg.open() == Window.OK)
            {
               int time = dlg.getTime();
               if (time > 0)
                  acknowledgeAlarms(true, time);
            }
         }
      };
      timeAcknowledgeOther.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.TimeAcknowledgeOther");  //$NON-NLS-1$
	}

	private void initializeTimeAcknowledge()
   {
	   IDialogSettings settings = Activator.getDefault().getDialogSettings();
	   int menuSize;
	   try
	   {
	      menuSize = settings.getInt("AlarmList.ackMenuSize");//$NON-NLS-1$
	   }
	   catch(NumberFormatException e)
	   {
	      settings.put("AlarmList.ackMenuSize", 4); //$NON-NLS-1$
	      timeAcknowledge = new ArrayList<Action>(4);
         createDefaultCations();
         settings.put("AlarmList.ackMenuEntry0", 1 * 60 * 60); //$NON-NLS-1$
         settings.put("AlarmList.ackMenuEntry1", 4 * 60 * 60); //$NON-NLS-1$
         settings.put("AlarmList.ackMenuEntry2", 24 * 60 * 60); //$NON-NLS-1$
         settings.put("AlarmList.ackMenuEntry3", 2 * 24 * 60 * 60); //$NON-NLS-1$
         return;
	   }
	   timeAcknowledge = new ArrayList<Action>(menuSize);
	   for (int i = 0; i<menuSize ; i++){
	      final int time = settings.getInt("AlarmList.ackMenuEntry"+Integer.toString(i)); //$NON-NLS-1$
	      if (time == 0)
	         continue;
	      String title = AlarmAcknowledgeTimeFunctions.timeToString(time);
	      Action action = new Action(title, Activator.getImageDescriptor("icons/acknowledged.png")) { //$NON-NLS-1$
	         @Override
	         public void run()
	         {
	            acknowledgeAlarms(true, time);
	         }
	      };
	      action.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.TimeAcknowledge"+Integer.toString(i)+"ID"); //$NON-NLS-1$ //$NON-NLS-2$
	      timeAcknowledge.add(action);
	   }
   }

   private void createDefaultCations()
   {
      Action act;
      act = new Action("1 hour(s)", Activator.getImageDescriptor("icons/acknowledged.png")) { //$NON-NLS-1$ //$NON-NLS-2$
         @Override
         public void run()
         {
            int time = 1*60*60; //hour to minutes, seconds
            acknowledgeAlarms(true, time);
         }
      };
      act.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.TimeAcknowledge0ID"); //$NON-NLS-1$
      timeAcknowledge.add(act);
      
      act = new Action("4 hour(s)", Activator.getImageDescriptor("icons/acknowledged.png")) { //$NON-NLS-1$ //$NON-NLS-2$
         @Override
         public void run()
         {
            int time = 4*60*60; //hour to minutes, seconds
            acknowledgeAlarms(true, time); 
         }
      };
      act.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.TimeAcknowledge1ID"); //$NON-NLS-1$
      timeAcknowledge.add(act);
      
      act = new Action("1 day(s)", Activator.getImageDescriptor("icons/acknowledged.png")) { //$NON-NLS-1$ //$NON-NLS-2$
         @Override
         public void run()
         {
            int time = 24*60*60; //day to hours, minutes, seconds
            acknowledgeAlarms(true, time);
         }
      };
      act.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.TimeAcknowledge2ID"); //$NON-NLS-1$
      timeAcknowledge.add(act);
      
      act = new Action("2 days(s)", Activator.getImageDescriptor("icons/acknowledged.png")) { //$NON-NLS-1$ //$NON-NLS-2$
         @Override
         public void run()
         {
            int time = 2*24*60*60; //day to hours, minutes, seconds
            acknowledgeAlarms(true, time);
         }
      };
      act.setId("org.netxms.ui.eclipse.alarmviewer.popupActions.TimeAcknowledge3ID"); //$NON-NLS-1$
      timeAcknowledge.add(act);
   }

   /**
	 * Create pop-up menu for alarm list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new FilteringMenuManager(Activator.PLUGIN_ID);
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

		// Register menu for extension.
		if (viewPart != null)
			viewPart.getSite().registerContextMenu(menuMgr, alarmViewer);
	}
	
	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
		if (selection.size() == 0)
			return;
		
		int states = getSelectionType(selection.toArray());
		
		if(states == 2)
		{
   		manager.add(actionAcknowledge);
   		manager.add(actionStickyAcknowledge);
   		
   		initializeTimeAcknowledge();
         timeAcknowledgeMenu = new MenuManager(Messages.get().AlarmList_StickyAckMenutTitle, "timeAcknowledge");   //$NON-NLS-2$ //$NON-NLS-1$
         for(Action act : timeAcknowledge)
         {
            timeAcknowledgeMenu.add(act);
         }
         timeAcknowledgeMenu.add(new Separator());   
         timeAcknowledgeMenu.add(timeAcknowledgeOther);
   		manager.add(timeAcknowledgeMenu);
		}
		
		if(states < 4)
		   manager.add(actionResolve);
		if (states == 4 || (session.getAlarmStatusFlowStrict() == 0))
		   manager.add(actionTerminate);
		
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());

		if (selection.size() == 1)
		{
			manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_MANAGEMENT));
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
			manager.add(actionShowAlarmDetails);
			manager.add(actionComments);
		}
	}

	private int getSelectionType(Object[] array)
   {
      int type = 0;
      for(int i = 0; i < array.length; i++)
      {
         type |= ((Alarm)array[i]).getState()+2;
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
		synchronized(alarmList)
		{
			alarmViewer.refresh();
		}
	}

	/**
	 * Refresh alarm list
	 */
	public void refresh()
	{
		new ConsoleJob(Messages.get().AlarmList_SyncJobName, viewPart, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final HashMap<Long, Alarm> list = session.getAlarms();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (!alarmViewer.getControl().isDisposed())
						{
							synchronized(alarmList)
							{
								alarmList.clear();
								alarmList.putAll(list);
								alarmViewer.setInput(alarmList.values());
							}
						}
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().AlarmList_SyncJobError;
			}
		}.start();
	}
	
	/**
	 * Open comments for selected alarm
	 */
	private void openAlarmDetailsView(String viewId)
	{
		IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
		if (selection.size() != 1)
			return;
		
		final String secondaryId = Long.toString(((Alarm)selection.getFirstElement()).getId());
		IWorkbenchPage page = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage();
		try
		{
			page.showView(viewId, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
		}
		catch(PartInitException e)
		{
			MessageDialogHelper.openError(getShell(), Messages.get().AlarmList_Error, Messages.get().AlarmList_ErrorText + e.getLocalizedMessage());
		}
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
		new ConsoleJob(Messages.get().AcknowledgeAlarm_JobName, viewPart, Activator.PLUGIN_ID, AlarmList.JOB_FAMILY) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				monitor.beginTask(Messages.get().AcknowledgeAlarm_TaskName, alarms.length);
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
				return Messages.get().AcknowledgeAlarm_ErrorMessage;
			}
		}.start();
	}
	
	
	/**
	 * Resolve selected alarms
	 */
	private void resolveAlarms()
	{
		IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
		if (selection.size() == 0)
			return;
		
		final Object[] alarms = selection.toArray();
		new ConsoleJob(Messages.get().AlarmList_Resolving, viewPart, Activator.PLUGIN_ID, AlarmList.JOB_FAMILY) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				monitor.beginTask(Messages.get().AlarmList_ResolveAlarm, alarms.length);
				for(Object o : alarms)
				{
					if (monitor.isCanceled())
						break;
					if (o instanceof Alarm)
						session.resolveAlarm(((Alarm)o).getId());
					monitor.worked(1);
				}
				monitor.done();
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().AlarmList_CannotResoveAlarm;
			}
		}.start();
	}

	/**
	 * Terminate selected alarms
	 */
	private void terminateAlarms()
	{
		IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
		if (selection.size() == 0)
			return;
		
		final Object[] alarms = selection.toArray();
		new ConsoleJob(Messages.get().TerminateAlarm_JobTitle, viewPart, Activator.PLUGIN_ID, AlarmList.JOB_FAMILY) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				monitor.beginTask(Messages.get().TerminateAlarm_TaskName, alarms.length);
				for(Object o : alarms)
				{
					if (monitor.isCanceled())
						break;
					if (o instanceof Alarm)
						session.terminateAlarm(((Alarm)o).getId());
					monitor.worked(1);
				}
				monitor.done();
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().TerminateAlarm_ErrorMessage;
			}
		}.start();
	}

	/**
	 * Show details for selected object
	 */
	private void showObjectDetails()
	{
		IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
		if (selection.size() != 1)
			return;
		
		AbstractObject object = session.findObjectById(((Alarm)selection.getFirstElement()).getSourceObjectId());
		if (object != null)
		{
			try
			{
				TabbedObjectView view = (TabbedObjectView)PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().showView(TabbedObjectView.ID);
				view.setObject(object);
			}
			catch(PartInitException e)
			{
				MessageDialogHelper.openError(getShell(), Messages.get().AlarmList_Error, Messages.get().AlarmList_OpenDetailsError + e.getLocalizedMessage());
			}
		}
	}
	
	/**
	 * Get underlying table viewer.
	 * 
	 * @return
	 */
	public TableViewer getViewer()
	{
		return alarmViewer;
	}
}
