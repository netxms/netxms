/**
 * 
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
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.NXCUser;
import org.netxms.client.NXCUserDBObject;
import org.netxms.client.NXCUserGroup;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.RefreshAction;
import org.netxms.ui.eclipse.tools.SortableTableViewer;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.dialogs.CreateObjectDialog;

/**
 * @author Victor
 * 
 */
public class UserManager extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.usermanager.view.user_manager";
	public static final String JOB_FAMILY = "UserManagerJob";

	// Columns
	private static final int COLUMN_NAME = 0;
	private static final int COLUMN_TYPE = 1;
	private static final int COLUMN_FULLNAME = 2;
	private static final int COLUMN_DESCRIPTION = 3;
	private static final int COLUMN_GUID = 4;

	private TableViewer viewer;
	private NXCSession session;
	private NXCListener sessionListener;
	private boolean databaseLocked = false;
	private long newUserId = -1;
	private NXCUserDBObject newUser;
	private String newUserFound = "";
	private Action actionAddUser;
	private Action actionAddGroup;
	private Action actionEditUser;
	private Action actionDeleteUser;
	private Action actionChangePassword;
	private RefreshAction actionRefresh;

	/**
	 * Label provider for user manager
	 * 
	 * @author Victor
	 * 
	 */
	private class UserLabelProvider implements ITableLabelProvider
	{
		private Image userImage;
		private Image groupImage;

		public UserLabelProvider()
		{
			userImage = Activator.getImageDescriptor("icons/user.png").createImage();
			groupImage = Activator.getImageDescriptor("icons/group.png").createImage();
		}

		@Override
		public Image getColumnImage(Object element, int columnIndex)
		{
			if (columnIndex == 0)
			{
				if (element instanceof NXCUser)
					return userImage;
				if (element instanceof NXCUserGroup)
					return groupImage;
			}
			return null;
		}

		@Override
		public String getColumnText(Object element, int columnIndex)
		{
			if (!(element instanceof NXCUserDBObject))
				return null;

			switch(columnIndex)
			{
				case COLUMN_NAME:
					return ((NXCUserDBObject) element).getName();
				case COLUMN_TYPE:
					return (element instanceof NXCUser) ? "User" : "Group";
				case COLUMN_FULLNAME:
					return (element instanceof NXCUser) ? ((NXCUser) element).getFullName() : null;
				case COLUMN_DESCRIPTION:
					return ((NXCUserDBObject) element).getDescription();
				case COLUMN_GUID:
					return ((NXCUserDBObject) element).getGuid().toString();
			}
			return null;
		}

		@Override
		public void addListener(ILabelProviderListener listener)
		{
		}

		@Override
		public void dispose()
		{
			userImage.dispose();
			groupImage.dispose();
		}

		@Override
		public boolean isLabelProperty(Object element, String property)
		{
			return false;
		}

		@Override
		public void removeListener(ILabelProviderListener listener)
		{
		}
	}

	/**
	 * Comparator for user list
	 * 
	 * @author Victor
	 * 
	 */
	private class UserComparator extends ViewerComparator
	{
		/**
		 * Compare object types
		 */
		private int compareTypes(Object o1, Object o2)
		{
			int type1 = (o1 instanceof NXCUserGroup) ? 1 : 0;
			int type2 = (o2 instanceof NXCUserGroup) ? 1 : 0;
			return type1 - type2;
		}

		/**
		 * Get full name for user db object
		 */
		private String getFullName(Object object)
		{
			if (object instanceof NXCUser)
				return ((NXCUser) object).getFullName();
			return "";
		}

		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
		 */
		@Override
		public int compare(Viewer viewer, Object e1, Object e2)
		{
			int result;

			switch((Integer) ((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"))
			{
				case COLUMN_NAME:
					result = ((NXCUserDBObject) e1).getName().compareToIgnoreCase(((NXCUserDBObject) e2).getName());
					break;
				case COLUMN_TYPE:
					result = compareTypes(e1, e2);
					break;
				case COLUMN_FULLNAME:
					result = getFullName(e1).compareToIgnoreCase(getFullName(e2));
					break;
				case COLUMN_DESCRIPTION:
					result = ((NXCUserDBObject) e1).getDescription().compareToIgnoreCase(
							((NXCUserDBObject) e2).getDescription());
					break;
				case COLUMN_GUID:
					result = ((NXCUserDBObject) e1).getGuid().toString().compareTo(
							((NXCUserDBObject) e2).getGuid().toString());
					break;
				default:
					result = 0;
					break;
			}
			return (((SortableTableViewer) viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
		}
	}

	/**
	 * Refresh job
	 * 
	 * @author victor
	 */
	private class RefreshJob extends Job
	{

		public RefreshJob()
		{
			super("Get user database");
		}

		@Override
		protected IStatus run(IProgressMonitor monitor)
		{
			IStatus status;

			try
			{
				final NXCUserDBObject[] origUsers = session.getUserDatabaseObjects();
				final NXCUserDBObject[] users = new NXCUserDBObject[origUsers.length];
				for(int i = 0; i < origUsers.length; i++)
				{
					users[i] = (NXCUserDBObject) origUsers[i].clone();
					if (users[i].getId() == newUserId)
					{
						newUser = users[i];
						synchronized(newUserFound)
						{
							newUserFound.notifyAll();
						}
					}
				}
				status = Status.OK_STATUS;

				new UIJob("Refresh user manager content")
				{
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						viewer.setInput(users);
						return Status.OK_STATUS;
					}
				}.schedule();
			}
			catch(Exception e)
			{
				status = new Status(Status.ERROR, Activator.PLUGIN_ID, (e instanceof NXCException) ? ((NXCException) e)
						.getErrorCode() : 0, "Cannot get user database: " + e.getMessage(), e);
			}
			return status;
		}

		/* (non-Javadoc)
		 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
		 */
		@Override
		public boolean belongsTo(Object family)
		{
			return family == UserManager.JOB_FAMILY;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = NXMCSharedData.getInstance().getSession();

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
				IStructuredSelection selection = (IStructuredSelection) event.getSelection();
				if (selection != null)
				{
					actionEditUser.setEnabled(selection.size() == 1);
					actionChangePassword.setEnabled(selection.size() == 1);
					actionDeleteUser.setEnabled(selection.size() > 0);
				}
			}
		});

		makeActions();
		contributeToActionBars();
		createPopupMenu();

		// Listener for server's notifications
		sessionListener = new NXCListener()
		{
			@Override
			public void notificationHandler(NXCNotification n)
			{
				if (n.getCode() == NXCNotification.USER_DB_CHANGED)
				{
					actionRefresh.run();
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
					session.lockUserDatabase();
					databaseLocked = true;
					actionRefresh.run();
					session.addListener(sessionListener);
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, (e instanceof NXCException) ? ((NXCException) e)
							.getErrorCode() : 0, "Cannot lock user database: " + e.getMessage(), e);
					new UIJob("Close user manager")
					{
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							UserManager.this.getViewSite().getPage().hideView(UserManager.this);
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
		manager.add(actionEditUser);
		manager.add(actionDeleteUser);
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
		manager.add(actionEditUser);
		manager.add(actionDeleteUser);
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
				Job job = new RefreshJob();
				IWorkbenchSiteProgressService siteService = (IWorkbenchSiteProgressService) getSite().getAdapter(
						IWorkbenchSiteProgressService.class);
				siteService.schedule(job, 0, true);
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
								newUser = null;
								newUserId = session.createUser(dlg.getLoginName());
								if (dlg.isEditAfterCreate())
								{
									// Wait for local user database copy update
									synchronized(newUserFound)
									{
										newUserFound.wait(3000);
									}
									if (newUser != null)
									{
										new UIJob("Edit new user")
										{
											@Override
											public IStatus runInUIThread(IProgressMonitor monitor)
											{
												viewer.setSelection(new StructuredSelection(newUser), true);
												actionEditUser.run();
												return Status.OK_STATUS;
											}
										}.schedule();
									}
								}
								status = Status.OK_STATUS;
							}
							catch(Exception e)
							{
								status = new Status(Status.ERROR, Activator.PLUGIN_ID,
										(e instanceof NXCException) ? ((NXCException) e).getErrorCode() : 0,
										"Cannot create user: " + e.getMessage(), e);
							}
							actionRefresh.run();
							return status;
						}

						/* (non-Javadoc)
						 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
						 */
						@Override
						public boolean belongsTo(Object family)
						{
							return family == UserManager.JOB_FAMILY;
						}
					};
					IWorkbenchSiteProgressService siteService = (IWorkbenchSiteProgressService) getSite().getAdapter(
							IWorkbenchSiteProgressService.class);
					siteService.schedule(job, 0, true);
				}
			}
		};
		actionAddUser.setText("Create new user");
		actionAddUser.setImageDescriptor(Activator.getImageDescriptor("icons/user_add.png"));

		actionAddGroup = new Action()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
			}
		};
		actionAddGroup.setText("Create new group");
		actionAddGroup.setImageDescriptor(Activator.getImageDescriptor("icons/group_add.png"));

		actionEditUser = new PropertyDialogAction(getSite(), viewer);
		actionEditUser.setText("Edit");
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
				final IStructuredSelection selection = (IStructuredSelection) viewer.getSelection();

				final String message = "Do you really wish to delete selected user" + ((selection.size() > 1) ? "s?" : "?");
				final Shell shell = UserManager.this.getViewSite().getShell();
				if (!MessageDialog.openQuestion(shell, "Confirm user deletion", message))
				{
					return;
				}

				Job job = new Job("Delete user database objects")
				{
					@SuppressWarnings("unchecked")
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
								if (object instanceof NXCUserDBObject)
								{
									session.deleteUserDBObject(((NXCUserDBObject) object).getId());
								}
								else
								{
									throw new NXCException(NXCSession.RCC_INTERNAL_ERROR);
								}
							}
							status = Status.OK_STATUS;
						}
						catch(Exception e)
						{
							status = new Status(Status.ERROR, Activator.PLUGIN_ID,
									(e instanceof NXCException) ? ((NXCException) e).getErrorCode() : 0,
									"Cannot delete user database object: " + e.getMessage(), e);
						}
						actionRefresh.run();
						return status;
					}

					/* (non-Javadoc)
					 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
					 */
					@Override
					public boolean belongsTo(Object family)
					{
						return family == UserManager.JOB_FAMILY;
					}
				};
				IWorkbenchSiteProgressService siteService = (IWorkbenchSiteProgressService) getSite().getAdapter(
						IWorkbenchSiteProgressService.class);
				siteService.schedule(job, 0, true);
			}
		};
		actionDeleteUser.setText("Delete");
		actionDeleteUser.setImageDescriptor(Activator.getImageDescriptor("icons/user_delete.png"));
		actionDeleteUser.setEnabled(false);

		actionChangePassword = new Action()
		{

			@Override
			public void run()
			{
				final IStructuredSelection selection = (IStructuredSelection) viewer.getSelection();

				final Object firstElement = selection.getFirstElement();
				if (firstElement instanceof org.netxms.client.NXCUser)
				{
					NXCUser user = (NXCUser) firstElement;
					// do change password
				}
			}

		};
		actionChangePassword.setText("Change Password");
		actionChangePassword.setImageDescriptor(Activator.getImageDescriptor("icons/group_key.png"));
		actionChangePassword.setEnabled(false);
	}

	/**
	 * Create pop-up menu for variable list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
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
	 * @param mgr
	 *           Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(actionEditUser);
		mgr.add(actionDeleteUser);
		mgr.add(actionChangePassword);
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
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
						session.unlockUserDatabase();
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						e.printStackTrace();
						status = new Status(Status.ERROR, Activator.PLUGIN_ID,
								(e instanceof NXCException) ? ((NXCException) e).getErrorCode() : 0,
								"Cannot unlock user database: " + e.getMessage(), e);
					}
					return status;
				}
			}.schedule();
		}
		super.dispose();
	}
}
