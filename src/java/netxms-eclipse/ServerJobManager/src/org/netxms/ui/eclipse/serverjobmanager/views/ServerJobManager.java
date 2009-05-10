package org.netxms.ui.eclipse.serverjobmanager.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCServerJob;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.serverjobmanager.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.RefreshAction;
import org.netxms.ui.eclipse.tools.SortableTableViewer;


/**
 * Server job manager - provides view to manage server-side jobs
 */

public class ServerJobManager extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.serverjobmanager.view.server_job_manager";
	public static final String JOB_FAMILY = "ServerJobManagerJob";
		
	private TableViewer viewer;
	private NXCSession session;
	private NXCServerJob[] jobList;
	
	private RefreshAction actionRefresh;
	
	// Columns
	private static final int COL_STATUS = 0;
	private static final int COL_NODE = 1;
	private static final int COL_DESCRIPTION = 2;
	private static final int COL_PROGRESS = 3;


	/**
	 * Label provider for server job list
	 * 
	 * @author Victor
	 *
	 */
	private class ViewLabelProvider extends LabelProvider implements ITableLabelProvider
	{
		/**
		 * Returns text for given column 
		 */
		@Override
		public String getColumnText(Object obj, int index)
		{
			if (obj instanceof NXCServerJob)
			{
				switch(index)
				{
					case COL_STATUS:
						return Integer.toString(((NXCServerJob)obj).getStatus());
					case COL_NODE:
						NXCObject object = session.findObjectById(((NXCServerJob)obj).getNodeId());
						return (object != null) ? object.getObjectName() : "<unknown>";
					case COL_DESCRIPTION:
						return ((NXCServerJob)obj).getDescription();
					case COL_PROGRESS:
						return (((NXCServerJob)obj).getStatus() == NXCServerJob.ACTIVE) ? Integer.toString(((NXCServerJob)obj).getProgress()) + "%" : "";
				}
			}
			return "";
		}

		@Override
		public Image getImage(Object obj)
		{
			return null;
		}

		@Override
		public Image getColumnImage(Object element, int columnIndex)
		{
			// TODO Auto-generated method stub
			return null;
		}
	}

	/**
	 * Comparator for server job list
	 * 
	 * @author Victor
	 *
	 */
	private class JobComparator extends ViewerComparator
	{
		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
		 */
		@Override
		public int compare(Viewer viewer, Object e1, Object e2)
		{
			int result;
			
			switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
			{
				case COL_STATUS:
					result = ((NXCServerJob)e1).getStatus() - ((NXCServerJob)e2).getStatus();
					break;
				case COL_NODE:
					NXCObject object1 = session.findObjectById(((NXCServerJob)e1).getNodeId());
					NXCObject object2 = session.findObjectById(((NXCServerJob)e2).getNodeId());
					String name1 = (object1 != null) ? object1.getObjectName() : "<unknown>";
					String name2 = (object2 != null) ? object2.getObjectName() : "<unknown>";
					result = name1.compareToIgnoreCase(name2);
					break;
				case COL_DESCRIPTION:
					result = ((NXCServerJob)e1).getDescription().compareToIgnoreCase(((NXCServerJob)e2).getDescription());
					break;
				default:
					result = 0;
					break;
			}
			return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.DOWN) ? result : -result;
		}
	}

	
	/**
	 * Refresh job
	 * 
	 * @author victor
	 */
	class RefreshJob extends Job
	{
		public RefreshJob()
		{
			super("Get server configuration variables");
		}

		@Override
		protected IStatus run(IProgressMonitor monitor)
		{
			IStatus status;
			
			try
			{
				jobList = session.getServerJobList();
				status = Status.OK_STATUS;

				new UIJob("Initialize server configuration editor") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						synchronized(jobList)
						{
							viewer.setInput(jobList);
						}
						return Status.OK_STATUS;
					}
				}.schedule();
			}
			catch(Exception e)
			{
				status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
                    "Cannot get list of server jobs: " + e.getMessage(), e);
			}
			return status;
		}

		/* (non-Javadoc)
		 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
		 */
		@Override
		public boolean belongsTo(Object family)
		{
			return family == ServerJobManager.JOB_FAMILY;
		}
	};
	
	
	/**
	 * The constructor.
	 */
	public ServerJobManager()
	{
	}

	/**
	 * This is a callback that will allow us to create the viewer and initialize
	 * it.
	 */
	public void createPartControl(Composite parent)
	{
		final String[] names = { "Status", "Node", "Description", "Progress" };
		final int[] widths = { 80, 150, 250, 100 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ViewLabelProvider());
		viewer.setComparator(new JobComparator());
		
		makeActions();
		contributeToActionBars();
		createPopupMenu();
		
		session = NXMCSharedData.getInstance().getSession();

		Job job = new RefreshJob();
		IWorkbenchSiteProgressService siteService =
	      (IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class);
		siteService.schedule(job, 0, true);
	}

	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionRefresh);
	}

	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionRefresh);
	}

	private void makeActions()
	{
		actionRefresh = new RefreshAction() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				Job job = new RefreshJob();
				IWorkbenchSiteProgressService siteService =
			      (IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class);
				siteService.schedule(job, 0, true);
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
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}

	
	/**
	 * Passing the focus request to the viewer's control.
	 */
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}
}
