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
package org.netxms.ui.eclipse.datacollection.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.WinPerfCounter;
import org.netxms.client.datacollection.WinPerfObject;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.WinPerfObjectTreeContentProvider;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.WinPerfObjectTreeLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.StringComparator;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Selection dialog for Windows Performance counters
 *
 */
public class WinPerfCounterSelectionDialog extends Dialog implements IParameterSelectionDialog
{
	private long nodeId;
	private TreeViewer objectTree;
	private TableViewer instanceList;
	private WinPerfCounter selectedCounter = null;
	private String selectedInstance = null;
	
	/**
	 * @param parentShell
	 */
	public WinPerfCounterSelectionDialog(Shell parentShell, long nodeId)
	{
		super(parentShell);
		this.nodeId = nodeId;
		setShellStyle(getShellStyle() | SWT.RESIZE);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Select Windows Performance Counter");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		dialogArea.setLayout(layout);
		
		new Label(dialogArea, SWT.NONE).setText("Objects and counters");
		new Label(dialogArea, SWT.NONE).setText("Instances");
		
		objectTree = new TreeViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 400;
		gd.widthHint = 350;
		objectTree.getControl().setLayoutData(gd);
		objectTree.setContentProvider(new WinPerfObjectTreeContentProvider());
		objectTree.setLabelProvider(new WinPerfObjectTreeLabelProvider());
		objectTree.setComparator(new ViewerComparator() {
			@Override
			public int compare(Viewer viewer, Object e1, Object e2)
			{
				String s1 = (e1 instanceof WinPerfCounter) ? ((WinPerfCounter)e1).getName() : ((WinPerfObject)e1).getName();
				String s2 = (e2 instanceof WinPerfCounter) ? ((WinPerfCounter)e2).getName() : ((WinPerfObject)e2).getName();
				return s1.compareToIgnoreCase(s2);
			}
		});
		objectTree.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				onObjectSelection();
			}
		});
		
		instanceList = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.widthHint = 350;
		instanceList.getControl().setLayoutData(gd);
		instanceList.setContentProvider(new ArrayContentProvider());
		instanceList.setLabelProvider(new LabelProvider());
		instanceList.setComparator(new StringComparator());
		
		fillData();
		return dialogArea;
	}
	
	/**
	 * Fill data
	 */
	private void fillData()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Get list of available Windows performance counters", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<WinPerfObject> objects = session.getNodeWinPerfObjects(nodeId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						objectTree.setInput(objects);
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get list of available Windows performance counters";
			}
		}.start();
	}
	
	/**
	 * Object tree selection handler
	 */
	private void onObjectSelection()
	{
		IStructuredSelection selection = (IStructuredSelection)objectTree.getSelection();
		if (selection.size() == 0)
		{
			instanceList.setInput(new String[0]);
			return;
		}
		
		WinPerfObject object = (selection.getFirstElement() instanceof WinPerfObject) ? (WinPerfObject)selection.getFirstElement() : ((WinPerfCounter)selection.getFirstElement()).getObject();
		instanceList.setInput(object.getInstances());
		instanceList.getTable().setEnabled(object.hasInstances());
		selectedCounter = (selection.getFirstElement() instanceof WinPerfCounter) ? (WinPerfCounter)selection.getFirstElement() : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		if (selectedCounter == null)
		{
			MessageDialogHelper.openWarning(getShell(), "Warning", "Please select counter and instance and then press OK");
			return;
		}
		
		if (selectedCounter.getObject().hasInstances())
		{
			IStructuredSelection selection = (IStructuredSelection)instanceList.getSelection();
			if (selection.size() != 1)
			{
				MessageDialogHelper.openWarning(getShell(), "Warning", "Please select counter and instance and then press OK");
				return;
			}
			selectedInstance = (String)selection.getFirstElement();
		}
		else
		{
			selectedInstance = null;
		}
		super.okPressed();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterName()
	 */
	@Override
	public String getParameterName()
	{
		if (selectedInstance == null)
			return "\\" + selectedCounter.getObject().getName() + "\\" + selectedCounter.getName();
		return "\\" + selectedCounter.getObject().getName() + "(" + selectedInstance + ")\\" + selectedCounter.getName();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDescription()
	 */
	@Override
	public String getParameterDescription()
	{
		if (selectedInstance == null)
			return selectedCounter.getObject().getName() + ": " + selectedCounter.getName();
		return selectedCounter.getObject().getName() + ": " + selectedCounter.getName() + " for " + selectedInstance;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDataType()
	 */
	@Override
	public int getParameterDataType()
	{
		return DataCollectionItem.DT_INT64;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getInstanceColumn()
	 */
	@Override
	public String getInstanceColumn()
	{
		return "";
	}
}
