/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dialog which asks user what to do with DCIs created by template being removed
 */
public class DciRemoveConfirmationDialog extends Dialog
{
	private boolean removeFlag;
	private Button radioRemove; 
	private Button radioUnbind;
	
	/**
	 * @param parentShell
	 */
	public DciRemoveConfirmationDialog(Shell parentShell)
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
		newShell.setText(Messages.get().DciRemoveConfirmationDialog_Title);
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
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		final Label image = new Label(dialogArea, SWT.NONE);
		image.setImage(Activator.getImageDescriptor("icons/question.png").createImage()); //$NON-NLS-1$
		GridData gd = new GridData();
		gd.verticalSpan = 3;
		image.setLayoutData(gd);
		image.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				image.getImage().dispose();
			}
		});
		
		final Label label = new Label(dialogArea, SWT.WRAP);
		label.setText(Messages.get().DciRemoveConfirmationDialog_WarningText);
		gd = new GridData();
		gd.widthHint = 300;
		label.setLayoutData(gd);
		
		radioRemove = new Button(dialogArea, SWT.RADIO);
		radioRemove.setText(Messages.get().DciRemoveConfirmationDialog_Remove);
		radioRemove.setSelection(true);
		
		radioUnbind = new Button(dialogArea, SWT.RADIO);
		radioUnbind.setText(Messages.get().DciRemoveConfirmationDialog_Unbind);
		radioUnbind.setSelection(false);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		removeFlag = radioRemove.getSelection();
		super.okPressed();
	}

	/**
	 * @return the removeFlag
	 */
	public boolean getRemoveFlag()
	{
		return removeFlag;
	}
}
