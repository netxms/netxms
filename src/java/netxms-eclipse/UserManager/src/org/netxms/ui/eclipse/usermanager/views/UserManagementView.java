/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.usermanager.views;

import java.util.Iterator;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.NetXMSClientException;
import org.netxms.api.client.Session;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.api.client.constants.CommonRCC;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.api.client.users.User;
import org.netxms.api.client.users.UserManager;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.UserComparator;
import org.netxms.ui.eclipse.usermanager.UserLabelProvider;
import org.netxms.ui.eclipse.usermanager.dialogs.ChangePasswordDialog;
import org.netxms.ui.eclipse.usermanager.dialogs.CreateObjectDialog;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * User management view
 * 
 */
public class UserManagementView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.usermanager.view.user_manager";
	public static final String JOB_FAMILY = "UserManagerJob";

	// Columns
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_TYPE = 1;
	public static final int COLUMN_FULLNAME = 2;
	public static final int COLUMN_DESCRIPTION = 3;
	public static final int COLUMN_GUID = 4;

	private TableViewer viewer;
	private Session session;
	private UserManager userManager;
	private SessionListener sessionListener;
	private boolean databaseLocked = false;
	private boolean editNewUser = false;
	private Action actionAddUser;
	private Action actionAddGroup;
	private Action actionEditUser;
	private Action actionDeleteUser;
	private Action actionChangePassword;
	private RefreshAction actionRefresh;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = ConsoleSharedData.getSession();
		userManager = (UserManager)ConsoleSharedData.getSession();

		final String[] names = { "Name", "Type", "Full Name", "Description", "GUID" };
		final int[] widths = { 100, 80, 180, 250, 250 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new UserLabelProvider());
		viewer.setComparator(new UserComparator());
		viewer.addSelectionChangedListener(new ISelectionChangedListener()
		{
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionEditUser.setEnabled(selection.size() == 1);
					actionChangePassword.setEnabled((selection.size() == 1) && (selection.getFirstElement() instanceof User));
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

		makeActions();
		contributeToActionBars();
		createPopupMenu();

		// Listener for server's notifications
		sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(final SessionNotification n)
			{
				if (n.getCode() == SessionNotification.USER_DB_CHANGED)
				{
					new UIJob("Update user list")
					{
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							viewer.setInput(userManager.getUserDatabaseObjects());
							if (editNewUser && (n.getSubCode() == SessionNotification.USER_DB_OBJECT_CREATED))
							{
								editNewUser = false;
								viewer.setSelection(new StructuredSelection(n.getObject()), true);
								actionEditUser.run();
							}
							return Status.OK_STATUS;
						}
					}.schedule();
				}
			}
		};

		// Request server to lock user database, and on success refresh view
		Job job = new Job("Open user database")
		{
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;

				try
				{
					userManager.lockUserDatabase();
					databaseLocked = true;
					new UIJob("Update user list")
					{
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							viewer.setInput(userManager.getUserDatabaseObjects());
							return Status.OK_STATUS;
						}
					}.schedule();
					session.addListener(sessionListener);
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, (e instanceof NetXMSClientException) ? ((NetXMSClientException) e)
							.getErrorCode() : 0, "Cannot lock user database: " + e.getMessage(), null);
					new UIJob("Close user manager")
					{
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							UserManagementView.this.getViewSite().getPage().hideView(UserManagementView.this);
							return Status.OK_STATUS;
						}
					}.schedule();
				}
				return status;
			}
		};
		IWorkbenchSiteProgressService siteService = (IWorkbenchSiteProgressService) getSite().getAdapter(
				IWorkbenchSiteProgressService.class);
		siteService.schedule(job, 0, true);
	}

	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionAddUser);
		manager.add(actionAddGroup);
		manager.add(actionChangePassword);
		manager.add(actionDeleteUser);
		manager.add(actionEditUser);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionAddUser);
		manager.add(actionAddGroup);
		manager.add(actionDeleteUser);
		manager.add(actionEditUser);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Create actions
	 */
	private void makeActions()
	{
		actionRefresh = new RefreshAction()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				viewer.setInput(userManager.getUserDatabaseObjects());
			}
		};

		actionAddUser = new Action()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				addUser();
			}
		};
		actionAddUser.setText("Create new &user");
		actionAddUser.setImageDescriptor(Activator.getImageDescriptor("icons/user_add.png"));

		actionAddGroup = new Action()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				addGroup();
			}
		};
		actionAddGroup.setText("Create new &group");
		actionAddGroup.setImageDescriptor(Activator.getImageDescriptor("icons/group_add.png"));

		actionEditUser = new PropertyDialogAction(getSite(), viewer);
		actionEditUser.setText("&Properties");
		actionEditUser.setImageDescriptor(Activator.getImageDescriptor("icons/user_edit.png"));
		actionEditUser.setEnabled(false);

		actionDeleteUser = new Action()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				deleteUser();
			}
		};
		actionDeleteUser.setText("&Delete");
		actionDeleteUser.setImageDescriptor(Activator.getImageDescriptor("icons/user_delete.png"));
		actionDeleteUser.setEnabled(false);

		actionChangePassword = new Action()
		{
			@Override
			public void run()
			{
				final IStructuredSelection selection = (IStructuredSelection) viewer.getSelection();

				final Object firstElement = selection.getFirstElement();
				if (firstElement instanceof User)
				{
					User user = (User)firstElement;
					final ChangePasswordDialog dialog = new ChangePasswordDialog(getSite().getShell(), user.getId() == session.getUserId());
					if (dialog.open() == Window.OK)
					{
						try
						{
							userManager.setUserPassword(user.getId(), dialog.getPassword(), dialog.getOldPassword());
						}
						catch(Exception e)
						{
							MessageDialog.openError(getSite().getShell(), "Unable to change password", e.getMessage());
						}
					}
				}
			}
		};
		actionChangePassword.setText("Change Password");
		actionChangePassword.setImageDescriptor(Activator.getImageDescriptor("icons/change_password.png"));
		actionChangePassword.setEnabled(false);
	}

	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
		mgr.add(actionAddUser);
		mgr.add(actionAddGroup);

		final IStructuredSelection selection = (IStructuredSelection) viewer.getSelection();
		final Object firstElement = selection.getFirstElement();
		if (firstElement instanceof User)
		{
			mgr.add(actionChangePassword);
		}

		mgr.add(actionDeleteUser);
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		mgr.add(actionEditUser);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if (sessionListener != null)
			session.removeListener(sessionListener);
		if (databaseLocked)
		{
			new Job("Unlock user database")
			{
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;

					try
					{
						userManager.unlockUserDatabase();
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID,
								(e instanceof NetXMSClientException) ? ((NetXMSClientException) e).getErrorCode() : 0,
								"Cannot unlock user database: " + e.getMessage(), null);
					}
					return status;
				}
			}.schedule();
		}
		super.dispose();
	}
	
	/**
	 * Add new user
	 */
	private void addUser()
	{
		final CreateObjectDialog dlg = new CreateObjectDialog(getViewSite().getShell(), true);
		if (dlg.open() == Window.OK)
		{
			Job job = new Job("Create user")
			{
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;

					try
					{
						editNewUser = dlg.isEditAfterCreate();
						userManager.createUser(dlg.getLoginName());
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID,
								(e instanceof NetXMSClientException) ? ((NetXMSClientException) e).getErrorCode() : 0,
								"Cannot create user: " + e.getMessage(), null);
					}
					return status;
				}

				/* (non-Javadoc)
				 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
				 */
				@Override
				public boolean belongsTo(Object family)
				{
					return family == UserManagementView.JOB_FAMILY;
				}
			};
			IWorkbenchSiteProgressService siteService = (IWorkbenchSiteProgressService) getSite().getAdapter(
					IWorkbenchSiteProgressService.class);
			siteService.schedule(job, 0, true);
		}
	}
	
	/**
	 * Add new group.
	 */
	private void addGroup()
	{
		final CreateObjectDialog dlg = new CreateObjectDialog(getViewSite().getShell(), false);
		if (dlg.open() == Window.OK)
		{
			Job job = new Job("Create group")
			{
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;

					try
					{
						editNewUser = dlg.isEditAfterCreate();
						userManager.createUserGroup(dlg.getLoginName());
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID,
								(e instanceof NetXMSClientException) ? ((NetXMSClientException)e).getErrorCode() : 0,
								"Cannot create group: " + e.getMessage(), null);
					}
					return status;
				}

				/* (non-Javadoc)
				 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
				 */
				@Override
				public boolean belongsTo(Object family)
				{
					return family == UserManagementView.JOB_FAMILY;
				}
			};
			IWorkbenchSiteProgressService siteService = (IWorkbenchSiteProgressService) getSite().getAdapter(
					IWorkbenchSiteProgressService.class);
			siteService.schedule(job, 0, true);
		}
	}

	/**
	 * Delete user or group
	 */
	private void deleteUser()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();

		final String message = "Do you really wish to delete selected user" + ((selection.size() > 1) ? "s?" : "?");
		final Shell shell = getViewSite().getShell();
		if (!MessageDialog.openQuestion(shell, "Confirm user deletion", message))
		{
			return;
		}

		Job job = new Job("Delete user database objects")
		{
			@SuppressWarnings("rawtypes")
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;

				try
				{
					Iterator it = selection.iterator();
					while(it.hasNext())
					{
						Object object = it.next();
						if (object instanceof AbstractUserObject)
						{
							userManager.deleteUserDBObject(((AbstractUserObject)object).getId());
						}
						else
						{
							throw new NetXMSClientException(CommonRCC.INTERNAL_ERROR) {
								private static final long serialVersionUID = -5171658794925752611L;

								@Override
								protected String getErrorMessage(int code)
								{
									return (code == CommonRCC.INTERNAL_ERROR) ? "Internal error" : null;
								}
							};
						}
					}
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID,
							(e instanceof NetXMSClientException) ? ((NetXMSClientException)e).getErrorCode() : 0,
							"Cannot delete user database object: " + e.getMessage(), null);
				}
				return status;
			}

			/* (non-Javadoc)
			 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
			 */
			@Override
			public boolean belongsTo(Object family)
			{
				return family == UserManagementView.JOB_FAMILY;
			}
		};
		IWorkbenchSiteProgressService siteService = (IWorkbenchSiteProgressService) getSite().getAdapter(
				IWorkbenchSiteProgressService.class);
		siteService.schedule(job, 0, true);
	}
}
