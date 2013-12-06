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
package org.netxms.ui.eclipse.datacollection.widgets;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.MobileDevice;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.api.DciOpenHandler;
import org.netxms.ui.eclipse.datacollection.widgets.internal.LastValuesComparator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.LastValuesFilter;
import org.netxms.ui.eclipse.datacollection.widgets.internal.LastValuesLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Viewer for last values of given object
 */
public class LastValuesWidget extends Composite
{
	public static final String JOB_FAMILY = "LastValuesViewJob"; //$NON-NLS-1$
	
	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_DESCRIPTION = 1;
	public static final int COLUMN_VALUE = 2;
	public static final int COLUMN_TIMESTAMP = 3;
	public static final int COLUMN_THRESHOLD = 4;
	
	private final ViewPart viewPart;
	private AbstractObject dcTarget;
	private NXCSession session;
	private boolean filterEnabled = false;
	private FilterText filterText;
	private SortableTableViewer dataViewer;
	private LastValuesLabelProvider labelProvider;
	private LastValuesComparator comparator;
	private LastValuesFilter filter;
	private boolean autoRefreshEnabled = false;
	private int autoRefreshInterval = 30000;	// in milliseconds
	private Runnable refreshTimer;
	private Action actionUseMultipliers;
	private Action actionShowErrors;
	private Action actionShowDisabled;
	private Action actionShowUnsupported;
	private Action actionExportToCsv;
	private List<OpenHandlerData> openHandlers = new ArrayList<OpenHandlerData>(0);
	
	/**
	 * Create "last values" widget
	 * 
	 * @param viewPart owning view part
	 * @param parent parent widget
	 * @param style style
	 * @param _node node to display data for
	 * @param configPrefix configuration prefix for saving/restoring viewer settings
	 */
	public LastValuesWidget(ViewPart viewPart, Composite parent, int style, AbstractObject dcTarget, final String configPrefix)
	{
		super(parent, style);
		session = (NXCSession)ConsoleSharedData.getSession();
		this.viewPart = viewPart;		
		setDataCollectionTarget(dcTarget);
		
		registerOpenHandlers();
		
		final IDialogSettings ds = Activator.getDefault().getDialogSettings();
		
		refreshTimer = new Runnable() {
			@Override
			public void run()
			{
				if (isDisposed())
					return;
				
				getDataFromServer();
				getDisplay().timerExec(autoRefreshInterval, this);
			}
		};
		
		setLayout(new FormLayout());
		
		// Create filter area
		filterText = new FilterText(this, SWT.NONE);
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
			}
		});
		
		// Setup table columns
		final String[] names = { Messages.get().LastValuesWidget_ColID, Messages.get().LastValuesWidget_ColDescr, Messages.get().LastValuesWidget_ColValue, Messages.get().LastValuesWidget_ColTime, Messages.get().LastValuesWidget_ColThreshold };
		final int[] widths = { 70, 250, 150, 120, 150 };
		dataViewer = new SortableTableViewer(this, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
	
		labelProvider = new LastValuesLabelProvider();
		comparator = new LastValuesComparator();
		filter = new LastValuesFilter();
		dataViewer.setLabelProvider(labelProvider);
		dataViewer.setContentProvider(new ArrayContentProvider());
		dataViewer.setComparator(comparator);
		dataViewer.addFilter(filter);
		WidgetHelper.restoreTableViewerSettings(dataViewer, ds, configPrefix);
		
		dataViewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)dataViewer.getSelection();
				if (selection.size() == 1)
				{
					callOpenObjectHandler((DciValue)selection.getFirstElement());
				}
			}
		});
		
		dataViewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(dataViewer, ds, configPrefix);
				ds.put(configPrefix + ".autoRefresh", autoRefreshEnabled); //$NON-NLS-1$
				ds.put(configPrefix + ".autoRefreshInterval", autoRefreshEnabled); //$NON-NLS-1$
				ds.put(configPrefix + ".useMultipliers", labelProvider.areMultipliersUsed()); //$NON-NLS-1$
				ds.put(configPrefix + ".showErrors", isShowErrors()); //$NON-NLS-1$
				ds.put(configPrefix + ".showDisabled", isShowDisabled()); //$NON-NLS-1$
				ds.put(configPrefix + ".showUnsupported", isShowUnsupported()); //$NON-NLS-1$
			}
		});

		// Setup layout
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(filterText);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		dataViewer.getTable().setLayoutData(fd);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		filterText.setLayoutData(fd);
		
		try
		{
			ds.getInt(configPrefix + ".autoRefreshInterval"); //$NON-NLS-1$
		}
		catch(NumberFormatException e)
		{
		}
		setAutoRefreshEnabled(ds.getBoolean(configPrefix + ".autoRefresh")); //$NON-NLS-1$
		if (ds.get(configPrefix + ".useMultipliers") != null) //$NON-NLS-1$
			labelProvider.setUseMultipliers(ds.getBoolean(configPrefix + ".useMultipliers")); //$NON-NLS-1$
		else
			labelProvider.setUseMultipliers(true);
		if (ds.get(configPrefix + ".showErrors") != null) //$NON-NLS-1$
		{
			labelProvider.setShowErrors(ds.getBoolean(configPrefix + ".showErrors")); //$NON-NLS-1$
			comparator.setShowErrors(ds.getBoolean(configPrefix + ".showErrors")); //$NON-NLS-1$
		}
		else
		{
			labelProvider.setShowErrors(true);
			comparator.setShowErrors(true);
		}
		filter.setShowDisabled(ds.getBoolean(configPrefix + ".showDisabled")); //$NON-NLS-1$
		filter.setShowUnsupported(ds.getBoolean(configPrefix + ".showUnsupported")); //$NON-NLS-1$
		
		createActions();
		createPopupMenu();

		getDataFromServer();
		
		// Set initial focus to filter input line
		if (filterEnabled)
			filterText.setFocus();
		else
			enableFilter(false);	// Will hide filter area correctly
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionUseMultipliers = new Action(Messages.get().LastValuesWidget_UseMultipliers, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setUseMultipliers(actionUseMultipliers.isChecked());
			}
		};
		actionUseMultipliers.setChecked(areMultipliersUsed());

		actionShowErrors = new Action(Messages.get().LastValuesWidget_ShowErrors, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setShowErrors(actionShowErrors.isChecked());
			}
		};
		actionShowErrors.setChecked(isShowErrors());

		actionShowUnsupported = new Action(Messages.get().LastValuesWidget_ShowUnsupported, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setShowUnsupported(actionShowUnsupported.isChecked());
			}
		};
		actionShowUnsupported.setChecked(isShowUnsupported());
		
		actionShowDisabled = new Action(Messages.get().LastValuesWidget_ShowDisabled, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setShowDisabled(actionShowDisabled.isChecked());
			}
		};
		actionShowDisabled.setChecked(isShowDisabled());
		
		actionExportToCsv = new ExportToCsvAction(viewPart, dataViewer, true);
	}
	
	/**
	 * Create pop-up menu
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
		Menu menu = menuMgr.createContextMenu(dataViewer.getControl());
		dataViewer.getControl().setMenu(menu);

		// Register menu for extension.
		if (viewPart != null)
			viewPart.getSite().registerContextMenu(menuMgr, dataViewer);
	}
	
	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_SECONDARY));
		manager.add(new Separator());
		manager.add(actionUseMultipliers);
		manager.add(actionShowErrors);
		manager.add(actionShowDisabled);
		manager.add(actionShowUnsupported);
	}
	
	/**
	 * Get data from server
	 */
	private void getDataFromServer()
	{
		if (dcTarget == null)
		{
			dataViewer.setInput(new DciValue[0]);
			return;
		}

		ConsoleJob job = new ConsoleJob(Messages.get().LastValuesWidget_JobTitle + dcTarget.getObjectName(), viewPart, Activator.PLUGIN_ID, LastValuesWidget.JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().LastValuesWidget_JobError + dcTarget.getObjectName();
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final DciValue[] data = session.getLastValues(dcTarget.getObjectId());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (!isDisposed())
							dataViewer.setInput(data);
					}
				});
			}
		};
		job.setUser(false);
		job.start();
	}
	
	/**
	 * Change data collection target object
	 * 
	 * @param dcTarget new data collection target object
	 */
	public void setDataCollectionTarget(AbstractObject dcTarget)
	{
		if ((dcTarget instanceof AbstractNode) || (dcTarget instanceof MobileDevice) || (dcTarget instanceof Cluster))
			this.dcTarget = dcTarget;
		else
			this.dcTarget = null;
	}
	
	/**
	 * Refresh view
	 */
	public void refresh()
	{
		getDataFromServer();
	}

	/**
	 * @return the autoRefreshEnabled
	 */
	public boolean isAutoRefreshEnabled()
	{
		return autoRefreshEnabled;
	}

	/**
	 * @param autoRefreshEnabled the autoRefreshEnabled to set
	 */
	public void setAutoRefreshEnabled(boolean autoRefreshEnabled)
	{
		this.autoRefreshEnabled = autoRefreshEnabled;
		getDisplay().timerExec(-1, refreshTimer);
		if (autoRefreshEnabled)
			getDisplay().timerExec(autoRefreshInterval, refreshTimer);
	}

	/**
	 * @return the autoRefreshInterval
	 */
	public int getAutoRefreshInterval()
	{
		return autoRefreshInterval;
	}

	/**
	 * @param autoRefreshInterval the autoRefreshInterval to set
	 */
	public void setAutoRefreshInterval(int autoRefreshInterval)
	{
		this.autoRefreshInterval = autoRefreshInterval;
	}
	
	/**
	 * @return the useMultipliers
	 */
	public boolean areMultipliersUsed()
	{
		return (labelProvider != null) ? labelProvider.areMultipliersUsed() : false;
	}

	/**
	 * @param useMultipliers the useMultipliers to set
	 */
	public void setUseMultipliers(boolean useMultipliers)
	{
		if (labelProvider != null)
		{
			labelProvider.setUseMultipliers(useMultipliers);
			if (dataViewer != null)
			{
				dataViewer.refresh(true);
			}
		}
	}

	/**
	 * Enable or disable filter
	 * 
	 * @param enable New filter state
	 */
	public void enableFilter(boolean enable)
	{
		filterEnabled = enable;
		filterText.setVisible(filterEnabled);
		FormData fd = (FormData)dataViewer.getTable().getLayoutData();
		fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
		layout();
		if (enable)
			filterText.setFocus();
		else
			setFilter(""); //$NON-NLS-1$
	}

	/**
	 * @return the filterEnabled
	 */
	public boolean isFilterEnabled()
	{
		return filterEnabled;
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
	 * Handler for filter modification
	 */
	private void onFilterModify()
	{
		final String text = filterText.getText();
		filter.setFilterString(text);
		dataViewer.refresh(false);
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
	 * @return
	 */
	public boolean isShowErrors()
	{
		return (labelProvider != null) ? labelProvider.isShowErrors() : false;
	}

	/**
	 * @param show
	 */
	public void setShowErrors(boolean show)
	{
		labelProvider.setShowErrors(show);
		comparator.setShowErrors(show);
		if (dataViewer != null)
		{
			dataViewer.refresh(true);
		}
	}
	
	/**
	 * @return
	 */
	public boolean isShowDisabled()
	{
		return (filter != null) ? filter.isShowDisabled() : false;
	}

	/**
	 * @param show
	 */
	public void setShowDisabled(boolean show)
	{
		filter.setShowDisabled(show);
		if (dataViewer != null)
		{
			dataViewer.refresh(true);
		}
	}

	/**
	 * @return
	 */
	public boolean isShowUnsupported()
	{
		return (filter != null) ? filter.isShowUnsupported() : false;
	}

	/**
	 * @param show
	 */
	public void setShowUnsupported(boolean show)
	{
		filter.setShowUnsupported(show);
		if (dataViewer != null)
		{
			dataViewer.refresh(true);
		}
	}

	/**
	 * Register object open handlers
	 */
	private void registerOpenHandlers()
	{
		// Read all registered extensions and create tabs
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.datacollection.dciOpenHandlers"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final OpenHandlerData h = new OpenHandlerData();
				h.handler = (DciOpenHandler)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
				h.priority = safeParseInt(elements[i].getAttribute("priority")); //$NON-NLS-1$
				openHandlers.add(h);
			}
			catch(CoreException e)
			{
				e.printStackTrace();
			}
		}
		
		// Sort tabs by appearance order
		Collections.sort(openHandlers, new Comparator<OpenHandlerData>() {
			@Override
			public int compare(OpenHandlerData arg0, OpenHandlerData arg1)
			{
				return arg0.priority - arg1.priority;
			}
		});
	}
	
	/**
	 * @param s
	 * @return
	 */
	private static int safeParseInt(String s)
	{
		if (s == null)
			return 65535;
		try
		{
			return Integer.parseInt(s);
		}
		catch(NumberFormatException e)
		{
			return 65535;
		}
	}
	
	/**
	 * Call object open handler
	 * 
	 * @return
	 */
	private boolean callOpenObjectHandler(DciValue object)
	{
		for(OpenHandlerData h : openHandlers)
		{
			if (h.handler.open(object))
				return true;
		}
		return false;
	}

	/**
	 * Internal data for object open handlers
	 */
	private class OpenHandlerData
	{
		DciOpenHandler handler;
		int priority;
	}
}
