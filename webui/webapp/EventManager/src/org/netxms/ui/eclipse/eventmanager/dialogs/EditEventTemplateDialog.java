/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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

import java.util.HashSet;
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
import org.netxms.ui.eclipse.eventmanager.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * @author Victor
 *
 */
public class EditEventTemplateDialog extends Dialog
{
	private EventTemplate eventTemplate;
	private boolean isNew;
	private LabeledText id;
   private LabeledText guid;
	private LabeledText name;
	private LabeledText message;
   private LabeledText tags;
	private LabeledText description;
	private Combo severity;
	private Button optionLog;
	
	/**
	 * Default constructor.
	 * 
	 * @param parentShell
	 * @param eventTemplate
	 * @param isNew
	 */
	public EditEventTemplateDialog(Shell parentShell, EventTemplate eventTemplate, boolean isNew)
	{
		super(parentShell);
		this.eventTemplate = eventTemplate;
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
      layout.numColumns = 3;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING * 2;
      dialogArea.setLayout(layout);
      
      id = new LabeledText(dialogArea, SWT.NONE);
      id.setLabel(Messages.get().EditEventTemplateDialog_EventCode);
      id.setText(Long.toString(eventTemplate.getCode()));
      id.getTextControl().setEditable(false);
      
      guid = new LabeledText(dialogArea, SWT.NONE);
      guid.setLabel("GUID");
      guid.setText(eventTemplate.getGuid().toString());
      guid.getTextControl().setEditable(false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      guid.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      severity = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, Messages.get().EditEventTemplateDialog_Severity, gd);
      severity.add(StatusDisplayInfo.getStatusText(Severity.NORMAL));
      severity.add(StatusDisplayInfo.getStatusText(Severity.WARNING));
      severity.add(StatusDisplayInfo.getStatusText(Severity.MINOR));
      severity.add(StatusDisplayInfo.getStatusText(Severity.MAJOR));
      severity.add(StatusDisplayInfo.getStatusText(Severity.CRITICAL));
      severity.select(eventTemplate.getSeverity().getValue());
      
      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(Messages.get().EditEventTemplateDialog_EventName);
      name.setText(eventTemplate.getName());
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      name.setLayoutData(gd);
      
      optionLog = new Button(dialogArea, SWT.CHECK);
      optionLog.setText(Messages.get().EditEventTemplateDialog_WriteToLog);
      optionLog.setSelection((eventTemplate.getFlags() & EventTemplate.FLAG_WRITE_TO_LOG) != 0);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = false;
      gd.horizontalAlignment = SWT.RIGHT;
      gd.verticalAlignment = SWT.BOTTOM;
      optionLog.setLayoutData(gd);
      
      message = new LabeledText(dialogArea, SWT.NONE);
      message.setLabel(Messages.get().EditEventTemplateDialog_Message);
      message.setText(eventTemplate.getMessage());
      gd = new GridData();
      gd.horizontalSpan = 3;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = 450;
      message.setLayoutData(gd);
		
      tags = new LabeledText(dialogArea, SWT.NONE);
      tags.setLabel("Tags");
      tags.setText(eventTemplate.getTagList());
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = 450;
      tags.setLayoutData(gd);
      
      description = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL | SWT.WRAP);
      description.setLabel(Messages.get().EditEventTemplateDialog_Description);
      description.setText(eventTemplate.getDescription());
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 200;
      gd.widthHint = 450;
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
		eventTemplate.setName(name.getText());
		eventTemplate.setSeverity(Severity.getByValue(severity.getSelectionIndex()));
		eventTemplate.setMessage(message.getText());
		eventTemplate.setDescription(description.getText());
		eventTemplate.setFlags(optionLog.getSelection() ? EventTemplate.FLAG_WRITE_TO_LOG : 0);
		
		HashSet<String> tagSet = new HashSet<String>();
		for(String s : tags.getText().split(","))
		{
		   tagSet.add(s.trim());
		}
		eventTemplate.setTags(tagSet);
		
		super.okPressed();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		newShell.setText(isNew ? Messages.get().EditEventTemplateDialog_TitleCreate : Messages.get().EditEventTemplateDialog_TitleEdit);
		super.configureShell(newShell);
	}
}
