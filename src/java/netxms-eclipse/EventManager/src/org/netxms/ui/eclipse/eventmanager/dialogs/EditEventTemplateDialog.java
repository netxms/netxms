/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.eventmanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * @author Victor
 *
 */
public class EditEventTemplateDialog extends Dialog
{
	private EventTemplate object;
	private boolean isNew;
	private LabeledText id;
	private LabeledText name;
	private LabeledText message;
	private LabeledText description;
	private Combo severity;
	private Button optionLog;
	
	/**
	 * Default constructor.
	 * 
	 * @param parentShell
	 */
	public EditEventTemplateDialog(Shell parentShell, EventTemplate object, boolean isNew)
	{
		super(parentShell);
		this.object = object;
		this.isNew = isNew;
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
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING * 2;
      dialogArea.setLayout(layout);
      
      id = new LabeledText(dialogArea, SWT.NONE);
      id.setLabel("Event code");
      id.setText(Long.toString(object.getCode()));
      id.getTextControl().setEditable(false);
      
      GridData gd = new GridData();
      //gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      severity = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, "Severity", gd);
      severity.add(StatusDisplayInfo.getStatusText(Severity.NORMAL));
      severity.add(StatusDisplayInfo.getStatusText(Severity.WARNING));
      severity.add(StatusDisplayInfo.getStatusText(Severity.MINOR));
      severity.add(StatusDisplayInfo.getStatusText(Severity.MAJOR));
      severity.add(StatusDisplayInfo.getStatusText(Severity.CRITICAL));
      severity.select(object.getSeverity());
      
      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel("Event name");
      name.setText(object.getName());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      name.setLayoutData(gd);
      
      optionLog = new Button(dialogArea, SWT.CHECK);
      optionLog.setText("&Write to event log");
      optionLog.setSelection((object.getFlags() & EventTemplate.FLAG_WRITE_TO_LOG) != 0);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = false;
      gd.horizontalAlignment = SWT.RIGHT;
      gd.verticalAlignment = SWT.BOTTOM;
      optionLog.setLayoutData(gd);
      
      message = new LabeledText(dialogArea, SWT.NONE);
      message.setLabel("Message");
      message.setText(object.getMessage());
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = 450;
      message.setLayoutData(gd);
		
      description = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL);
      description.setLabel("Description");
      description.setText(object.getDescription());
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 200;
      gd.verticalAlignment = SWT.FILL;
      description.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		object.setName(name.getText());
		object.setSeverity(severity.getSelectionIndex());
		object.setMessage(message.getText());
		object.setDescription(description.getText());
		object.setFlags(optionLog.getSelection() ? EventTemplate.FLAG_WRITE_TO_LOG : 0);
		super.okPressed();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		newShell.setText(isNew ? "Create Event Template" : "Edit Event Template");
		super.configureShell(newShell);
	}
}
