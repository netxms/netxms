/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for adding group box decoration
 *
 */
public class AddGroupBoxDialog extends Dialog
{
	private String title;
	private int width;
	private int height;
	private LabeledText textTitle;
	private Spinner spinnerWidth;
	private Spinner spinnerHeight;
	
	/**
	 * 
	 * @param parentShell
	 */
	public AddGroupBoxDialog(Shell parentShell)
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
		newShell.setText("Create Group Box");
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
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		dialogArea.setLayout(layout);
		
		textTitle = new LabeledText(dialogArea, SWT.NONE);
		textTitle.setLabel("Title");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		textTitle.setLayoutData(gd);
		
		Composite sizeArea = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.numColumns = 2;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
		sizeArea.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		sizeArea.setLayoutData(gd);
		
		WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite parent, int style)
			{
				Spinner spinner = new Spinner(parent, style | SWT.BORDER);
				spinner.setMinimum(20);
				spinner.setMaximum(4096);
				return spinner;
			}
		};
		spinnerWidth = (Spinner)WidgetHelper.createLabeledControl(sizeArea, SWT.NONE, factory, "Width", WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerWidth.setSelection(250);
		spinnerHeight = (Spinner)WidgetHelper.createLabeledControl(sizeArea, SWT.NONE, factory, "Height", WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerHeight.setSelection(100);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		title = textTitle.getText();
		width = spinnerWidth.getSelection();
		height = spinnerHeight.getSelection();
		super.okPressed();
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @return the width
	 */
	public int getWidth()
	{
		return width;
	}

	/**
	 * @return the height
	 */
	public int getHeight()
	{
		return height;
	}
}
