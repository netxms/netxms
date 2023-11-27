/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.filemanager.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerFile;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.views.helpers.ServerFileComparator;
import org.netxms.nxmc.modules.filemanager.views.helpers.ServerFileLabelProvider;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting files from server's file store
 *
 */
public class SelectServerFileDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SelectServerFileDialog.class);
   
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_SIZE = 1;
	public static final int COLUMN_MODTIME = 2;
	
	private SortableTableViewer viewer;
	private ServerFile[] selectedFiles;
	private boolean multiSelect;
	
	/**
	 * @param parentShell
	 * @param multiSelect
	 */
	public SelectServerFileDialog(Shell parentShell, boolean multiSelect)
	{
		super(parentShell);
		this.multiSelect = multiSelect;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(i18n.tr("Select File on Server"));
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);
		
		final String[] names = { i18n.tr("Name"), i18n.tr("Size"), i18n.tr("Modified") };
		final int[] widths = { 200, 100, 150 };
		viewer = new SortableTableViewer(dialogArea, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.BORDER | (multiSelect ? SWT.MULTI : 0));
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ServerFileLabelProvider());
		viewer.setComparator(new ServerFileComparator());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 450;
		viewer.getControl().setLayoutData(gd);
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				okPressed();
			}
		});
		
		final NXCSession session = Registry.getSession();
		new Job(i18n.tr("Get server file list"), null) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final ServerFile[] files = session.listServerFiles();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(files);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot get file store content");
			}
		}.start();
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void okPressed()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.isEmpty())
		{
			MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select file from the list"));
			return;
		}
		
		final List<ServerFile> list = selection.toList();
		selectedFiles = list.toArray(new ServerFile[list.size()]);
		
		super.okPressed();
	}

	/**
	 * @return the selectedFiles
	 */
	public ServerFile[] getSelectedFiles()
	{
		return selectedFiles;
	}
}
