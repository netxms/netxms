/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.usermanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.api.client.Session;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.api.client.users.User;
import org.netxms.api.client.users.UserManager;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * User's "general" property page
 */
public class General extends PropertyPage
{
	private Text textName;
	private Text textFullName;
	private Text textDescription;
	private Text textXmppId;
	private String initialName;
	private String initialFullName;
	private String initialDescription;
   private String initialXmppId;
	private AbstractUserObject object;
	private Session session;
	
	/**
	 * Default constructor
	 */
	public General()
	{
		super();
		session = ConsoleSharedData.getSession();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (AbstractUserObject)getElement().getAdapter(AbstractUserObject.class);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);
      
      // Object ID
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, Messages.get().General_ObjectID,
                                     Long.toString(object.getId()), WidgetHelper.DEFAULT_LAYOUT_DATA);
      
		// Object name
      initialName = new String(object.getName());
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, Messages.get().General_LoginName,
      		                                    initialName, WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		// Full name
      if (object instanceof User)
      {
	      initialFullName = new String(((User)object).getFullName());
	      textFullName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, Messages.get().General_FullName,
	      		                                        initialFullName, WidgetHelper.DEFAULT_LAYOUT_DATA);

         initialXmppId = new String(((User)object).getXmppId());
         textXmppId = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, "XMPP ID",
                                                       initialXmppId, WidgetHelper.DEFAULT_LAYOUT_DATA);
      }
      else
      {
      	initialFullName = ""; //$NON-NLS-1$
      	initialXmppId = "";
      }
      
		// Description
      initialDescription = new String(object.getDescription());
      textDescription = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT,
                                                       Messages.get().General_Description, initialDescription, WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		return dialogArea;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		final String newName = new String(textName.getText());
		final String newDescription = new String(textDescription.getText());
		final String newFullName = (object instanceof User) ? textFullName.getText() : ""; //$NON-NLS-1$
      final String newXmppId = (object instanceof User) ? textXmppId.getText() : ""; //$NON-NLS-1$
		
		if (newName.equals(initialName) && 
		    newDescription.equals(initialDescription) &&
		    newFullName.equals(initialFullName) &&
		    newXmppId.equals(initialXmppId))
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		new ConsoleJob(Messages.get().General_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				initialName = newName;
				initialFullName = newFullName;
				initialDescription = newDescription;
				initialXmppId = newXmppId;
				
				int fields = UserManager.USER_MODIFY_LOGIN_NAME | UserManager.USER_MODIFY_DESCRIPTION;
				object.setName(newName);
				object.setDescription(newDescription);
				if (object instanceof User)
				{
					((User)object).setFullName(newFullName);
               ((User)object).setXmppId(newXmppId);
					fields |= UserManager.USER_MODIFY_FULL_NAME | UserManager.USER_MODIFY_XMPP_ID;
				}
				((UserManager)session).modifyUserDBObject(object, fields);
			}
			
			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							General.this.setValid(true);
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().General_JobError;
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
