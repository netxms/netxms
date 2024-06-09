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
package org.netxms.nxmc.modules.datacollection.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
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
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.WinPerfCounter;
import org.netxms.client.datacollection.WinPerfObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.helpers.StringComparator;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.propertypages.helpers.WinPerfObjectTreeContentProvider;
import org.netxms.nxmc.modules.datacollection.propertypages.helpers.WinPerfObjectTreeLabelProvider;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Selection dialog for Windows Performance counters
 */
public class WinPerfCounterSelectionDialog extends Dialog implements IParameterSelectionDialog
{
   private final I18n i18n = LocalizationHelper.getI18n(WinPerfCounterSelectionDialog.class);

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

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(i18n.tr("Select Windows Performance Counter"));
	}

   /**
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

		new Label(dialogArea, SWT.NONE).setText(i18n.tr("Objects and counters"));
		new Label(dialogArea, SWT.NONE).setText(i18n.tr("Instances"));

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

      objectTree.setInput(WinPerfObject.createPlaceholderList(i18n.tr("Loading...")));

		fillData();
		return dialogArea;
	}

	/**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createButtonBar(Composite parent)
   {
      Control bar = super.createButtonBar(parent);
      getButton(IDialogConstants.OK_ID).setEnabled(false);
      return bar;
   }

   /**
    * Fill data
    */
	private void fillData()
	{
		final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Reading list of available Windows performance counters"), null) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final List<WinPerfObject> objects = session.getNodeWinPerfObjects(nodeId);
            runInUIThread(() -> {
               getButton(IDialogConstants.OK_ID).setEnabled(true);
               objectTree.setInput(objects);
            });
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot get list of available Windows performance counters");
			}
		}.start();
	}

	/**
	 * Object tree selection handler
	 */
	private void onObjectSelection()
	{
		IStructuredSelection selection = objectTree.getStructuredSelection();
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

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		if (selectedCounter == null)
		{
			MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select counter and instance and then press OK"));
			return;
		}

		if (selectedCounter.getObject().hasInstances())
		{
			IStructuredSelection selection = instanceList.getStructuredSelection();
			if (selection.size() != 1)
			{
				MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select counter and instance and then press OK"));
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

   /**
    * @see org.netxms.nxmc.modules.datacollection.dialogs.IParameterSelectionDialog#getParameterName()
    */
	@Override
	public String getParameterName()
	{
		if (selectedInstance == null)
         return "\\" + selectedCounter.getObject().getName() + "\\" + selectedCounter.getName();
      return "\\" + selectedCounter.getObject().getName() + "(" + selectedInstance + ")\\" + selectedCounter.getName();
	}

   /**
    * @see org.netxms.nxmc.modules.datacollection.dialogs.IParameterSelectionDialog#getParameterDescription()
    */
	@Override
	public String getParameterDescription()
	{
		if (selectedInstance == null)
         return selectedCounter.getObject().getName() + ": " + selectedCounter.getName();
      return selectedCounter.getObject().getName() + ": " + selectedCounter.getName() + i18n.tr(" for ") + selectedInstance;
	}

   /**
    * @see org.netxms.nxmc.modules.datacollection.dialogs.IParameterSelectionDialog#getParameterDataType()
    */
	@Override
	public DataType getParameterDataType()
	{
		return DataType.INT64;
	}

   /**
    * @see org.netxms.nxmc.modules.datacollection.dialogs.IParameterSelectionDialog#getInstanceColumn()
    */
	@Override
	public String getInstanceColumn()
	{
      return "";
	}
}
