package org.netxms.ui.eclipse.serverconfig.views;

import java.util.HashMap;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.part.*;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.viewers.*;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.graphics.Image;
import org.eclipse.jface.action.*;
import org.eclipse.ui.*;
import org.eclipse.swt.SWT;
import org.netxms.api.client.servermanager.ServerVariable;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.dialogs.VariableEditDialog;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.RefreshAction;
import org.netxms.ui.eclipse.tools.SortableTableViewer;

/**
 */

public class ServerConfigurationEditor extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.view.server_config";
	public static final String JOB_FAMILY = "ServerConfigJob";
		
	private TableViewer viewer;
	private NXCSession session;
	private HashMap<String, ServerVariable> varList;
	
	private Action actionAddVariable;
	private RefreshAction actionRefresh;
	
	// Columns
	private static final int COLUMN_NAME = 0;
	private static final int COLUMN_VALUE = 1;
	private static final int COLUMN_NEED_RESTART = 2;

	/**
	 * Label provider for server configuration variables
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
			switch(index)
			{
				case COLUMN_NAME:
					return ((ServerVariable)obj).getName();
				case COLUMN_VALUE:
					return ((ServerVariable)obj).getValue();
				case COLUMN_NEED_RESTART:
					return ((ServerVariable)obj).isServerRestartNeeded() ? "Yes" : "No";
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
			return null;
		}
	}

	/**
	 * Comparator for server configuration variables
	 * @author Victor
	 *
	 */
	private class VarComparator extends ViewerComparator
	{
		/**
		 * Compare two booleans and return -1, 0, or 1
		 */
		private int compareBooleans(boolean b1, boolean b2)
		{
			return (!b1 && b2) ? -1 : ((b1 && !b2) ? 1 : 0);
		}
		
		
		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
		 */
		@Override
		public int compare(Viewer viewer, Object e1, Object e2)
		{
			int result;
			
			switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
			{
				case COLUMN_NAME:
					result = ((ServerVariable)e1).getName().compareToIgnoreCase(((ServerVariable)e2).getName());
					break;
				case COLUMN_VALUE:
					result = ((ServerVariable)e1).getValue().compareToIgnoreCase(((ServerVariable)e2).getValue());
					break;
				case COLUMN_NEED_RESTART:
					result = compareBooleans(((ServerVariable)e1).isServerRestartNeeded(), ((ServerVariable)e2).isServerRestartNeeded());
					break;
				default:
					result = 0;
					break;
			}
			return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
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
			super("Get server configuration variables");
		}

		@Override
		protected IStatus run(IProgressMonitor monitor)
		{
			IStatus status;
			
			try
			{
				varList = session.getServerVariables();
				status = Status.OK_STATUS;

				new UIJob("Initialize server configuration editor") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						synchronized(varList)
						{
							viewer.setInput(varList.values().toArray());
						}
						return Status.OK_STATUS;
					}
				}.schedule();
			}
			catch(Exception e)
			{
				status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
                    "Cannot get server configuration variables: " + e.getMessage(), e);
			}
			return status;
		}

		/* (non-Javadoc)
		 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
		 */
		@Override
		public boolean belongsTo(Object family)
		{
			return family == ServerConfigurationEditor.JOB_FAMILY;
		}
	};
	
	
	/**
	 * The constructor.
	 */
	public ServerConfigurationEditor()
	{
	}

	/**
	 * This is a callback that will allow us to create the viewer and initialize
	 * it.
	 */
	public void createPartControl(Composite parent)
	{
		final String[] names = { "Name", "Value", "Restart" };
		final int[] widths = { 200, 150, 80 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ViewLabelProvider());
		viewer.setComparator(new VarComparator());
		
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				if ((selection != null) && (selection.size() == 1))
				{
					ServerVariable var = (ServerVariable)selection.getFirstElement();
					if (var != null)
					{
						editVariable(var);
					}
				}
			}
		});

		// Create the help context id for the viewer's control
		PlatformUI.getWorkbench().getHelpSystem().setHelp(viewer.getControl(), "org.netxms.nxmc.serverconfig.viewer");
		makeActions();
		contributeToActionBars();
		createPopupMenu();
		
		session = NXMCSharedData.getInstance().getSession();

		Job job = new RefreshJob();
		IWorkbenchSiteProgressService siteService =
	      (IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class);
		siteService.schedule(job, 0, true);
	}

	/**
	 * Fill action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionRefresh);
		manager.add(actionAddVariable);
	}

	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionRefresh);
		manager.add(actionAddVariable);
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
		
		actionAddVariable = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				ServerConfigurationEditor.this.addVariable();
			}
		};
		actionAddVariable.setText("Add variable");
		actionAddVariable.setImageDescriptor(Activator.getImageDescriptor("icons/add_variable.png"));
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
	
	
	/**
	 * Add new variable
	 */
	private void addVariable()
	{
		final VariableEditDialog dlg = new VariableEditDialog(getSite().getShell(), null, null);
		if (dlg.open() == Window.OK)
		{
			Job job = new Job("Create configuration variable") {
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;
					
					try
					{
						NXMCSharedData.getInstance().getSession().setServerVariable(dlg.getVarName(), dlg.getVarValue());
						actionRefresh.run();
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
						                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
						                    "Cannot create configuration variable: " + e.getMessage(), e);
					}
					return status;
				}

				
				/* (non-Javadoc)
				 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
				 */
				@Override
				public boolean belongsTo(Object family)
				{
					return family == ServerConfigurationEditor.JOB_FAMILY;
				}
			};
			IWorkbenchSiteProgressService siteService =
		      (IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class);
			siteService.schedule(job, 0, true);
		}
	}
	
	/**
	 * Refresh list
	 */
	public void refreshVariablesList()
	{
		actionRefresh.run();
	}
	
	/**
	 * Edit currently selected variable
	 * @param var
	 */
	public void editVariable(ServerVariable var)
	{
		final VariableEditDialog dlg = new VariableEditDialog(getSite().getShell(), var.getName(), var.getValue());
		if (dlg.open() == Window.OK)
		{
			Job job = new Job("Modify configuration variable") {
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;
					
					try
					{
						NXMCSharedData.getInstance().getSession().setServerVariable(dlg.getVarName(), dlg.getVarValue());
						refreshVariablesList();
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
						                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
						                    "Cannot modify configuration variable: " + e.getMessage(), e);
					}
					return status;
				}
				
				/* (non-Javadoc)
				 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
				 */
				@Override
				public boolean belongsTo(Object family)
				{
					return family == ServerConfigurationEditor.JOB_FAMILY;
				}
			};
			IWorkbenchSiteProgressService siteService =
		      (IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class);
			siteService.schedule(job, 0, true);
		}
	}
}
