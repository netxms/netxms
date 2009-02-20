package org.netxms.ui.eclipse.jobmonitor;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.part.*;
import org.eclipse.ui.progress.IProgressService;
import org.eclipse.core.runtime.jobs.IJobChangeEvent;
import org.eclipse.core.runtime.jobs.IJobChangeListener;
import org.eclipse.core.runtime.jobs.IJobManager;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.viewers.*;
import org.eclipse.swt.graphics.Image;
import org.eclipse.jface.action.*;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.ui.*;
import org.eclipse.swt.SWT;

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

public class JobMonitor extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.jobmonitor.view.job_monitor";
	
	private TableViewer viewer;
	private Action actionClearCompleted;
	private Action actionShowSystem;
	
	// Columns
	private static final int COL_NAME = 0;
	private static final int COL_STATE = 1;
	private static final int COL_SYSTEM = 2;

	/*
	 * The content provider class is responsible for providing objects to the
	 * view. It can wrap existing objects in adapters or simply return objects
	 * as-is. These objects may be sensitive to the current input of the view, or
	 * ignore it and always show the same content (like Task List, for example).
	 */

	class ViewContentProvider implements IStructuredContentProvider
	{
		public void inputChanged(Viewer v, Object oldInput, Object newInput)
		{
		}

		public void dispose()
		{
		}

		public Object[] getElements(Object parent)
		{
			return (parent instanceof IJobManager) ? ((IJobManager)parent).find(null) : null;
		}
	}

	class ViewLabelProvider extends LabelProvider implements ITableLabelProvider
	{
		/**
		 * Convert job state to textual representation
		 * @param state Job state code
		 * @return Job state as text
		 */
		private String stateToText(int state)
		{
			switch(state)
			{
				case Job.RUNNING:
					return "running";
				case Job.WAITING:
					return "waiting";
				case Job.SLEEPING:
					return "sleeping";
				case Job.NONE:
					return "other";
				default:
					return "unknown";
			}
		}
		
		
		/**
		 * Returns text for given column 
		 */
		@Override
		public String getColumnText(Object obj, int index)
		{
			switch(index)
			{
				case COL_NAME:
					return ((Job)obj).getName();
				case COL_STATE:
					return stateToText(((Job)obj).getState());
				case COL_SYSTEM:
					return ((Job)obj).isSystem() ? "yes" : "no";
				default:
					return "";
			}
		}

		@Override
		public Image getColumnImage(Object obj, int index)
		{
			switch(index)
			{
				case COL_NAME:
					return getImage(obj);
				case COL_STATE:
					return null;
				default:
					return null;
			}
		}

		@Override
		public Image getImage(Object obj)
		{
			IProgressService service = PlatformUI.getWorkbench().getProgressService();
			Image image = service.getIconFor((Job)obj);
			return (image != null) ? image : Activator.getImageDescriptor("icons/default.png").createImage();
		}
	}

	class NameSorter extends ViewerSorter
	{
	}

	
	/**
	 * Job change listener class for job monitor
	 * 
	 * @author victor
	 */
	class ChangeListener implements IJobChangeListener
	{
		@Override
		public void aboutToRun(IJobChangeEvent event)
		{
			viewer.refresh();
		}

		@Override
		public void awake(IJobChangeEvent event)
		{
			viewer.refresh();
		}

		@Override
		public void done(IJobChangeEvent event)
		{
			viewer.refresh();
		}

		@Override
		public void running(IJobChangeEvent event)
		{
			viewer.refresh();
		}

		@Override
		public void scheduled(IJobChangeEvent event)
		{
			viewer.refresh();
		}

		@Override
		public void sleeping(IJobChangeEvent event)
		{
			viewer.refresh();
		}
	}

	
	/**
	 * The constructor.
	 */
	public JobMonitor()
	{
	}

	/**
	 * This is a callback that will allow us to create the viewer and initialize
	 * it.
	 */
	public void createPartControl(Composite parent)
	{
		// Setup table
		final Table table = new Table(parent, SWT.VIRTUAL | SWT.FULL_SELECTION);
		final String[] names = { "Name", "State", "System" };
		final int[] widths = { 200, 80, 80 };
		for(int i = 0; i < names.length; i++)
		{
			final TableColumn tc = new TableColumn(table, SWT.LEFT);
			tc.setText(names[i]);
			tc.setWidth(widths[i]);
		}
		table.setLinesVisible(true);
		table.setHeaderVisible(true);
		
		viewer = new TableViewer(table);
		viewer.setContentProvider(new ViewContentProvider());
		viewer.setLabelProvider(new ViewLabelProvider());
		viewer.setSorter(new NameSorter());
		viewer.setInput(Job.getJobManager());

		// Create the help context id for the viewer's control
		PlatformUI.getWorkbench().getHelpSystem().setHelp(viewer.getControl(), "org.netxms.nxmc.jobmonitor.viewer");
		makeActions();
		contributeToActionBars();
		
		Job.getJobManager().addJobChangeListener(new ChangeListener());
	}

	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionClearCompleted);
		manager.add(actionShowSystem);
	}

	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionClearCompleted);
	}

	private void makeActions()
	{
		actionClearCompleted = new Action()
		{
			public void run()
			{
				showMessage("Action 1 executed");
			}
		};
		actionClearCompleted.setText("Clear Completed");
		actionClearCompleted.setToolTipText("Remove completed jobs from the list");
		actionClearCompleted.setImageDescriptor(PlatformUI.getWorkbench().getSharedImages().getImageDescriptor(ISharedImages.IMG_OBJS_INFO_TSK));

		actionShowSystem = new Action()
		{
			public void run()
			{
			}
		};
		actionShowSystem.setText("Show System Jobs");
		actionShowSystem.setToolTipText("Show system jobs in the list");
	}

	private void showMessage(String message)
	{
		MessageDialog.openInformation(viewer.getControl().getShell(), "Job Monitor", message);
	}

	/**
	 * Passing the focus request to the viewer's control.
	 */
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}
}
