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
package org.netxms.ui.eclipse.nxsl.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.api.client.scripts.Script;
import org.netxms.api.client.scripts.ScriptLibraryManager;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.Activator;
import org.netxms.ui.eclipse.nxsl.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dialog for selecting library script
 */
public class SelectScriptDialog extends Dialog
{
	private TableViewer viewer;
	private Script script;
	
	/**
	 * @param parentShell
	 */
	public SelectScriptDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		newShell.setText(Messages.SelectScriptDialog_Title);
		super.configureShell(newShell);
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
		
		new Label(dialogArea, SWT.NONE).setText(Messages.SelectScriptDialog_AvailableScripts);
		
      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider() {
			@Override
			public String getText(Object element)
			{
				return ((Script)element).getName();
			}
      });
      viewer.setComparator(new ViewerComparator() {
      	@Override
      	public int compare(Viewer viewer, Object e1, Object e2)
      	{
      		Script s1 = (Script)e1;
      		Script s2 = (Script)e2;
				return s1.getName().compareToIgnoreCase(s2.getName());
      	}
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				SelectScriptDialog.this.okPressed();
			}
      });
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 300;
      gd.widthHint = 400;
      viewer.getControl().setLayoutData(gd);
      
		final ScriptLibraryManager session = (ScriptLibraryManager)ConsoleSharedData.getSession();
      new ConsoleJob(Messages.SelectScriptDialog_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<Script> scripts = session.getScriptLibrary();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(scripts.toArray());
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.SelectScriptDialog_JobError;
			}
		}.start();
      
      return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 0)
		{
			MessageDialogHelper.openWarning(getShell(), Messages.SelectScriptDialog_Warning, Messages.SelectScriptDialog_WarningEmptySelection);
			return;
		}
		script = (Script)selection.getFirstElement();
		super.okPressed();
	}

	/**
	 * @return selected script
	 */
	public Script getScript()
	{
		return script;
	}
}
