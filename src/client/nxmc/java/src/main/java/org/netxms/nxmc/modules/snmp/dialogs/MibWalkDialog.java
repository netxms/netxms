/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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
package org.netxms.nxmc.modules.snmp.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.client.snmp.SnmpWalkListener;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.snmp.helpers.SnmpValueLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * MIB walk dialog
 */
public class MibWalkDialog extends Dialog implements SnmpWalkListener
{
   private I18n i18n = LocalizationHelper.getI18n(MibWalkDialog.class);
   
	public static final int COLUMN_OID = 0;
	public static final int COLUMN_TYPE = 1;
	public static final int COLUMN_VALUE = 2;
	
	private long nodeId;
	private SnmpObjectId rootObject;
   private SortableTableViewer viewer;
	private List<SnmpValue> walkData = new ArrayList<SnmpValue>();
	private SnmpValue value;
	
	/**
	 * @param parentShell
	 * @param rootObject root object to start walk from
	 */
	public MibWalkDialog(Shell parentShell, long nodeId, SnmpObjectId rootObject)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.nodeId = nodeId;
		this.rootObject = rootObject;
	}
	
   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(i18n.tr("MIB Walk Results"));
		PreferenceStore settings = PreferenceStore.getInstance();
		newShell.setSize(settings.getAsPoint("MibWalkDialog.size", 400, 250)); 
	}	

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);

      viewer = new SortableTableViewer(dialogArea, SWT.FULL_SELECTION | SWT.SINGLE | SWT.BORDER);
		setupViewerColumns();
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SnmpValueLabelProvider()); 
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.widthHint = 600;
		gd.heightHint = 400;
		viewer.getTable().setLayoutData(gd);
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				okPressed();
			}
		});
		
		return dialogArea;
	}

	/**
	 * Setup viewer columns
	 */
	private void setupViewerColumns()
	{
		TableColumn tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(i18n.tr("OID"));
		tc.setWidth(300);
		
      tc = new TableColumn(viewer.getTable(), SWT.LEFT);
      tc.setText("OID as text");
      tc.setWidth(300);

		tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(i18n.tr("Type"));
		tc.setWidth(100);

		tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(i18n.tr("Value"));
		tc.setWidth(300);
	}
	
   /**
    * @see org.eclipse.jface.dialogs.Dialog#create()
    */
	@Override
	public void create()
	{
		super.create();
		startWalk();
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		value = (SnmpValue)selection.getFirstElement();
		
		saveSettings();
		super.okPressed();
	}

	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		Point size = getShell().getSize();
		PreferenceStore settings = PreferenceStore.getInstance();

		settings.set("MibWalkDialog.size", size); //$NON-NLS-1$
	}

	/**
	 * Start SNMP walk
	 */
	private void startWalk()
	{
		getButton(IDialogConstants.OK_ID).setEnabled(false);
		getButton(IDialogConstants.CANCEL_ID).setEnabled(false);
		final NXCSession session = Registry.getSession();
		new Job(i18n.tr("Walk MIB tree"), null) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.snmpWalk(nodeId, rootObject.toString(), MibWalkDialog.this);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot do SNMP MIB tree walk");
			}

			@Override
			protected void jobFinalize()
			{
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						getButton(IDialogConstants.OK_ID).setEnabled(true);
						getButton(IDialogConstants.CANCEL_ID).setEnabled(true);
					}
				});
			}
		}.start();
	}

   /**
    * @see org.netxms.client.snmp.SnmpWalkListener#onSnmpWalkData(long, java.util.List)
    */
	@Override
   public void onSnmpWalkData(final long nodeId, final List<SnmpValue> data)
	{
		viewer.getControl().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run()
			{
				walkData.addAll(data);
				viewer.setInput(walkData.toArray());
            viewer.packColumns();
				try
				{
					viewer.getTable().showItem(viewer.getTable().getItem(viewer.getTable().getItemCount() - 1));
				}
				catch(Exception e)
				{
					// silently ignore any possible problem with table scrolling
				}
			}
		});
	}

	/**
	 * @return
	 */
	public SnmpValue getValue()
	{
		return value;
	}
}
