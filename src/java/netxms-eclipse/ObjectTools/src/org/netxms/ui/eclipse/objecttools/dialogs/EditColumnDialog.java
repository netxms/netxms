/**
 * 
 */
package org.netxms.ui.eclipse.objecttools.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objecttools.ObjectToolTableColumn;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for creating or editing object tool column
 *
 */
public class EditColumnDialog extends Dialog
{
	private static final String[] formatNames = { "String", "Integer", "Float", "IP Address", "MAC Address", "Interface Index" };
	private boolean create;
	private boolean snmpColumn;
	private ObjectToolTableColumn columnObject;
	private LabeledText name;
	private Combo format;
	private LabeledText data;
	
	/**
	 * 
	 * @param parentShell
	 * @param create
	 * @param snmpColumn
	 */
	public EditColumnDialog(Shell parentShell, boolean create, boolean snmpColumn, ObjectToolTableColumn columnObject)
	{
		super(parentShell);
		this.create = create;
		this.snmpColumn = snmpColumn;
		this.columnObject = columnObject;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(create ? "Create Column" : "Edit Column");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = (Composite)super.createContents(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);
		
		name = new LabeledText(dialogArea, SWT.NONE);
		name.setLabel("Name");
		name.setText(columnObject.getName());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 400;
		name.setLayoutData(gd);
		
		format = WidgetHelper.createLabeledCombo(dialogArea, SWT.NONE, "Format", WidgetHelper.DEFAULT_LAYOUT_DATA);
		for(int i = 0; i < formatNames.length; i++)
			format.add(formatNames[i]);
		format.select(columnObject.getFormat());
		
		data = new LabeledText(dialogArea, SWT.NONE);
		if (snmpColumn)
		{
			data.setLabel("SNMP Object Identifier (OID)");
			data.setText(columnObject.getSnmpOid());
		}
		else
		{
			data.setLabel("Substring index (starting from 1)");
			data.setText(Integer.toString(columnObject.getSubstringIndex()));
		}
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		data.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		if (snmpColumn)
		{
			/* TODO: add OID validation */
			columnObject.setSnmpOid(data.getText());
		}
		else
		{
			try
			{
				int n = Integer.parseInt(data.getText());
				columnObject.setSubstringIndex(n);
			}
			catch(NumberFormatException e)
			{
				MessageDialog.openWarning(getShell(), "Warning", "Please enter valid substring number");
				return;
			}
		}
		
		columnObject.setFormat(format.getSelectionIndex());
		columnObject.setName(name.getText().trim());
		
		super.okPressed();
	}
}
