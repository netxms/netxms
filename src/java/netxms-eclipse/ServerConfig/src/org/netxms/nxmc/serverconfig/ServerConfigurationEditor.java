package org.netxms.nxmc.serverconfig;

import java.util.HashMap;

import org.eclipse.swt.widgets.Composite;
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
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.ui.*;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCException;
import org.netxms.client.NXCServerVariable;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.core.extensionproviders.NXMCSharedData;
import org.netxms.nxmc.tools.RefreshAction;
import org.netxms.nxmc.tools.SortableTableViewer;

/**
 * This sample class demonstrates how to plug-in a new workbench view. The view
 * shows data obtained from the model. The sample creates a dummy model on the
 * fly, but a real implementation would connect to the model available either in
 * this or another plug-in (e.g. the workspace). The view is connected to the
 * model using a content provider.
 * <p>
 * The view uses a label provider to define how model objects should be
 * presented in the view. Each view can present the same model objects using
 * different labels and icons, if needed. Alternatively, a single label provider
 * can be shared between views in order to ensure that objects of the same type
 * are presented in the same way everywhere.
 * <p>
 */

public class ServerConfigurationEditor extends ViewPart
{
	public static final String ID = "org.netxms.nxmc.serverconfig.view.server_config";
	public static final String JOB_FAMILY = "ServerConfigJob";
		
	private TableViewer viewer;
	private NXCSession session;
	private HashMap<String, NXCServerVariable> varList;
	
	private Action actionAddVariable;
	private RefreshAction actionRefresh;
	
	// Columns
	private static final int COL_NAME = 0;
	private static final int COL_VALUE = 1;
	private static final int COL_NEED_RESTART = 2;


	class ViewLabelProvider extends LabelProvider implements ITableLabelProvider
	{
		/**
		 * Returns text for given column 
		 */
		@Override
		public String getColumnText(Object obj, int index)
		{
			switch(index)
			{
				case COL_NAME:
					return ((NXCServerVariable)obj).getName();
				case COL_VALUE:
					return ((NXCServerVariable)obj).getValue();
				case COL_NEED_RESTART:
					return ((NXCServerVariable)obj).isServerRestartNeeded() ? "Yes" : "No";
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

	class VarComparator extends ViewerComparator
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
				case COL_NAME:
					result = ((NXCServerVariable)e1).getName().compareToIgnoreCase(((NXCServerVariable)e2).getName());
					break;
				case COL_VALUE:
					result = ((NXCServerVariable)e1).getValue().compareToIgnoreCase(((NXCServerVariable)e2).getValue());
					break;
				case COL_NEED_RESTART:
					result = compareBooleans(((NXCServerVariable)e1).isServerRestartNeeded(), ((NXCServerVariable)e2).isServerRestartNeeded());
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
		final int[] widths = { 200, 80, 80 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ViewLabelProvider());
		viewer.setComparator(new VarComparator());

		// Create the help context id for the viewer's control
		PlatformUI.getWorkbench().getHelpSystem().setHelp(viewer.getControl(), "org.netxms.nxmc.serverconfig.viewer");
		makeActions();
		contributeToActionBars();
		
		session = NXMCSharedData.getSession();

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
		final VariableEditDialog dlg = new VariableEditDialog(getSite().getShell(), null);
		if (dlg.open() == Window.OK)
		{
			MessageDialog.openInformation(null, "zz", "ok");
			Job job = new Job("Create configuration variable") {
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;
					
					try
					{
						NXMCSharedData.getSession().setServerVariable(dlg.getVarName(), dlg.getVarValue());
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
			};
			IWorkbenchSiteProgressService siteService =
		      (IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class);
			siteService.schedule(job, 0, true);
		}
	}
}
