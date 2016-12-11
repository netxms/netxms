/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.ui.AbstractSourceProvider;
import org.eclipse.ui.ISources;
import org.eclipse.ui.services.IServiceLocator;
import org.netxms.client.constants.UserAccessRights;

/**
 * Source provider
 */
public class SourceProvider extends AbstractSourceProvider
{
   public static final String UA_ALL_SCHEDULED_TASKS   = "org.netxms.access.AllScheduledTasks"; //$NON-NLS-1$
   public static final String UA_CONFIGURE_TRAPS       = "org.netxms.access.ConfigureTraps"; //$NON-NLS-1$
   public static final String UA_DELETE_ALARMS         = "org.netxms.access.DeleteAlarms"; //$NON-NLS-1$
   public static final String UA_EDIT_EVENT_DB         = "org.netxms.access.EditEventConfiguration"; //$NON-NLS-1$
   public static final String UA_EPP                   = "org.netxms.access.EventProcessingPolicy"; //$NON-NLS-1$
   public static final String UA_MANAGE_ACTIONS        = "org.netxms.access.ManageActions"; //$NON-NLS-1$
   public static final String UA_MANAGE_AGENT_CFG      = "org.netxms.access.ManageAgentConfig"; //$NON-NLS-1$
   public static final String UA_MANAGE_SERVER_FILES   = "org.netxms.access.ManageServerFiles"; //$NON-NLS-1$
   public static final String UA_MANAGE_IMAGE_LIB      = "org.netxms.access.ManageImageLibray"; //$NON-NLS-1$
   public static final String UA_MANAGE_MAPPING_TBLS   = "org.netxms.access.ManageMappingTables"; //$NON-NLS-1$
   public static final String UA_MANAGE_PACKAGES       = "org.netxms.access.ManagePackages"; //$NON-NLS-1$
   public static final String UA_MANAGE_SCRIPTS        = "org.netxms.access.ManageScripts"; //$NON-NLS-1$
   public static final String UA_MANAGE_SESSIONS       = "org.netxms.access.ManageSessions"; //$NON-NLS-1$
   public static final String UA_MANAGE_PSTORAGE       = "org.netxms.access.ManagePStorage"; //$NON-NLS-1$
   public static final String UA_MANAGE_SUMMARY_TBLS   = "org.netxms.access.ManageSummaryTables"; //$NON-NLS-1$
   public static final String UA_MANAGE_TOOLS          = "org.netxms.access.ManageTools"; //$NON-NLS-1$
	public static final String UA_MANAGE_USERS          = "org.netxms.access.ManageUsers"; //$NON-NLS-1$
   public static final String UA_MOBILE_DEVICE_LOGIN   = "org.netxms.access.MobileDeviceLogin"; //$NON-NLS-1$
   public static final String UA_OWN_SCHEDULED_TASKS   = "org.netxms.access.OwnScheduledTasks"; //$NON-NLS-1$
   public static final String UA_READ_SERVER_FILES     = "org.netxms.access.ReadServerFiles"; //$NON-NLS-1$
   public static final String UA_REGISTER_AGENTS       = "org.netxms.access.RegisterAgents"; //$NON-NLS-1$
   public static final String UA_REPORTING_SERVER      = "org.netxms.access.ReportingServer"; //$NON-NLS-1$
   public static final String UA_SCHEDULE_FILE_UPLOAD  = "org.netxms.access.ScheduleFileUpload"; //$NON-NLS-1$
   public static final String UA_SCHEDULE_MAINTENANCE  = "org.netxms.access.ScheduleMaintenance"; //$NON-NLS-1$
   public static final String UA_SCHEDULE_SCRIPT       = "org.netxms.access.ScheduleScript"; //$NON-NLS-1$
   public static final String UA_SEND_SMS              = "org.netxms.access.SendSMS"; //$NON-NLS-1$
   public static final String UA_SERVER_CONFIG         = "org.netxms.access.ServerConfig"; //$NON-NLS-1$
   public static final String UA_SERVER_CONSOLE        = "org.netxms.access.ServerConsole"; //$NON-NLS-1$
   public static final String UA_UNLINK_ISSUES         = "org.netxms.access.UnlinkIssues"; //$NON-NLS-1$
   public static final String UA_USER_SCHEDULED_TASKS  = "org.netxms.access.UserScheduledTasks"; //$NON-NLS-1$
   public static final String UA_VIEW_AUDIT_LOG        = "org.netxms.access.ViewAuditLog"; //$NON-NLS-1$
   public static final String UA_VIEW_EVENT_DB         = "org.netxms.access.ViewEventConfiguration"; //$NON-NLS-1$
   public static final String UA_VIEW_EVENT_LOG        = "org.netxms.access.ViewEventLog"; //$NON-NLS-1$
   public static final String UA_VIEW_SYSLOG           = "org.netxms.access.ViewSyslog"; //$NON-NLS-1$
   public static final String UA_VIEW_TRAP_LOG         = "org.netxms.access.ViewTrapLog"; //$NON-NLS-1$
   public static final String UA_XMPP_COMMANDS         = "org.netxms.access.XMPPCommands"; //$NON-NLS-1$
	
	private static final String[] PROVIDED_SOURCE_NAMES =
	   { 
   	   UA_ALL_SCHEDULED_TASKS,
   	   UA_CONFIGURE_TRAPS,
   	   UA_DELETE_ALARMS,
   	   UA_EDIT_EVENT_DB,
   	   UA_EPP,
   	   UA_MANAGE_ACTIONS,
   	   UA_MANAGE_AGENT_CFG,
   	   UA_MANAGE_SERVER_FILES,
   	   UA_MANAGE_IMAGE_LIB,
   	   UA_MANAGE_MAPPING_TBLS,
   	   UA_MANAGE_PACKAGES,
   	   UA_MANAGE_SCRIPTS,
   	   UA_MANAGE_SESSIONS,
   	   UA_MANAGE_PSTORAGE,
   	   UA_MANAGE_SUMMARY_TBLS,
   	   UA_MANAGE_TOOLS,
   	   UA_MANAGE_USERS,
   	   UA_MOBILE_DEVICE_LOGIN,
   	   UA_OWN_SCHEDULED_TASKS,
   	   UA_READ_SERVER_FILES,
   	   UA_REGISTER_AGENTS,
   	   UA_REPORTING_SERVER,
   	   UA_SCHEDULE_FILE_UPLOAD,
   	   UA_SCHEDULE_MAINTENANCE,
   	   UA_SCHEDULE_SCRIPT,
   	   UA_SEND_SMS,
   	   UA_SERVER_CONFIG,
   	   UA_SERVER_CONSOLE,
   	   UA_UNLINK_ISSUES,
   	   UA_USER_SCHEDULED_TASKS,
   	   UA_VIEW_AUDIT_LOG,
   	   UA_VIEW_EVENT_DB,
   	   UA_VIEW_EVENT_LOG,
   	   UA_VIEW_SYSLOG,
   	   UA_VIEW_TRAP_LOG,
   	   UA_XMPP_COMMANDS
	   };
	private static final Map<String, Object> values = new HashMap<String, Object>(1);

	private static SourceProvider instance = null;
	
	/**
	 * Get source provider instance.
	 * 
	 * @return
	 */
	public static SourceProvider getInstance()
	{
		return instance;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.AbstractSourceProvider#initialize(org.eclipse.ui.services.IServiceLocator)
	 */
	@Override
	public void initialize(IServiceLocator locator)
	{
		super.initialize(locator);
		values.put(UA_MANAGE_USERS, true);
		instance = this;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISourceProvider#getCurrentState()
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Map getCurrentState()
	{
		return values;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISourceProvider#getProvidedSourceNames()
	 */
	@Override
	public String[] getProvidedSourceNames()
	{
		return PROVIDED_SOURCE_NAMES;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISourceProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}
	
	/**
	 * @param name
	 * @param value
	 */
	public void updateAccessRights(long rights)
	{
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_ALL_SCHEDULED_TASKS, UA_ALL_SCHEDULED_TASKS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_CONFIGURE_TRAPS, UA_CONFIGURE_TRAPS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_DELETE_ALARMS, UA_DELETE_ALARMS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_EDIT_EVENT_DB, UA_EDIT_EVENT_DB);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_EPP, UA_EPP);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_ACTIONS, UA_MANAGE_ACTIONS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_CFG, UA_MANAGE_AGENT_CFG);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_IMAGE_LIB, UA_MANAGE_IMAGE_LIB);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_MAPPING_TBLS, UA_MANAGE_MAPPING_TBLS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_PACKAGES, UA_MANAGE_PACKAGES);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_SCRIPTS, UA_MANAGE_SCRIPTS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_SERVER_FILES, UA_MANAGE_SERVER_FILES);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_SESSIONS, UA_MANAGE_SESSIONS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_PERSISTENT_STORAGE, UA_MANAGE_PSTORAGE);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS, UA_MANAGE_SUMMARY_TBLS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_TOOLS, UA_MANAGE_TOOLS);
	   setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MANAGE_USERS, UA_MANAGE_USERS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN, UA_MOBILE_DEVICE_LOGIN);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_OWN_SCHEDULED_TASKS, UA_OWN_SCHEDULED_TASKS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_READ_SERVER_FILES, UA_READ_SERVER_FILES);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_REGISTER_AGENTS, UA_REGISTER_AGENTS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_REPORTING_SERVER, UA_REPORTING_SERVER);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD, UA_SCHEDULE_FILE_UPLOAD);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_MAINTENANCE, UA_SCHEDULE_MAINTENANCE);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_SCRIPT, UA_SCHEDULE_SCRIPT);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_SEND_SMS, UA_SEND_SMS);
	   setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_SERVER_CONFIG, UA_SERVER_CONFIG);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_SERVER_CONSOLE, UA_SERVER_CONSOLE);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_UNLINK_ISSUES, UA_UNLINK_ISSUES);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_USER_SCHEDULED_TASKS, UA_USER_SCHEDULED_TASKS);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_VIEW_AUDIT_LOG, UA_VIEW_AUDIT_LOG);
	   setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_DB, UA_VIEW_EVENT_DB);
	   setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_LOG, UA_VIEW_EVENT_LOG);
      setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_VIEW_SYSLOG, UA_VIEW_SYSLOG);
	   setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_VIEW_TRAP_LOG, UA_VIEW_TRAP_LOG);
	   setAccessRight(rights, UserAccessRights.SYSTEM_ACCESS_XMPP_COMMANDS, UA_XMPP_COMMANDS);
		fireSourceChanged(ISources.WORKBENCH, getCurrentState());
	}
	
	/**
	 * @param rights
	 * @param bit
	 * @param name
	 */
	private void setAccessRight(long rights, long bit, final String name)
	{
	   values.put(name, (rights & bit) != 0);
	}
}
