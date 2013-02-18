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
package org.netxms.ui.eclipse.reporter.propertypages;

import java.io.File;
import java.io.FileInputStream;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Report;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.reporter.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Report Definition" property page
 */
public class ReportDefinition extends PropertyPage
{
	private Report object;
	private Text reportDefinition;
	private boolean modified = false;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (Report)getElement().getAdapter(Report.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      // Label
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Report definition");

      // Report definition
      reportDefinition = new Text(dialogArea, SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL);
      reportDefinition.setText(object.getDefinition());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.widthHint = 0;
      gd.heightHint = 0;
      reportDefinition.setLayoutData(gd);
      reportDefinition.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				modified = true;
			}
      });
      
      // "Update from file" button
      final Button updateButton = new Button(dialogArea, SWT.PUSH);
      updateButton.setText("Update from file...");
      updateButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				updateDefinitionFromFile();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
      
		return dialogArea;
	}

	/**
	 * Update report definition from file
	 */
	private void updateDefinitionFromFile()
	{
		FileDialog fd = new FileDialog(getShell(), SWT.OPEN);
		fd.setText("Select File");
		fd.setFilterExtensions(new String[] { "*.jrxml", "*.*" });
		fd.setFilterNames(new String[] { "Jasper Report Files", "All Files" });
		String selected = fd.open();
		if (selected != null)
		{
			File file = new File(selected);
			byte[] buffer = new byte[(int)file.length()];
			try
			{
				FileInputStream in = new FileInputStream(file);
				try
				{
					in.read(buffer);
				}
				finally
				{
					if (in != null)
						in.close();
				}
				reportDefinition.setText(new String(buffer));
				modified = true;
			}
			catch(Exception e)
			{
				MessageDialogHelper.openError(getShell(), "Error", "Cannot load report definition from file: " + e.getLocalizedMessage());
			}
		}
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (!modified)
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final String newDefinition = reportDefinition.getText();
		new ConsoleJob("Update report definition", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				((NXCSession)ConsoleSharedData.getSession()).setReportDefinition(object.getObjectId(), newDefinition);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update report definition";
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					PlatformUI.getWorkbench().getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							ReportDefinition.this.setValid(true);
							modified = false;
						}
					});
				}
			}
		}.start();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
