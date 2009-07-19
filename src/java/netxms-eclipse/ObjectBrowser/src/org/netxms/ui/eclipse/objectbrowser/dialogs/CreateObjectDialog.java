/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser.dialogs;

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
 * @author Victor
 *
 */
public class CreateObjectDialog extends Dialog
{
	private String objectClassName;
	private String objectName;
	private Text textName;

	/**
	 * Constructor
	 * 
	 * @param parentShell Parent shell
	 * @param objectClassName Object class - this string will be added to dialog's title
	 */
	public CreateObjectDialog(Shell parentShell, String objectClassName)
	{
		super(parentShell);
		this.objectClassName = objectClassName;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.getString("CreateObjectDialog.title") + objectClassName); //$NON-NLS-1$
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
      label.setText(Messages.getString("CreateObjectDialog.object_name")); //$NON-NLS-1$
      
      textName = new Text(dialogArea, SWT.SINGLE | SWT.BORDER);
      textName.setTextLimit(63);
      textName.setFocus();
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		objectName = textName.getText();
		super.okPressed();
	}

	/**
	 * @return the objectName
	 */
	public String getObjectName()
	{
		return objectName;
	}
}
