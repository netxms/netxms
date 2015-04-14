/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.reporter.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.reporter.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.dialogs.SelectUserDialog;

/**
 * User selection field editor
 */
public class UserFieldEditor extends FieldEditor
{
	private static final String EMPTY_SELECTION_TEXT = Messages.get().UserFieldEditor_None;
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

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContent(Composite parent)
	{
		Composite content = toolkit.createComposite(parent, SWT.BORDER);

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		content.setLayout(layout);

		text = new CLabel(content, SWT.NONE);
		toolkit.adapt(text);
		GridData gd = new GridData();
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
		
		return content;
	}

	/**
	 * 
	 */
	protected void selectUser()
	{
		final SelectUserDialog dialog = new SelectUserDialog(getShell(), User.class);
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
