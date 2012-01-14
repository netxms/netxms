package org.netxms.ui.eclipse.imagelibrary.dialogs;

import java.io.File;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
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
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;

public class ImagePropertiesDialog extends Dialog
{
	private String name;
	private String category;
	private String fileName;

	private Text fileNameInputField;
	private Text nameInputField;
	private Combo categoryCombo;
	private Set<String> categories;
	private Shell shell;

	public ImagePropertiesDialog(Shell parentShell, Set<String> knownCategories)
	{
		super(parentShell);
		this.shell = parentShell;
		this.categories = knownCategories;
	}

	@Override
	protected Control createDialogArea(Composite parent)
	{
		final Composite dialogArea = (Composite)super.createDialogArea(parent);

		final GridLayout layout = new GridLayout();
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		dialogArea.setLayout(layout);

		WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, new WidgetFactory()
		{
			@Override
			public Control createControl(Composite parent, int style)
			{
				final Composite composite = new Composite(parent, SWT.NONE);
				final GridLayout compositeLayout = new GridLayout();
				compositeLayout.numColumns = 2;
				// compositeLayout.verticalSpacing = WidgetHelper.INNER_SPACING;
				composite.setLayout(compositeLayout);
				fileNameInputField = new Text(composite, SWT.SINGLE | SWT.READ_ONLY | SWT.BORDER);
				GridData gd = new GridData();
				gd.widthHint = 300;
				// gd.heightHint = 300;
				gd.grabExcessHorizontalSpace = true;
				gd.grabExcessVerticalSpace = true;
				gd.horizontalAlignment = SWT.FILL;
				gd.verticalAlignment = SWT.FILL;
				fileNameInputField.setLayoutData(gd);

				final Button button = new Button(composite, SWT.NONE);
				button.addSelectionListener(new SelectionListener()
				{
					@Override
					public void widgetSelected(SelectionEvent e)
					{
					}

					@Override
					public void widgetDefaultSelected(SelectionEvent e)
					{
					}
				});
				button.setText("...");
				return composite;
			}
		}, "Image File", WidgetHelper.DEFAULT_LAYOUT_DATA);

		// nameInputField = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE
		// | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, "File Name",
		// fileName == null ? "n/a" : fileName, WidgetHelper.DEFAULT_LAYOUT_DATA);
		// nameInputField.getShell().setMinimumSize(300, 0);

		nameInputField = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, "Image name",
				name == null ? "" : name, WidgetHelper.DEFAULT_LAYOUT_DATA);
		nameInputField.getShell().setMinimumSize(300, 0);

		categoryCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.BORDER, "Category",
				WidgetHelper.DEFAULT_LAYOUT_DATA);
		categoryCombo.getShell().setMinimumSize(300, 0);

		int counter = 0;
		for(String name : categories)
		{
			categoryCombo.add(name);
			if (name.equals(category))
			{
				categoryCombo.select(counter);
			}
			counter++;
		}

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		category = categoryCombo.getText();
		name = nameInputField.getText();

		super.okPressed();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Upload New Image");
	}

	public void setDefaultCategory(String category)
	{
		this.category = category;
	}

	public String getCategory()
	{
		return category;
	}

	public String getName()
	{
		return name;
	}

	public void setName(String name)
	{
		this.name = name;
	}

	public String getFileName()
	{
		return fileName;
	}
}
