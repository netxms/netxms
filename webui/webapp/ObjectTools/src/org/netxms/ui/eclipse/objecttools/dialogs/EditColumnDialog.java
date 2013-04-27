/**
 * 
 */
package org.netxms.ui.eclipse.objecttools.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objecttools.ObjectToolTableColumn;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.ui.eclipse.snmp.dialogs.MibSelectionDialog;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for creating or editing object tool column
 *
 */
public class EditColumnDialog extends Dialog
{
	private static final long serialVersionUID = 1L;
	private static final String[] formatNames = { "String", "Integer", "Float", "IP Address", "MAC Address", "Interface Index" };

	private boolean create;
	private boolean snmpColumn;
	private ObjectToolTableColumn columnObject;
	private LabeledText name;
	private Combo format;
	private LabeledText data;
	private Button selectButton;
	
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
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
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
		gd.widthHint = 350;
		name.setLayoutData(gd);
		
		format = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Format", WidgetHelper.DEFAULT_LAYOUT_DATA);
		for(int i = 0; i < formatNames.length; i++)
			format.add(formatNames[i]);
		format.select(columnObject.getFormat());
		
		Composite dataGroup = null;
		if (snmpColumn)
		{
			dataGroup = new Composite(dialogArea, SWT.NONE);
			layout = new GridLayout();
			layout.marginHeight = 0;
			layout.marginWidth = 0;
			layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
			layout.numColumns = 2;
			dataGroup.setLayout(layout);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			dataGroup.setLayoutData(gd);
		}
		
		data = new LabeledText(snmpColumn ? dataGroup : dialogArea, SWT.NONE);
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
		gd.widthHint = 350;
		data.setLayoutData(gd);
		
		if (snmpColumn)
		{
			selectButton = new Button(dataGroup, SWT.PUSH);
			selectButton.setText("...");
			gd = new GridData();
			gd.verticalAlignment = SWT.BOTTOM;
			selectButton.setLayoutData(gd);
			selectButton.addSelectionListener(new SelectionListener() {
				private static final long serialVersionUID = 1L;

				@Override
				public void widgetSelected(SelectionEvent e)
				{
					SnmpObjectId initial;
					try
					{
						initial = SnmpObjectId.parseSnmpObjectId(data.getText());
					}
					catch(SnmpObjectIdFormatException ex)
					{
						initial = null;
					}
					MibSelectionDialog dlg = new MibSelectionDialog(getShell(), initial, 0);
					if (dlg.open() == Window.OK)
					{
						data.setText(dlg.getSelectedObjectId().toString());
					}
				}
				
				@Override
				public void widgetDefaultSelected(SelectionEvent e)
				{
					widgetSelected(e);
				}
			});
		}
		
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
