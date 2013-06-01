/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
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
import org.netxms.ui.eclipse.alarmviewer.views.AlarmComments;
import org.netxms.ui.eclipse.alarmviewer.views.AlarmDetails;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmComparator;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmListFilter;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmListLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.views.TabbedObjectView;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.IActionConstants;
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
	private Map<Long, Alarm> alarmList = new HashMap<Long, Alarm>();
	private Action actionComments;
	private Action actionAcknowledge;
	private Action actionResolve;
	private Action actionStickyAcknowledge;
	private Action actionTerminate;
	private Action actionShowAlarmDetails;
	private Action actionShowObjectDetails;
	private Action actionExportToCsv;
	
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
		
		/*
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
		getDisplay().timerExec(500, blinkTimer);
		*/
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
		actionComments = new Action(Messages.get().AlarmList_Comments, Activator.getImageDescriptor("icons/comments.png")) { //$NON-NLS-1$
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				openAlarmDetailsView(AlarmComments.ID);
			}
		};

		actionShowAlarmDetails = new Action(Messages.get().AlarmList_ActionAlarmDetails) {
			@Override
			public void run()
			{
				openAlarmDetailsView(AlarmDetails.ID);
			}
		};

		actionAcknowledge = new Action(Messages.get().AlarmList_Acknowledge, Activator.getImageDescriptor("icons/acknowledged.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				acknowledgeAlarms(false);
			}
		};

		actionStickyAcknowledge = new Action(Messages.get().AlarmList_StickyAck, Activator.getImageDescriptor("icons/acknowledged_sticky.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				acknowledgeAlarms(true);
			}
		};

		actionResolve = new Action(Messages.get().AlarmList_Resolve, Activator.getImageDescriptor("icons/resolved.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				resolveAlarms();
			}
		};

		actionTerminate = new Action(Messages.get().AlarmList_Terminate, Activator.getImageDescriptor("icons/terminated.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				terminateAlarms();
			}
		};
		
		actionShowObjectDetails = new Action(Messages.get().AlarmList_ActionObjectDetails) {
			@Override
			public void run()
			{
				showObjectDetails();
			}
		};
		
		actionExportToCsv = new ExportToCsvAction(viewPart, alarmViewer, true);
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
		
		manager.add(actionAcknowledge);
		manager.add(actionStickyAcknowledge);
		manager.add(actionResolve);
		manager.add(actionTerminate);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());

		if (selection.size() == 1)
		{
			manager.add(new GroupMarker(IActionConstants.MB_OBJECT_MANAGEMENT));
			manager.add(new Separator());
			manager.add(actionShowObjectDetails);
			manager.add(new Separator());
		}
		
		manager.add(actionExportToCsv);

		if (selection.size() == 1)
		{
			manager.add(new Separator());
			manager.add(actionShowAlarmDetails);
			manager.add(actionComments);
		}
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
				return Messages.get(getDisplay()).AlarmList_SyncJobError;
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
	 * Acknowledge selected alarms
	 * 
	 * @param sticky
	 */
	private void acknowledgeAlarms(final boolean sticky)
	{
		IStructuredSelection selection = (IStructuredSelection)alarmViewer.getSelection();
		if (selection.size() == 0)
			return;
		
		final Object[] alarms = selection.toArray();
		new ConsoleJob(Messages.get().AcknowledgeAlarm_JobName, viewPart, Activator.PLUGIN_ID, AlarmList.JOB_FAMILY) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				monitor.beginTask(Messages.get(getDisplay()).AcknowledgeAlarm_TaskName, alarms.length);
				for(Object o : alarms)
				{
					if (monitor.isCanceled())
						break;
					if (o instanceof Alarm)
						session.acknowledgeAlarm(((Alarm)o).getId(), sticky);
					monitor.worked(1);
				}
				monitor.done();
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get(getDisplay()).AcknowledgeAlarm_ErrorMessage;
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
				monitor.beginTask(Messages.get(getDisplay()).AlarmList_ResolveAlarm, alarms.length);
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
				return Messages.get(getDisplay()).AlarmList_CannotResoveAlarm;
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
				monitor.beginTask(Messages.get(getDisplay()).TerminateAlarm_TaskName, alarms.length);
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
				return Messages.get(getDisplay()).TerminateAlarm_ErrorMessage;
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
