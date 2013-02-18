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
package org.netxms.ui.eclipse.dashboard.dialogs;

import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.dialogs.helpers.DciIdMatchingData;
import org.netxms.ui.eclipse.dashboard.dialogs.helpers.IdMatchingContentProvider;
import org.netxms.ui.eclipse.dashboard.dialogs.helpers.IdMatchingLabelProvider;
import org.netxms.ui.eclipse.dashboard.dialogs.helpers.ObjectIdMatchingData;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectNodeDciDialog;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Dialog for node and DCI ID matching
 */
public class IdMatchingDialog extends Dialog
{
	public static final int COLUMN_SOURCE_ID = 0;
	public static final int COLUMN_SOURCE_NAME = 1;
	public static final int COLUMN_DESTINATION_ID = 2;
	public static final int COLUMN_DESTINATION_NAME = 3;
	
	private SortableTreeViewer viewer;
	private Map<Long, ObjectIdMatchingData> objects;
	private Map<Long, DciIdMatchingData> dcis;
	private Action actionMap;
	
	/**
	 * @param parentShell
	 * @param objects
	 * @param dcis
	 */
	public IdMatchingDialog(Shell parentShell, Map<Long, ObjectIdMatchingData> objects, Map<Long, DciIdMatchingData> dcis)
	{
		super(parentShell);
		this.objects = objects;
		this.dcis = dcis;
		setShellStyle(getShellStyle() | SWT.RESIZE);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.IdMatchingDialog_Title);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		dialogArea.setLayout(layout);
		
		Label label = new Label(dialogArea, SWT.WRAP);
		label.setText(Messages.IdMatchingDialog_HelpText);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 0;
		label.setLayoutData(gd);
		
		final String[] names = { Messages.IdMatchingDialog_ColumnOriginalID, Messages.IdMatchingDialog_ColumnName, Messages.IdMatchingDialog_ColumnMatchID, Messages.IdMatchingDialog_ColumnMatchName };
		final int[] widths = { 100, 300, 80, 300 };
		viewer = new SortableTreeViewer(dialogArea, names, widths, 0, SWT.UP, SWT.BORDER | SWT.SINGLE | SWT.FULL_SELECTION);
		viewer.getTree().setLinesVisible(true);
		viewer.getTree().setHeaderVisible(true);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 400;
		viewer.getControl().setLayoutData(gd);
		viewer.setContentProvider(new IdMatchingContentProvider());
		viewer.setLabelProvider(new IdMatchingLabelProvider());
		viewer.setInput(new Object[] { objects, dcis });
		viewer.expandAll();

		createActions();
		createPopupMenu();
		
		return dialogArea;
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionMap = new Action(Messages.IdMatchingDialog_MapTo, Activator.getImageDescriptor("icons/sync.gif")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				if (selection.size() != 1)
					return;
				
				Object element = selection.getFirstElement();
				if (element instanceof ObjectIdMatchingData)
					mapNode((ObjectIdMatchingData)element);
				if (element instanceof DciIdMatchingData)
					mapDci((DciIdMatchingData)element);
			}
		};
	}

	/**
	 * Create context menu
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
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionMap);
	}
	
	/**
	 * Map node
	 * 
	 * @param data
	 */
	private void mapNode(ObjectIdMatchingData data)
	{
		Set<Integer> classFilter;
		switch(data.objectClass)
		{
			case GenericObject.OBJECT_NODE:
				classFilter = ObjectSelectionDialog.createNodeSelectionFilter(false);
				break;
			case GenericObject.OBJECT_CONTAINER:
				classFilter = ObjectSelectionDialog.createContainerSelectionFilter();
				break;
			case GenericObject.OBJECT_ZONE:
				classFilter = ObjectSelectionDialog.createZoneSelectionFilter();
				break;
			case GenericObject.OBJECT_DASHBOARD:
				classFilter = new HashSet<Integer>(2);
				classFilter.add(GenericObject.OBJECT_DASHBOARD);
				classFilter.add(GenericObject.OBJECT_DASHBOARDROOT);
				break;
			default:
				classFilter = null;
				break;
		}
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), null, classFilter);
		if (dlg.open() == Window.OK)
		{
			GenericObject object = dlg.getSelectedObjects().get(0);
			if (object.getObjectClass() == data.objectClass)
			{
				data.dstId = object.getObjectId();
				data.dstName = object.getObjectName();
				
				updateDciMapping(data);
				viewer.update(data, null);
			}
			else
			{
				MessageDialogHelper.openWarning(getShell(), Messages.IdMatchingDialog_Warning, Messages.IdMatchingDialog_ClassMismatch);
			}
		}
	}
	
	/**
	 * Update mapping for all DCIs of given node after node mapping change
	 * 
	 * @param objData
	 */
	private void updateDciMapping(final ObjectIdMatchingData objData)
	{
		if (objData.dcis.size() == 0)
			return;
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.IdMatchingDialog_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final DciValue[] dciValues = session.getLastValues(objData.dstId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						for(DciIdMatchingData d : objData.dcis)
						{
							d.dstNodeId = objData.dstId;
							d.dstDciId = 0;
							d.dstName = null;
							for(DciValue v : dciValues)
							{
								if (v.getDescription().equalsIgnoreCase(d.srcName))
								{
									d.dstDciId = v.getId();
									d.dstName = v.getDescription();
									break;
								}
							}
						}
						viewer.refresh(true);
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.IdMatchingDialog_JobErrorText;
			}
		}.start();
	}
	
	/**
	 * Map DCI
	 * 
	 * @param data
	 */
	private void mapDci(DciIdMatchingData data)
	{
		if (data.dstNodeId == 0)
			return;
		
		SelectNodeDciDialog dlg = new SelectNodeDciDialog(getShell(), data.dstNodeId);
		if (dlg.open() == Window.OK)
		{
			DciValue v = dlg.getSelection();
			if (v != null)
			{
				data.dstDciId = v.getId();
				data.dstName = v.getDescription();
				viewer.update(data, null);
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		// check if all elements have a match
		boolean ok = true;
		for(ObjectIdMatchingData o : objects.values())
			if (o.dstId == 0)
			{
				ok = false;
				break;
			}
		for(DciIdMatchingData d : dcis.values())
			if ((d.dstNodeId == 0) || (d.dstDciId == 0))
			{
				ok = false;
				break;
			}
		
		if (!ok)
		{
			if (!MessageDialogHelper.openQuestion(getShell(), Messages.IdMatchingDialog_MatchingErrors, Messages.IdMatchingDialog_ConfirmationText))
				return;
		}
		
		super.okPressed();
	}
}
