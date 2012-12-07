/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.api.client.constants.UserAccessRights;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.api.client.users.UserManager;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.usermanager.Activator;

/**
 * "System Rights" property page for user object
 */
public class SystemRights extends PropertyPage
{
	private static final long serialVersionUID = 1L;

	private UserManager userManager;
	private AbstractUserObject object;
	private Map<Integer, Button> buttons = new HashMap<Integer, Button>();
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		userManager = (UserManager)ConsoleSharedData.getSession();
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		object = (AbstractUserObject)getElement().getAdapter(AbstractUserObject.class);

		GridLayout layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);
		
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_DELETE_ALARMS, "Delete alarms");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_ACTIONS, "Configure server actions");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_EDIT_EVENT_DB, "Configure event templates");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_DB, "View event templates configuration");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_SITUATIONS, "Configure situations");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_EPP, "Edit event processing policy");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_SCRIPTS, "Manage script library");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_TOOLS, "Configure object tools");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_CONFIGURE_TRAPS, "Configure SNMP traps");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_CFG, "Manage agent configurations");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_PACKAGES, "Manage packages");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_LOG, "View event log");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_VIEW_AUDIT_LOG, "View audit log");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_VIEW_TRAP_LOG, "View SNMP trap log");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_SERVER_CONFIG, "Edit server configuration variables");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_READ_FILES, "Read server files");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_FILES, "Manage server files");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_SERVER_CONSOLE, "Access server console");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_SESSIONS, "Control user sessions");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_USERS, "Manage users");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_SEND_SMS, "Send SMS");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_REGISTER_AGENTS, "Register agents");
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN, "Login as mobile device");
		
		return dialogArea;
	}
	
	/**
	 * Add checkbox for given access right
	 * 
	 * @param parent parent conposite
	 * @param access access right
	 * @param name name
	 */
	private void addCheckbox(Composite parent, int access, String name)
	{
		Button b = new Button(parent, SWT.CHECK);
		b.setText(name);
		b.setSelection((object.getSystemRights() & access) != 0);
		buttons.put(access, b);
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		int systemRights = 0;
		for(Entry<Integer, Button> e : buttons.entrySet())
			if (e.getValue().getSelection())
				systemRights |= e.getKey();
		
		object.setSystemRights(systemRights);
		
		new ConsoleJob("Update user database object", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				userManager.modifyUserDBObject(object, UserManager.USER_MODIFY_ACCESS_RIGHTS);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update user object";
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
							SystemRights.this.setValid(true);
						}
					});
				}
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
