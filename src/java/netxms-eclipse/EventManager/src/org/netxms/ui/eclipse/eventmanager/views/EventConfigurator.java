/**
 * 
 */
package org.netxms.ui.eclipse.eventmanager.views;

import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.tools.RefreshAction;
import org.netxms.ui.eclipse.tools.SortableTableViewer;

/**
 * @author Victor
 * 
 */
public class EventConfigurator extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.eventmanager.view.event_configurator";
	public static final String JOB_FAMILY = "EventConfiguratorJob";

	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_SEVERITY = 2;
	public static final int COLUMN_FLAGS = 3;
	public static final int COLUMN_MESSAGE = 4;
	public static final int COLUMN_DESCRIPTION = 5;

	private TableViewer viewer;
	private boolean databaseLocked = false;
	private RefreshAction actionRefresh;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { "ID", "Name", "Severity", "Flags", "Message", "Description" };
		final int[] widths = { 80, 100, 100, 70, 250, 250 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
//		viewer.setLabelProvider(new UserLabelProvider());
//		viewer.setComparator(new UserComparator());
		viewer.addSelectionChangedListener(new ISelectionChangedListener()
		{
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
				}
			}
		});
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
			}
		});

		makeActions();
		contributeToActionBars();
		createPopupMenu();

		// Request server to lock event database, and on success refresh view
		/*
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
					new UIJob("Update user list")
					{
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							viewer.setInput(session.getUserDatabaseObjects());
							return Status.OK_STATUS;
						}
					}.schedule();
					session.addListener(sessionListener);
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, (e instanceof NXCException) ? ((NXCException) e)
							.getErrorCode() : 0, "Cannot lock user database: " + e.getMessage(), null);
					new UIJob("Close user manager")
					{
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							EventConfigurator.this.getViewSite().getPage().hideView(EventConfigurator.this);
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
		*/
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
//				viewer.setInput(session.getUserDatabaseObjects());
			}
		};

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
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
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
/*		if (sessionListener != null)
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
						status = new Status(Status.ERROR, Activator.PLUGIN_ID,
								(e instanceof NXCException) ? ((NXCException) e).getErrorCode() : 0,
								"Cannot unlock user database: " + e.getMessage(), null);
					}
					return status;
				}
			}.schedule();
		}*/
		super.dispose();
	}
}
