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
import org.netxms.ui.eclipse.usermanager.Messages;

/**
 * "System Rights" property page for user object
 */
public class SystemRights extends PropertyPage
{
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
		
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_DELETE_ALARMS, Messages.get().SystemRights_DeleteAlarms);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_ACTIONS, Messages.get().SystemRights_ConfigureActions);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_EDIT_EVENT_DB, Messages.get().SystemRights_ConfigureEvents);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_DB, Messages.get().SystemRights_ViewEventConfig);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_SITUATIONS, Messages.get().SystemRights_ConfigureSituations);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_EPP, Messages.get().SystemRights_EditEPP);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_SCRIPTS, Messages.get().SystemRights_ManageScripts);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_TOOLS, Messages.get().SystemRights_ConfigureObjTools);
      addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS, Messages.get().SystemRights_ManageDCISummaryTables);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_CONFIGURE_TRAPS, Messages.get().SystemRights_ConfigureTraps);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_CFG, Messages.get().SystemRights_ManageAgents);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_PACKAGES, Messages.get().SystemRights_ManagePackages);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_LOG, Messages.get().SystemRights_ViewEventLog);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_VIEW_AUDIT_LOG, Messages.get().SystemRights_ViewAuditLog);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_VIEW_TRAP_LOG, Messages.get().SystemRights_ViewTrapLog);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_MAPPING_TBLS, Messages.get().SystemRights_ManageMappingTables);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_SERVER_CONFIG, Messages.get().SystemRights_EditServerConfig);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_READ_FILES, Messages.get().SystemRights_ReadFiles);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_FILES, Messages.get().SystemRights_ManageFiles);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_SERVER_CONSOLE, Messages.get().SystemRights_AccessConsole);
      addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_XMPP_COMMANDS, Messages.get().SystemRights_ExecuteXMPPCommands);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_SESSIONS, Messages.get().SystemRights_ControlSessions);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MANAGE_USERS, Messages.get().SystemRights_ManageUsers);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_SEND_SMS, Messages.get().SystemRights_SendSMS);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_REGISTER_AGENTS, Messages.get().SystemRights_RegisterAgents);
		addCheckbox(dialogArea, UserAccessRights.SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN, Messages.get().SystemRights_LoginAsMobile);
		
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
		
		new ConsoleJob(Messages.get().SystemRights_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				userManager.modifyUserDBObject(object, UserManager.USER_MODIFY_ACCESS_RIGHTS);
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().SystemRights_JobError;
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
