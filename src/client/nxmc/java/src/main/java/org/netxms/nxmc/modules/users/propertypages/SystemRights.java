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
package org.netxms.nxmc.modules.users.propertypages;

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
import org.netxms.client.NXCSession;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "System Rights" property page for user object
 */
public class SystemRights extends PropertyPage
{
   private static I18n i18n = LocalizationHelper.getI18n(SystemRights.class);
	private NXCSession session;
	private AbstractUserObject object;
   private List<AccessAttribute> attributes = new ArrayList<AccessAttribute>();
   private String filterText = "";
   private FilterText filter;
   private CheckboxTableViewer viewer;

   /**
    * Default constructor
    */
   public SystemRights(AbstractUserObject user)
   {
      super(i18n.tr("System Rights"));
      session = Registry.getSession();
      object = user;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_ALL_SCHEDULED_TASKS, i18n.tr("Manage all scheduled tasks")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_AM_ATTRIBUTE_MANAGE, i18n.tr("Assert management attributes")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_CONFIGURE_TRAPS, i18n.tr("Configure SNMP traps")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_EDIT_EVENT_DB, i18n.tr("Configure event templates")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_EPP, i18n.tr("Edit event processing policy")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_EXTERNAL_INTEGRATION, "External tool integration account"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_IMPORT_CONFIGURATION, "Import configuration"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_2FA_METHODS, "Manage two-factor authentication methods"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_ACTIONS, i18n.tr("Configure server actions")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_CFG, i18n.tr("Manage agent configurations")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_GEO_AREAS, "Manage geographical areas"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_IMAGE_LIB, i18n.tr("Manage Image Library")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_MAPPING_TBLS, i18n.tr("Manage mapping tables")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_OBJECT_QUERIES, "Manage object queries"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_PACKAGES, i18n.tr("Manage packages")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_SCRIPTS, i18n.tr("Manage script library")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_SERVER_FILES, i18n.tr("Manage server files")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_SESSIONS, i18n.tr("Control user sessions")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS, i18n.tr("Manage DCI summary tables")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_TOOLS, i18n.tr("Configure object tools")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MANAGE_USERS, i18n.tr("Manage users")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN, i18n.tr("Login as mobile device")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_OBJECT_CATEGORIES, "Manage object categories"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_OWN_SCHEDULED_TASKS, i18n.tr("Manage own scheduled tasks")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_PERSISTENT_STORAGE, "Manage persistent storage"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_READ_SERVER_FILES, i18n.tr("Read server files")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_REGISTER_AGENTS, i18n.tr("Manage agent tunnels")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_REPORTING_SERVER, i18n.tr("Reporting server access")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD, i18n.tr("Schedule file upload"))); 
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SCHEDULE_MAINTENANCE, i18n.tr("Schedule object maintenance")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SCHEDULE_SCRIPT, i18n.tr("Schedule script execution")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SEND_NOTIFICATION, "Send notifications"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SERVER_CONFIG, i18n.tr("Edit server configuration variables")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SERVER_CONSOLE, i18n.tr("Access server console")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SETUP_TCP_PROXY, "Initiate TCP proxy sessions")); 
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_SSH_KEY_CONFIGURATION, "Manage SSH keys"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_UA_NOTIFICATIONS, "Manage user support application notifications")); 
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_UNLINK_ISSUES, i18n.tr("Unlink helpdesk tickets")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_USER_SCHEDULED_TASKS, i18n.tr("Manage user scheduled tasks")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_ALL_ALARMS, "View all alarm categories"));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_AUDIT_LOG, i18n.tr("View audit log")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_DB, i18n.tr("View event templates configuration")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_LOG, i18n.tr("View event log")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_SYSLOG, i18n.tr("View syslog")));
      attributes.add(new AccessAttribute(UserAccessRights.SYSTEM_ACCESS_VIEW_TRAP_LOG, i18n.tr("View SNMP trap log")));
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
	@Override
	protected boolean applyChanges(final boolean isApply)
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
		
		new Job(i18n.tr("Update user database object"), null) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyUserDBObject(object, AbstractUserObject.MODIFY_ACCESS_RIGHTS);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot update user object");
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
		
		return true;
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
