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
package org.netxms.nxmc.modules.reporting.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.dialogs.UserSelectionDialog;
import org.netxms.nxmc.modules.users.views.helpers.DecoratingUserLabelProvider;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * User selection field editor
 */
public class UserFieldEditor extends ReportFieldEditor
{
   private I18n i18n;
   private DecoratingUserLabelProvider labelProvider;
	private CLabel text;
	private boolean returnName;
	private AbstractUserObject user;
   private String emptySelectionText;

   public UserFieldEditor(ReportParameter parameter, Composite parent, boolean returnName)
	{
      super(parameter, parent);
		this.returnName = returnName;
      labelProvider = new DecoratingUserLabelProvider();
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelProvider.dispose();
			}
		});
      emptySelectionText = i18n.tr("None");
	}

   /**
    * @see org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor#setupLocalization()
    */
   @Override
   protected void setupLocalization()
   {
      i18n = LocalizationHelper.getI18n(UserFieldEditor.class);
   }

   /**
    * @see org.netxms.ReportFieldEditor.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContent(Composite parent)
	{
      Composite content = new Composite(parent, SWT.BORDER);

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		content.setLayout(layout);

		text = new CLabel(content, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		text.setLayoutData(gd);
		text.setText(emptySelectionText);

      final ImageHyperlink selectionLink = new ImageHyperlink(content, SWT.NONE);
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
		final UserSelectionDialog dialog = new UserSelectionDialog(getShell(), User.class);
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
				text.setText(emptySelectionText);
				text.setImage(null);
			}
		}
	}

   /**
    * @see org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor#getValue()
    */
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
