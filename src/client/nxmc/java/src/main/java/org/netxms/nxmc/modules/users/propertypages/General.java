/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.users.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * User's "general" property page
 */
public class General extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(General.class);

	private Text textName;
	private Text textFullName;
	private Text textDescription;
   private Text textEmail;
   private Text textPhoneNumber;
	private String initialName;
	private String initialFullName;
	private String initialDescription;
   private String initialEmail;
   private String initialPhoneNumber;
	private AbstractUserObject object;
	private NXCSession session;

	/**
	 * Default constructor
	 */
   public General(AbstractUserObject user, MessageAreaHolder messageArea)
	{
      super(LocalizationHelper.getI18n(General.class).tr("General"), messageArea);
		session = Registry.getSession();
		object = user;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);
      
      // Object ID
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, i18n.tr("Object ID"),
                                     Long.toString(object.getId()), WidgetHelper.DEFAULT_LAYOUT_DATA);
      
		// Object name
      initialName = new String(object.getName());
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Login name"),
      		                                    initialName, WidgetHelper.DEFAULT_LAYOUT_DATA);
		
      if (object instanceof User)
      {
	      initialFullName = new String(((User)object).getFullName());
	      textFullName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Full name"),
	      		                                        initialFullName, WidgetHelper.DEFAULT_LAYOUT_DATA);

         initialEmail = new String(((User)object).getEmail());
         textEmail = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, "Email", initialEmail, WidgetHelper.DEFAULT_LAYOUT_DATA);

         initialPhoneNumber = new String(((User)object).getPhoneNumber());
         textPhoneNumber = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, "Phone number",
               initialPhoneNumber, WidgetHelper.DEFAULT_LAYOUT_DATA);

      }
      else
      {
         initialFullName = "";
         initialEmail = "";
         initialPhoneNumber = "";
      }

		// Description
      initialDescription = new String(object.getDescription());
      textDescription = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT,
            i18n.tr("Description"), initialDescription, WidgetHelper.DEFAULT_LAYOUT_DATA);

		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	@Override
	protected boolean applyChanges(final boolean isApply)
	{
		final String newName = new String(textName.getText());
		final String newDescription = new String(textDescription.getText());
      final String newFullName = (object instanceof User) ? textFullName.getText() : "";
      final String newEmail = (object instanceof User) ? textEmail.getText() : "";
      final String newPhoneNumber = (object instanceof User) ? textPhoneNumber.getText() : "";
		
		if (newName.equals(initialName) && 
		    newDescription.equals(initialDescription) &&
		    newFullName.equals(initialFullName) &&
          newEmail.equals(initialEmail) &&
          newPhoneNumber.equals(initialPhoneNumber))
			return true;		// Nothing to apply

      if (isApply)
      {
         setMessage(null);
			setValid(false);
      }

      new Job(i18n.tr("Update user database object"), null, getMessageArea(isApply)) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				initialName = newName;
				initialFullName = newFullName;
				initialDescription = newDescription;
            initialEmail = newEmail;
            initialPhoneNumber = newPhoneNumber;

				int fields = AbstractUserObject.MODIFY_LOGIN_NAME | AbstractUserObject.MODIFY_DESCRIPTION;
				object.setName(newName);
				object.setDescription(newDescription);
				if (object instanceof User)
				{
					((User)object).setFullName(newFullName);
               ((User)object).setEmail(newEmail);
               ((User)object).setPhoneNumber(newPhoneNumber);
               fields |= AbstractUserObject.MODIFY_FULL_NAME | AbstractUserObject.MODIFY_EMAIL |
                     AbstractUserObject.MODIFY_PHONE_NUMBER;
				}
				session.modifyUserDBObject(object, fields);
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> General.this.setValid(true));
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot update user account");
			}
		}.start();

		return true;
	}
}
