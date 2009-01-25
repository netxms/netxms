/**
 * 
 */
package org.netxms.nxmc.objectbrowser;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.window.IShellProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.core.extensionproviders.IUIConstants;

/**
 * @author Victor
 * 
 */
public class ObjectSelectionDialog extends Dialog
{
	private Text textFilter;

	/**
	 * @param parentShell
	 */
	public ObjectSelectionDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/**
	 * @param parentShell
	 */
	public ObjectSelectionDialog(IShellProvider parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText("Select Object");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      RowLayout rowLayout = new RowLayout();
      rowLayout.type = SWT.VERTICAL;
      rowLayout.marginWidth = IUIConstants.DIALOG_WIDTH_MARGIN;
      rowLayout.marginHeight = IUIConstants.DIALOG_HEIGHT_MARGIN;
      rowLayout.fill = true;
      dialogArea.setLayout(rowLayout);

      // Filter
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Filter");
      textFilter = new Text(dialogArea, SWT.SINGLE | SWT.BORDER);
      String text = settings.get("SelectObject.Filter");
      if (text != null)
      	textFilter.setText(text);

      // Object tree
      label = new Label(dialogArea, SWT.NONE);
      label.setText("Objects");
      
      return dialogArea;
	}

	
}
