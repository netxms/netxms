/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.shared.IUIConstants;


/**
 * @author victor
 *
 */
public class AttributeEditDialog extends Dialog
{
	private Text textName;
	private Text textValue;
	private String attrName;
	private String attrValue;
	
	/**
	 * @param parentShell
	 */
	public AttributeEditDialog(Shell parentShell, String attrName, String attrValue)
	{
		super(parentShell);
		this.attrName = attrName;
		this.attrValue = attrValue;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		FillLayout layout = new FillLayout();
      layout.type = SWT.VERTICAL;
      layout.marginWidth = IUIConstants.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = IUIConstants.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);
		
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Name");
      
      textName = new Text(dialogArea, SWT.SINGLE | SWT.BORDER);
      textName.setTextLimit(63);
      if (attrName != null)
      {
      	textName.setText(attrName);
      	textName.setEditable(false);
      }
      
      label = new Label(dialogArea, SWT.NONE);
      label.setText("");

      label = new Label(dialogArea, SWT.NONE);
      label.setText("Value");

      textValue = new Text(dialogArea, SWT.SINGLE | SWT.BORDER);
      textValue.setTextLimit(255);
      textValue.getShell().setMinimumSize(300, 0);
      if (attrValue != null)
      	textValue.setText(attrValue);
      
      if (attrName != null)
      	textValue.setFocus();
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText((attrName == null) ? "Add Attribute" : "Modify Attribute");
	}
	
	
	/**
	 * Get variable name
	 * 
	 */
	public String getAttrName()
	{
		return attrName;
	}
	
	
	/**
	 * Get variable value
	 * 
	 */
	public String getAttrValue()
	{
		return attrValue;
	}

	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		attrName = textName.getText();
		attrValue = textValue.getText();
		super.okPressed();
	}
}
