/**
 * 
 */
package org.netxms.ui.eclipse.perfview.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Save graph as predefined
 */
public class SaveGraphDlg extends Dialog
{
   public static final int OVERRIDE = 101;

   private LabeledText fieldName;
	private Label errorMessage;
	private String name;
	private Button checkOverwrite;
	private String ErrorMessage;
	private boolean havePermissionToOverwrite;
	
	/**
	 * @param parentShell
	 */
	public SaveGraphDlg(Shell parentShell, String initialName, String message, boolean havePermissionToOverwrite)
	{
		super(parentShell);
		name = initialName;
		ErrorMessage = message;
		this.havePermissionToOverwrite = havePermissionToOverwrite;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Save Graph");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);
		
		fieldName = new LabeledText(dialogArea, SWT.NONE);
		fieldName.setLabel("Name");
		fieldName.setText(name);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 400;
		fieldName.setLayoutData(gd);
		
      if(ErrorMessage != null && havePermissionToOverwrite)
      {
         errorMessage = new Label(dialogArea, SWT.LEFT);
         errorMessage.setForeground(SharedColors.getColor(SharedColors.STATUS_CRITICAL, parent.getDisplay()));
         errorMessage.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
         errorMessage.setText(ErrorMessage);
         
         checkOverwrite = new Button(dialogArea, SWT.CHECK);
         checkOverwrite.setText("Overwrite existing graph");
      }   
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		name = fieldName.getText().trim();
		if (name.isEmpty())
		{
			MessageDialogHelper.openWarning(getShell(), "Warning", "Predefined graph name must not be empty!");
			return;
		}
		if (ErrorMessage != null && checkOverwrite.getSelection())
		{
		   setReturnCode(OVERRIDE);
		   super.close();
		}
		else
		{
		   super.okPressed();
		}
	}
	
	protected void overridePressed()
   {
      name = fieldName.getText().trim();
      if (name.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Predefined graph name must not be empty!");
         return;
      }
      super.okPressed();
   }

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
}
