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
package org.netxms.nxmc.modules.users.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.UserAuthenticationMethod;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyDialog;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.actions.GenerateObjectAccessReportAction;
import org.netxms.nxmc.modules.users.dialogs.ChangePasswordDialog;
import org.netxms.nxmc.modules.users.dialogs.CreateUserOrGroupDialog;
import org.netxms.nxmc.modules.users.dialogs.IssueTokenDialog;
import org.netxms.nxmc.modules.users.propertypages.Authentication;
import org.netxms.nxmc.modules.users.propertypages.CustomAttributes;
import org.netxms.nxmc.modules.users.propertypages.General;
import org.netxms.nxmc.modules.users.propertypages.GroupMembership;
import org.netxms.nxmc.modules.users.propertypages.Members;
import org.netxms.nxmc.modules.users.propertypages.SystemRights;
import org.netxms.nxmc.modules.users.propertypages.UIAccessRules;
import org.netxms.nxmc.modules.users.views.helpers.DecoratingUserLabelProvider;
import org.netxms.nxmc.modules.users.views.helpers.UserComparator;
import org.netxms.nxmc.modules.users.views.helpers.UserFilter;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * User management view
 */
public class UserManagementView extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(UserManagementView.class);
   private static final String ID = "configuration.user-manager";

	// Columns
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_TYPE = 1;
	public static final int COLUMN_FULLNAME = 2;
	public static final int COLUMN_DESCRIPTION = 3;
   public static final int COLUMN_SOURCE = 4;
   public static final int COLUMN_AUTH_METHOD = 5;
	public static final int COLUMN_GUID = 6;
   public static final int COLUMN_LDAP_DN = 7;
   public static final int COLUMN_LAST_LOGIN = 8;
   public static final int COLUMN_CREATED = 9;

   private SortableTableViewer viewer;
	private NXCSession session;
	private SessionListener sessionListener;
	private boolean editNewUser = false;
	private Action actionAddUser;
	private Action actionAddGroup;
	private Action actionEditUser;
	private Action actionDeleteUser;
	private Action actionChangePassword;
   private Action actionIssueToken;
	private Action actionEnable;
	private Action actionDisable;
	private Action actionDetachUserFromLDAP;
   private Action actionGenerateAccessReport;
   private UserFilter filter;

   /**
    * Create user management view
    */
   public UserManagementView()
   {
      super(LocalizationHelper.getI18n(UserManagementView.class).tr("Users and Groups"), ResourceManager.getImageDescriptor("icons/config-views/user_manager.png"), ID, true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   protected void createContent(Composite parent)
	{
		final String[] names = { 
		      i18n.tr("Name"), 
		      i18n.tr("Type"), 
		      i18n.tr("Full name"), 
		      i18n.tr("Description"), 
		      i18n.tr("Source"), 
		      i18n.tr("Authentication"), 
		      i18n.tr("GUID"),
		      i18n.tr("LDAP DN"),
		      i18n.tr("Last login"),
		      i18n.tr("Created")
		   };
		final int[] widths = { 100, 80, 180, 250, 80, 170, 250, 400, 250, 250 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		DecoratingUserLabelProvider labelProvider = new DecoratingUserLabelProvider();
		viewer.setLabelProvider(labelProvider);
		viewer.setComparator(new UserComparator());
      filter = new UserFilter(labelProvider);
      viewer.addFilter(filter);
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionEditUser.setEnabled(selection.size() == 1);
               boolean singleUser = (selection.size() == 1) && (selection.getFirstElement() instanceof User);
               actionChangePassword.setEnabled(singleUser);
               actionIssueToken.setEnabled(singleUser);
					actionDeleteUser.setEnabled(selection.size() > 0);
				}
			}
		});
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				actionEditUser.run();
			}
		});

      createActions();
		createContextMenu();

		// Listener for server's notifications
		sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(final SessionNotification n)
			{
				if (n.getCode() == SessionNotification.USER_DB_CHANGED)
				{
					viewer.getControl().getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							viewer.setInput(session.getUserDatabaseObjects());
							if (editNewUser && (n.getSubCode() == SessionNotification.USER_DB_OBJECT_CREATED))
							{
								editNewUser = false;
								viewer.setSelection(new StructuredSelection(n.getObject()), true);
								actionEditUser.run();
							}
						}
					});
				}
			}
		};
      session.addListener(sessionListener);      
      viewer.setInput(session.getUserDatabaseObjects());

		getUsersAndRefresh();
	}

   /**
    * Get user info and refresh view
    */
   private void getUsersAndRefresh()
   {
      if (session.isUserDatabaseSynchronized())
         return;

      Job job = new Job(i18n.tr("Synchronizing users"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.syncUserDatabase();
            runInUIThread(() -> viewer.setInput(session.getUserDatabaseObjects()));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot synchronize user database");
         }
      };
      job.setUser(false);
      job.start();  
   }

	/**
	 * Create actions
	 */
   private void createActions()
	{
		actionAddUser = new Action(i18n.tr("Create new &user..."), ResourceManager.getImageDescriptor("icons/user_add.png")) {
			@Override
			public void run()
			{
				addUser();
			}
		};

      actionAddGroup = new Action(i18n.tr("Create new &group..."), ResourceManager.getImageDescriptor("icons/group_add.png")) {
			@Override
			public void run()
			{
				addGroup();
			}
		};

      actionEditUser = new Action(i18n.tr("&Properties..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            final IStructuredSelection selection = viewer.getStructuredSelection();
            if (selection.isEmpty())
               return;

            AbstractUserObject userObject = (AbstractUserObject)selection.getFirstElement();

            PreferenceManager pm = new PreferenceManager();
            pm.addToRoot(new PreferenceNode("general", new General(userObject, UserManagementView.this)));
            if (userObject instanceof User)
            {
               pm.addToRoot(new PreferenceNode("authentication", new Authentication((User)userObject, UserManagementView.this)));
               pm.addToRoot(new PreferenceNode("group-membership", new GroupMembership((User)userObject, UserManagementView.this)));
            }
            else if (userObject instanceof UserGroup)
            {
               pm.addToRoot(new PreferenceNode("members", new Members((UserGroup)userObject, UserManagementView.this)));
            }
            pm.addToRoot(new PreferenceNode("system-rights", new SystemRights(userObject, UserManagementView.this)));
            pm.addToRoot(new PreferenceNode("ui-access-rules", new UIAccessRules(userObject, UserManagementView.this)));
            pm.addToRoot(new PreferenceNode("custom-attributes", new CustomAttributes(userObject, UserManagementView.this)));

            PropertyDialog dlg = new PropertyDialog(getWindow().getShell(), pm, String.format(i18n.tr("Properties for %s"), userObject.getLabel()));
            dlg.open();
         }
      };
      actionEditUser.setEnabled(false);

      actionDeleteUser = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteUser();
			}
		};
		actionDeleteUser.setEnabled(false);

      actionChangePassword = new Action(i18n.tr("Change &password..."), ResourceManager.getImageDescriptor("icons/change_password.png")) {
			@Override
			public void run()
			{
				changePassword();
			}
		};
		actionChangePassword.setEnabled(false);
		
      actionIssueToken = new Action(i18n.tr("Issue authentication &token...")) {
         @Override
         public void run()
         {
            issueToken();
         }
      };
      actionIssueToken.setEnabled(false);

      actionEnable = new Action(i18n.tr("Enable")) {
         @Override
         public void run()
         {
            enableUser();
         }
      };      
    
      actionDisable = new Action(i18n.tr("Disable")) {
         @Override
         public void run()
         {
            disableUser();
         }
      };
      
      actionDetachUserFromLDAP = new Action(i18n.tr("Detach from LDAP")) {
         @Override
         public void run()
         {
            detachLDAPUser();
         }
      };

      actionGenerateAccessReport = new GenerateObjectAccessReportAction(i18n.tr("Generate object access report..."), this);
	}

	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionGenerateAccessReport);
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionGenerateAccessReport);
      super.fillLocalMenu(manager);
   }

   /**
    * Create contex menu for user list
    */
	private void createContextMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
      final IStructuredSelection selection = viewer.getStructuredSelection();

      mgr.add(actionAddUser);
      mgr.add(actionAddGroup);
      mgr.add(new Separator());

	   boolean containDisabled = false;
	   boolean containEnabled = false;
	   boolean containLDAP = false;
      for(Object object : selection.toList())
      {
         if (((AbstractUserObject)object).isDisabled())
            containDisabled = true;
         if (!((AbstractUserObject)object).isDisabled())
            containEnabled = true;
         if ((((AbstractUserObject)object).getFlags() & AbstractUserObject.LDAP_USER) != 0)
            containLDAP = true;
      }
      if (containDisabled)
         mgr.add(actionEnable);
      if (containEnabled)
         mgr.add(actionDisable);
      if (containLDAP)
         mgr.add(actionDetachUserFromLDAP);

      if ((selection.size() == 1) && (selection.getFirstElement() instanceof User))
		{
         UserAuthenticationMethod method = ((User)selection.getFirstElement()).getAuthMethod();
         if ((method == UserAuthenticationMethod.LOCAL) || (method == UserAuthenticationMethod.CERTIFICATE_OR_LOCAL))
            mgr.add(actionChangePassword);
         mgr.add(actionIssueToken);
		}

		mgr.add(actionDeleteUser);
		mgr.add(new Separator());
		mgr.add(actionEditUser);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
	@Override
	public void dispose()
	{
		if (sessionListener != null)
			session.removeListener(sessionListener);
		super.dispose();
	}

	/**
	 * Add new user
	 */
	private void addUser()
	{
      final CreateUserOrGroupDialog dlg = new CreateUserOrGroupDialog(getWindow().getShell(), true);
		if (dlg.open() == Window.OK)
		{
         new Job(i18n.tr("Creating new user"), this) {
				@Override
            protected void run(IProgressMonitor monitor) throws Exception
				{
					editNewUser = dlg.isEditAfterCreate();
					session.createUser(dlg.getLoginName());
				}

				@Override
				protected String getErrorMessage()
				{
               return i18n.tr("Cannot create user");
				}
			}.start();
		}
	}

	/**
	 * Add new group.
	 */
	private void addGroup()
	{
      final CreateUserOrGroupDialog dlg = new CreateUserOrGroupDialog(getWindow().getShell(), false);
		if (dlg.open() == Window.OK)
		{
         new Job(i18n.tr("Create user group"), this) {
				@Override
            protected void run(IProgressMonitor monitor) throws Exception
				{
					editNewUser = dlg.isEditAfterCreate();
					session.createUserGroup(dlg.getLoginName());
				}

				@Override
				protected String getErrorMessage()
				{
               return i18n.tr("Cannot create user group");
				}
			}.start();
		}
	}

	/**
	 * Delete user or group
	 */
	private void deleteUser()
	{
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final String message = (selection.size() == 1) ? i18n.tr("Do you really want to delete selected user?") : i18n.tr("Do you really want to delete selected users?");
      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Confirm user deletion"), message))
			return;

      new Job(i18n.tr("Delete user"), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				for(Object object : selection.toList())
				{
					session.deleteUserDBObject(((AbstractUserObject)object).getId());
				}
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot delete user");
			}
		}.start();
	}

	/**
    * Enable user/group
    */
   private void enableUser()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final List<AbstractUserObject> objects = new ArrayList<>();
      for(Object o : selection.toList())
         objects.add((AbstractUserObject)o);

      new Job(i18n.tr("Enable user"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(AbstractUserObject object : objects)
            {
               object.setFlags(object.getFlags() & ~AbstractUserObject.DISABLED);
               session.modifyUserDBObject(object, AbstractUserObject.MODIFY_FLAGS);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot enable user");
         }
      }.start();
   }

   /**
    * Disable user/group
    */
   private void disableUser()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;
      
      final List<AbstractUserObject> objects = new ArrayList<>();
      for(Object o : selection.toList())
         objects.add((AbstractUserObject)o);

      new Job(i18n.tr("Disable user"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(AbstractUserObject object : objects)
            {
               object.setFlags(object.getFlags() | AbstractUserObject.DISABLED);
               session.modifyUserDBObject(object, AbstractUserObject.MODIFY_FLAGS);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot disable user");
         }
      }.start();
   }

   /**
    * Change password
    */
   private void changePassword()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty() || !(selection.getFirstElement() instanceof User))
         return;

      final User user = (User)selection.getFirstElement();
      final ChangePasswordDialog dialog = new ChangePasswordDialog(getWindow().getShell(), user.getId() == session.getUserId());
      if (dialog.open() != Window.OK)
         return;

      new Job(i18n.tr("Change password"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.setUserPassword(user.getId(), dialog.getPassword(), dialog.getOldPassword());
            runInUIThread(() -> addMessage(MessageArea.SUCCESS, i18n.tr("Successfully set password for user {0}", user.getName())));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot change password for user {0}", user.getName());
         }
      }.start();
   }

   /**
    * Issue authentication token to user
    */
   private void issueToken()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty() || !(selection.getFirstElement() instanceof User))
         return;

      final User user = (User)selection.getFirstElement();
      new IssueTokenDialog(getWindow().getShell(), user.getId()).open();
   }

   /**
    * Set user/group to non LDAP 
    */
   private void detachLDAPUser()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      new Job(i18n.tr("Detach LDAP user"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object object : selection.toList())
            {                
               ((AbstractUserObject)object).setFlags(((AbstractUserObject)object).getFlags() & ~AbstractUserObject.LDAP_USER);
               session.detachUserFromLdap(((AbstractUserObject)object).getId());
            }
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  changePassword();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot detach LDAP user");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#onFilterModify()
    */
   @Override
   protected void onFilterModify()
   {
      final String text = getFilterText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}
