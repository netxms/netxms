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

import java.io.File;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.filemanager.widgets.LocalFileSelector;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Import Dashboard" dialog
 */
public class ImportDashboardDialog extends Dialog
{
	private String objectName;
	private File importFile;
	private LabeledText textName;
	private LocalFileSelector importFileSelector;

	/**
	 * Constructor
	 * 
	 * @param parentShell Parent shell
	 */
	public ImportDashboardDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.ImportDashboardDialog_Title);
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
		
      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(Messages.ImportDashboardDialog_ObjectName);
      textName.getTextControl().setTextLimit(63);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 350;
      textName.setLayoutData(gd);
      
      importFileSelector = new LocalFileSelector(dialogArea, SWT.NONE, false, SWT.OPEN);
      importFileSelector.setLabel(Messages.ImportDashboardDialog_ImportFile);
      importFileSelector.setFilterExtensions(new String[] { "*.xml", "*.*" }); //$NON-NLS-1$ //$NON-NLS-2$
      importFileSelector.setFilterNames(new String[] { Messages.ImportDashboardDialog_XMLFiles, Messages.ImportDashboardDialog_AllFiles });
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 350;
      importFileSelector.setLayoutData(gd);
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		objectName = textName.getText().trim();
		if (objectName.isEmpty())
		{
			MessageDialogHelper.openWarning(getShell(), Messages.ImportDashboardDialog_Warning, Messages.ImportDashboardDialog_WarningValidName);
			return;
		}
		importFile = importFileSelector.getFile();
		if (importFile == null)
		{
			MessageDialogHelper.openWarning(getShell(), Messages.ImportDashboardDialog_Warning, Messages.ImportDashboardDialog_WarningSelectFile);
			return;
		}
		super.okPressed();
	}

	/**
	 * @return the objectName
	 */
	public String getObjectName()
	{
		return objectName;
	}

	/**
	 * @return the importFile
	 */
	public File getImportFile()
	{
		return importFile;
	}
}
