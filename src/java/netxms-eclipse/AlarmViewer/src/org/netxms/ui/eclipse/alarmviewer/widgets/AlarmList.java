/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.AlarmComparator;
import org.netxms.ui.eclipse.alarmviewer.AlarmListFilter;
import org.netxms.ui.eclipse.alarmviewer.AlarmListLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.SortableTableViewer;

/**
 * @author victor
 *
 */
public class AlarmList extends Composite
{
	public static final String JOB_FAMILY = "AlarmViewJob";
	
	// Columns
	public static final int COLUMN_SEVERITY = 0;
	public static final int COLUMN_STATE = 1;
	public static final int COLUMN_SOURCE = 2;
	public static final int COLUMN_MESSAGE = 3;
	public static final int COLUMN_COUNT = 4;
	public static final int COLUMN_CREATED = 5;
	public static final int COLUMN_LASTCHANGE = 6;
	
	private final ViewPart viewPart;
	private NXCSession session = null;
	private NXCListener clientListener = null;
	private TableViewer alarmViewer;
	private AlarmListFilter alarmFilter;
	private HashMap<Long, Alarm> alarmList;
	
	public AlarmList(ViewPart viewPart, Composite parent, int style)
	{
		super(parent, style);
		session = NXMCSharedData.getInstance().getSession();
		this.viewPart = viewPart;		
		
		// Setup table columns
		final String[] names = { "Severity", "State", "Source", "Message", "Count", "Created", "Last Change" };
		final int[] widths = { 100, 100, 150, 300, 70, 100, 100 };
		alarmViewer = new SortableTableViewer(this, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
	
		alarmViewer.setLabelProvider(new AlarmListLabelProvider());
		alarmViewer.setContentProvider(new ArrayContentProvider());
		alarmViewer.setComparator(new AlarmComparator());
		alarmFilter = new AlarmListFilter();
		alarmViewer.addFilter(alarmFilter);
		
		createPopupMenu();

		addListener(SWT.Resize, new Listener() {
			public void handleEvent(Event e)
			{
				alarmViewer.getControl().setBounds(AlarmList.this.getClientArea());
			}
		});

		// Request alarm list from server
		new ConsoleJob("Synchronize alarm list", viewPart, Activator.PLUGIN_ID, JOB_FAMILY)
		{
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				alarmList = session.getAlarms(false);

				new UIJob("Initialize alarm viewer") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						synchronized(alarmList)
						{
							alarmViewer.setInput(alarmList.values());
						}
						return Status.OK_STATUS;
					}
				}.schedule();
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot synchronize alarm list";
			}
		}.start();
		
		// Add client library listener
		clientListener = new NXCListener() {
			@Override
			public void notificationHandler(NXCNotification n)
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
	}
		
	/**
	 * Schedule alarm viewer update
	 */
	private void scheduleAlarmViewerUpdate()
	{
		new UIJob("Update alarm list") {
			@Override
			public IStatus runInUIThread(IProgressMonitor monitor)
			{
				synchronized(alarmList)
				{
					alarmViewer.refresh();
				}
				return Status.OK_STATUS;
			}
		}.schedule();
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
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
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

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Widget#dispose()
	 */
	@Override
	public void dispose()
	{
		if ((session != null) && (clientListener != null))
			session.removeListener(clientListener);
		super.dispose();
	}
}
