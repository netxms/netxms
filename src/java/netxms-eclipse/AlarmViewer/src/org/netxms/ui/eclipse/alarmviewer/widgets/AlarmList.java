/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.AlarmComparator;
import org.netxms.ui.eclipse.alarmviewer.AlarmListFilter;
import org.netxms.ui.eclipse.alarmviewer.AlarmListLabelProvider;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.IActionConstants;
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
	public static final int COLUMN_CREATED = 5;
	public static final int COLUMN_LASTCHANGE = 6;
	
	private final IViewPart viewPart;
	private NXCSession session = null;
	private NXCListener clientListener = null;
	private SortableTableViewer alarmViewer;
	private AlarmListFilter alarmFilter;
	private Map<Long, Alarm> alarmList = new HashMap<Long, Alarm>();
	private Action actionCopy;
	private Action actionCopyMessage;
	
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
		final String[] names = { Messages.AlarmList_ColumnSeverity, Messages.AlarmList_ColumnState, Messages.AlarmList_ColumnSource, Messages.AlarmList_ColumnMessage, Messages.AlarmList_ColumnCount, Messages.AlarmList_ColumnCreated, Messages.AlarmList_ColumnLastChange };
		final int[] widths = { 100, 100, 150, 300, 70, 100, 100 };
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
		
		createActions();
		createPopupMenu();

		addListener(SWT.Resize, new Listener() {
			public void handleEvent(Event e)
			{
				alarmViewer.getControl().setBounds(AlarmList.this.getClientArea());
			}
		});

		// Request alarm list from server
		new ConsoleJob(Messages.AlarmList_SyncJobName, viewPart, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final HashMap<Long, Alarm> list = session.getAlarms(false);
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
				return Messages.AlarmList_SyncJobError;
			}
		}.start();
		
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
					alarmViewer.refresh();
			}
		});
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionCopy = new Action(Messages.AlarmList_CopyToClipboard) {
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

		actionCopyMessage = new Action(Messages.AlarmList_CopyMsgToClipboard) {
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
	}

	/**
	 * Create pop-up menu for alarm list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
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
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_OBJECT_MANAGEMENT));
		manager.add(new Separator());
		manager.add(actionCopy);
		manager.add(actionCopyMessage);
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
}
