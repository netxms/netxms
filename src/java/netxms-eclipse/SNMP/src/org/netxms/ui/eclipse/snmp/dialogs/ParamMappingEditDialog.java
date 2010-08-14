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
package org.netxms.ui.eclipse.snmp.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.snmp.SnmpTrapParameterMapping;
import org.netxms.ui.eclipse.shared.IUIConstants;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author victor
 *
 */
public class ParamMappingEditDialog extends Dialog
{
	private SnmpTrapParameterMapping pm;
	private Text description;
	private Button radioByOid;
	private Button radioByPosition;
	private Text objectId;
	private Button buttonSelect;
	private Spinner position;
	
	public ParamMappingEditDialog(Shell parentShell, SnmpTrapParameterMapping pm)
	{
		super(parentShell);
		this.pm = pm;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
      layout.marginWidth = IUIConstants.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = IUIConstants.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		dialogArea.setLayout(layout);
		
		description = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, "Description", pm.getDescription(), WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		Group varbind = new Group(dialogArea, SWT.NONE);
		varbind.setText("Varbind");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		varbind.setLayoutData(gd);
		varbind.setLayout(new RowLayout(SWT.VERTICAL));
		
		radioByOid = new Button(varbind, SWT.RADIO);
		radioByOid.setText("By object ID (OID)");
		radioByOid.setSelection(pm.getType() == SnmpTrapParameterMapping.BY_OBJECT_ID);
		
		Composite oidSelection = new Composite(varbind, SWT.NONE);
		layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		oidSelection.setLayout(layout);
		
		objectId = new Text(oidSelection, SWT.BORDER);
		if (pm.getType() == SnmpTrapParameterMapping.BY_OBJECT_ID)
			objectId.setText(pm.getObjectId().toString());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		objectId.setLayoutData(gd);
		
		buttonSelect = new Button(oidSelection, SWT.PUSH);
		buttonSelect.setText("Select...");
		gd = new GridData();
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		buttonSelect.setLayoutData(gd);
		
		radioByPosition = new Button(varbind, SWT.RADIO);
		radioByPosition.setText("By position");
		radioByPosition.setSelection(pm.getType() == SnmpTrapParameterMapping.BY_POSITION);
		
		Composite positionSelection = new Composite(varbind, SWT.NONE);
		layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		oidSelection.setLayout(layout);
		
		position = new Spinner(positionSelection, SWT.BORDER);
		position.setIncrement(1);
		position.setMaximum(255);
		position.setMinimum(1);
		
		new Label(positionSelection, SWT.NONE).setText("Enter varbind's position in range 1 .. 255"); 
		
		return dialogArea;
	}

}
