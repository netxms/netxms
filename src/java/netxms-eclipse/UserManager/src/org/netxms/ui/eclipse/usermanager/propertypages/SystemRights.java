/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CheckStateChangedEvent;
import org.eclipse.jface.viewers.CheckboxTableViewer;
import org.eclipse.jface.viewers.ICheckStateListener;
import org.eclipse.jface.viewers.ICheckStateProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.Messages;
import org.netxms.ui.eclipse.widgets.FilterText;

/**
 * "System Rights" property page for user object
 */
public class SystemRights extends PropertyPage
{
	private NXCSession session;
	private AbstractUserObject object;
   private List<AccessAttribute> attributes = new ArrayList<AccessAttribute>();
   private String filterText = "";
   private FilterText filter;
   private CheckboxTableViewer viewer;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		session = ConsoleSharedData.getSession();
      object = (AbstractUserObject)getElement().getAdapter(AbstractUserObject.class);

      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_ALL_SCHEDULED_TASKS, Messages.get().SystemRights_ManageAllScheduledTasks));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_AM_ATTRIBUTE_MANAGE, "Assert management attributes"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_CONFIGURE_TRAPS, Messages.get().SystemRights_ConfigureTraps));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_EDIT_EVENT_DB, Messages.get().SystemRights_ConfigureEvents));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_EPP, Messages.get().SystemRights_EditEPP));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_EXTERNAL_INTEGRATION, "External tool integration account"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_IMPORT_CONFIGURATION, "Import configuration"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_2FA_METHODS, "Manage two-factor authentication methods"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_ACTIONS, Messages.get().SystemRights_ConfigureActions));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_CFG, Messages.get().SystemRights_ManageAgents));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_GEO_AREAS, "Manage geographical areas"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_IMAGE_LIB, Messages.get().SystemRights_ManageImageLibrary));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_MAPPING_TBLS, Messages.get().SystemRights_ManageMappingTables));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_OBJECT_QUERIES, "Manage object queries"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_PACKAGES, Messages.get().SystemRights_ManagePackages));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_SCRIPTS, Messages.get().SystemRights_ManageScripts));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_SERVER_FILES, Messages.get().SystemRights_ManageFiles));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_SESSIONS, Messages.get().SystemRights_ControlSessions));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS, Messages.get().SystemRights_ManageDCISummaryTables));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_TOOLS, Messages.get().SystemRights_ConfigureObjTools));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_USERS, Messages.get().SystemRights_ManageUsers));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN, Messages.get().SystemRights_LoginAsMobile));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_OBJECT_CATEGORIES, "Manage object categories"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_OWN_SCHEDULED_TASKS, Messages.get().SystemRights_ManageOwnScheduledTasks));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_PERSISTENT_STORAGE, "Manage persistent storage"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_READ_SERVER_FILES, Messages.get().SystemRights_ReadFiles));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_REGISTER_AGENTS, Messages.get().SystemRights_RegisterAgents));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_REPORTING_SERVER, Messages.get().SystemRights_ReportingServerAccess));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD, Messages.get().SystemRights_ScheduleFileUploadTask)); 
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SCHEDULE_MAINTENANCE, Messages.get().SystemRights_ScheduleObjectMaint));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SCHEDULE_SCRIPT, Messages.get().SystemRights_ScheduleScriptTask));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SEND_NOTIFICATION, "Send notifications"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SERVER_CONFIG, Messages.get().SystemRights_EditServerConfig));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SERVER_CONSOLE, Messages.get().SystemRights_AccessConsole));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SETUP_TCP_PROXY, "Initiate TCP proxy sessions")); 
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SSH_KEY_CONFIGURATION, "Manage SSH keys"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_UA_NOTIFICATIONS, "Manage user support application notifications")); 
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_UNLINK_ISSUES, Messages.get().SystemRights_UnlinkTicket));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_USER_SCHEDULED_TASKS, Messages.get().SystemRights_ManageUserScheduledTasks));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_ALL_ALARMS, "View all alarm categories"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_AUDIT_LOG, Messages.get().SystemRights_ViewAuditLog));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_DB, Messages.get().SystemRights_ViewEventConfig));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_LOG, Messages.get().SystemRights_ViewEventLog));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_SYSLOG, Messages.get().SystemRights_ViewSyslog));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_TRAP_LOG, Messages.get().SystemRights_ViewTrapLog));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS, "Manage web service definitions"));

      Composite dialogArea = new Composite(parent, SWT.BORDER);
      dialogArea.setBackground(parent.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

		GridLayout layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);

      filter = new FilterText(dialogArea, SWT.NONE, null, false, false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      filter.setLayoutData(gd);
      filter.setBackground(parent.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
      filter.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filterText = filter.getText();
            viewer.refresh();
         }
      });

      viewer = CheckboxTableViewer.newCheckList(dialogArea, SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 400;
      viewer.getControl().setLayoutData(gd);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setCheckStateProvider(new ICheckStateProvider() {
         @Override
         public boolean isGrayed(Object element)
         {
            return false;
         }

         @Override
         public boolean isChecked(Object element)
         {
            return ((AccessAttribute)element).selected;
         }
      });
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((AccessAttribute)element).displayName;
         }
      });
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((AccessAttribute)e1).comparatorName.compareTo(((AccessAttribute)e2).comparatorName);
         }
      });
      viewer.addFilter(new ViewerFilter() {
         @Override
         public boolean select(Viewer viewer, Object parentElement, Object element)
         {
            return filterText.isEmpty() ? true : ((AccessAttribute)element).comparatorName.contains(filterText);
         }
      });
      viewer.addCheckStateListener(new ICheckStateListener() {
         @Override
         public void checkStateChanged(CheckStateChangedEvent event)
         {
            ((AccessAttribute)event.getElement()).selected = event.getChecked();
         }
      });
      viewer.setInput(attributes);

		return dialogArea;
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
		
		long systemRights = 0;
      for(AccessAttribute a : attributes)
      {
         if (a.selected)
            systemRights |= a.value;
      }
		object.setSystemRights(systemRights);
		
		new ConsoleJob(Messages.get().SystemRights_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyUserDBObject(object, AbstractUserObject.MODIFY_ACCESS_RIGHTS);
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

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

   /**
    * Access attribute
    */
   private class AccessAttribute
   {
      String displayName;
      String comparatorName;
      long value;
      boolean selected;

      public AccessAttribute(long value, String name)
      {
         this.displayName = name;
         this.comparatorName = name.toLowerCase();
         this.value = value;
         this.selected = (object.getSystemRights() & value) != 0;
      }
   }
}
