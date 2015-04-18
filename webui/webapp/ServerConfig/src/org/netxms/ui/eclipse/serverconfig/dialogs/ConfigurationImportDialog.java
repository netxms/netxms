/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.serverconfig.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dialog for configuration import
 *
 */
public class ConfigurationImportDialog extends Dialog
{
	private Text textFileName;
	private Button browseButton;
	private Button overrideEventsByName;
	private Button overrideEventsByCode;
	
	private String fileName;
	private int flags;
	
	/**
	 * @param parentShell
	 */
	public ConfigurationImportDialog(Shell parentShell)
	{
		super(parentShell);
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
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);
      
      textFileName = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, Messages.get().ConfigurationImportDialog_FileName, null, WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      browseButton = new Button(dialogArea, SWT.PUSH);
      browseButton.setText(Messages.get().ConfigurationImportDialog_Browse);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      browseButton.setLayoutData(gd);
      browseButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				FileDialog fd = new FileDialog(getShell(), SWT.OPEN);
				fd.setText(Messages.get().ConfigurationImportDialog_SelectFile);
				String selected = fd.open();
				if (selected != null)
					textFileName.setText(selected);
			}
      });
      
      Group options = new Group(dialogArea, SWT.NONE);
      FillLayout optionsLayout = new FillLayout(SWT.VERTICAL);
      optionsLayout.spacing = WidgetHelper.OUTER_SPACING;
      options.setLayout(optionsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      options.setLayoutData(gd);
      
      overrideEventsByName = new Button(options, SWT.CHECK);
      overrideEventsByName.setText(Messages.get().ConfigurationImportDialog_ReplaceByName);
      
      overrideEventsByCode = new Button(options, SWT.CHECK);
      overrideEventsByCode.setText(Messages.get().ConfigurationImportDialog_ReplaceByCode);
      
      return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		fileName = textFileName.getText();
		
		flags = 0;
		if (overrideEventsByName.getSelection())
			flags |= NXCSession.CFG_IMPORT_REPLACE_EVENT_BY_NAME;
		if (overrideEventsByCode.getSelection())
			flags |= NXCSession.CFG_IMPORT_REPLACE_EVENT_BY_CODE;
		
		super.okPressed();
	}

	/**
	 * @return the fileName
	 */
	public String getFileName()
	{
		return fileName;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().ConfigurationImportDialog_Title);
	}
}
