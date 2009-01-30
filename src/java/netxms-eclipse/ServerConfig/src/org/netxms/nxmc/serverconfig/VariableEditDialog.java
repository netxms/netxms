/**
 * 
 */
package org.netxms.nxmc.serverconfig;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.core.extensionproviders.IUIConstants;


/**
 * @author victor
 *
 */
public class VariableEditDialog extends Dialog
{
	private Text textName;
	private Text textValue;
	private String varName;
	private String varValue;
	
	/**
	 * @param parentShell
	 */
	public VariableEditDialog(Shell parentShell, String varName)
	{
		super(parentShell);
		this.varName = varName;
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
      if (varName != null)
      {
      	textName.setText(varName);
      	textName.setEditable(false);
      }

      label = new Label(dialogArea, SWT.NONE);
      label.setText("Value");

      textValue = new Text(dialogArea, SWT.SINGLE | SWT.BORDER);
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText((varName == null) ? "Create Variable" : "Edit Variable");
	}
	
	
	/**
	 * Get variable name
	 * 
	 */
	public String getVarName()
	{
		return varName;
	}
	
	
	/**
	 * Get variable value
	 * 
	 */
	public String getVarValue()
	{
		return varValue;
	}

	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		varName = textName.getText();
		varValue = textValue.getText();
		super.okPressed();
	}
}
