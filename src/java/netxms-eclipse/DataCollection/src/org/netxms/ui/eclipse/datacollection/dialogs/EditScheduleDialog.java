/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.shared.IUIConstants;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class EditScheduleDialog extends Dialog
{
	private Text textSchedule;
	private String schedule;
	
	/**
	 * Constructor
	 * 
	 * @param parent Parent shell
	 */
	public EditScheduleDialog(Shell parent, String currentValue)
	{
		super(parent);
		schedule = currentValue;
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
      dialogArea.setLayout(layout);
		
      textSchedule = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, "Schedule", schedule,
                                                    WidgetHelper.DEFAULT_LAYOUT_DATA);
      textSchedule.getShell().setMinimumSize(300, 0);
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		schedule = textSchedule.getText();
		super.okPressed();
	}

	/**
	 * @return Schedule entered
	 */
	public String getSchedule()
	{
		return schedule;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Edit Schedule");
	}
}
