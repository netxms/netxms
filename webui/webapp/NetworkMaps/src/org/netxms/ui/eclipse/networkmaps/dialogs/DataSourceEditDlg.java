/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden SOlutions
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
package org.netxms.ui.eclipse.networkmaps.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.maps.configs.MapDataSource;
import org.netxms.ui.eclipse.datacollection.widgets.DciSelector;
import org.netxms.ui.eclipse.networkmaps.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Data source editing dialog for map line
 */
public class DataSourceEditDlg extends Dialog
{
	private MapDataSource dci;
	private DciSelector dciSelector;
	private LabeledText instance;
	private LabeledText dataColumn;
	private LabeledText formatString;
	
	/**
	 * @param parentShell
	 * @param dci
	 */
	public DataSourceEditDlg(Shell parentShell, MapDataSource dci)
	{
		super(parentShell);
		this.dci = dci;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().DataSourceEditDlg_Title);
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
      dciSelector = new DciSelector(dialogArea, SWT.NONE);
		dciSelector.setLabel(Messages.get().DataSourceEditDlg_DCI);
		dciSelector.setDciId(dci.getNodeId(), dci.getDciId());
		dciSelector.setDcObjectType(dci.getType());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 400;
		gd.horizontalSpan = 2;
		dciSelector.setLayoutData(gd);
      
		formatString = new LabeledText(dialogArea, SWT.NONE);
		formatString.setLabel(Messages.get().DataSourceEditDlg_FormatString);
		formatString.setText(dci.getFormatString());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      formatString.setLayoutData(gd);
      
		if (dci.getType() == MapDataSource.TABLE)
		{
			Group tableGroup = new Group(dialogArea, SWT.NONE);
			tableGroup.setText(Messages.get().DataSourceEditDlg_TableCell);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			gd.horizontalSpan = 2;
			tableGroup.setLayoutData(gd);
			
			layout = new GridLayout();
			tableGroup.setLayout(layout);
			
			dataColumn = new LabeledText(tableGroup, SWT.NONE);
			dataColumn.setLabel(Messages.get().DataSourceEditDlg_DataColumn);
			dataColumn.setText(dci.getColumn());
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			dataColumn.setLayoutData(gd);
			
			instance = new LabeledText(tableGroup, SWT.NONE);
			instance.setLabel(Messages.get().DataSourceEditDlg_Instance);
			instance.setText(dci.getInstance());
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			instance.setLayoutData(gd);
		}
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		dci.setNodeId(dciSelector.getNodeId());
		dci.setDciId(dciSelector.getDciId());
		dci.setFormatString(formatString.getText());
		if (dci.getType() == MapDataSource.TABLE)
		{
			dci.setColumn(dataColumn.getText().trim());
			dci.setInstance(instance.getText());
		}		
		super.okPressed();
	}
}
