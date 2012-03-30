/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;

/**
 * Column definition editing dialog
 */
public class EditColumnDialog extends Dialog
{
	private ColumnDefinition column;
	private ScriptEditor transformationScript; 
	
	/**
	 * @param parentShell
	 */
	public EditColumnDialog(Shell parentShell, ColumnDefinition column)
	{
		super(parentShell);
		this.column = column;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Column Definition: " + column.getName());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		
		
		return dialogArea;
	}
	
}
