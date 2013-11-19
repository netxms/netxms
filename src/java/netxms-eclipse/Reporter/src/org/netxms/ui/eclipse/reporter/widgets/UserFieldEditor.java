package org.netxms.ui.eclipse.reporter.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.dialogs.SelectUserDialog;

public class UserFieldEditor extends FieldEditor
{

	private static final String EMPTY_SELECTION_TEXT = "<none>";
	private WorkbenchLabelProvider labelProvider;
	private CLabel text;
	private boolean returnName;
	private AbstractUserObject user;

	public UserFieldEditor(ReportParameter parameter, FormToolkit toolkit, Composite parent, boolean returnName)
	{
		super(parameter, toolkit, parent);
		this.returnName = returnName;
		labelProvider = new WorkbenchLabelProvider();
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelProvider.dispose();
			}
		});
	}

	@Override
	protected void createContent(Composite parent)
	{
		Composite content = toolkit.createComposite(parent, SWT.BORDER);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		content.setLayoutData(gd);

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		content.setLayout(layout);

		text = new CLabel(content, SWT.NONE);
		toolkit.adapt(text);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		text.setLayoutData(gd);
		text.setText(EMPTY_SELECTION_TEXT);

		final ImageHyperlink selectionLink = toolkit.createImageHyperlink(content, SWT.NONE);
		selectionLink.setImage(SharedIcons.IMG_FIND);
		selectionLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				selectUser();
			}
		});
	}

	/**
	 * 
	 */
	protected void selectUser()
	{
		final SelectUserDialog dialog = new SelectUserDialog(getShell(), false);
		if (dialog.open() == Window.OK)
		{
			final AbstractUserObject[] selection = dialog.getSelection();
			if (selection.length > 0)
			{
				final AbstractUserObject user = selection[0];
				this.user = user;
				text.setText(user.getName());
				text.setImage(labelProvider.getImage(user));
			}
			else
			{
				this.user = null;
				text.setText(EMPTY_SELECTION_TEXT);
				text.setImage(null);
			}
		}
	}

	@Override
	public String getValue()
	{
		if (user != null)
		{
			if (returnName)
			{
				return user.getName();
			}
			else
			{
				return Long.toString(user.getId());
			}
		}
		return null;
	}

}
