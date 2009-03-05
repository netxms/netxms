/**
 * 
 */
package org.netxms.ui.eclipse.objectview;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCDCIValue;
import org.netxms.client.NXCException;
import org.netxms.client.NXCNode;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.SortableTableViewer;

/**
 * @author victor
 *
 */
public class LastValuesView extends Composite
{
	public static final String JOB_FAMILY = "LastValuesViewJob";
	
	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_DESCRIPTION = 1;
	public static final int COLUMN_VALUE = 2;
	public static final int COLUMN_TIMESTAMP = 3;
	
	private final ViewPart viewPart;
	private final NXCNode node;
	private NXCSession session;
	private TableViewer dataViewer;
	
	public LastValuesView(ViewPart viewPart, Composite parent, int style, NXCNode _node)
	{
		super(parent, style);
		session = NXMCSharedData.getInstance().getSession();
		this.viewPart = viewPart;		
		this.node = _node;
		
		// Setup table columns
		final String[] names = { "ID", "Description", "Value", "Timestamp" };
		final int[] widths = { 70, 250, 150, 100 };
		dataViewer = new SortableTableViewer(this, names, widths, 0, SWT.DOWN);
	
		dataViewer.setLabelProvider(new LastValuesLabelProvider());
		dataViewer.setContentProvider(new ArrayContentProvider());
		//dataViewer.setComparator(new LastValuesComparator());
		
		createPopupMenu();

		addListener(SWT.Resize, new Listener() {
			public void handleEvent(Event e)
			{
				dataViewer.getControl().setBounds(LastValuesView.this.getClientArea());
			}
		});

		// Request data from server
		Job job = new Job("Get DCI values for node " + node.getObjectName())
		{
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					final NXCDCIValue[] data = session.getLastValues(node.getObjectId());
					status = Status.OK_STATUS;

					new UIJob("Initialize last values view") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							dataViewer.setInput(data);
							return Status.OK_STATUS;
						}
					}.schedule();
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
	                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
	                    "Cannot get DCI values for node " + node.getObjectName() + ": " + e.getMessage(), e);
				}
				return status;
			}

			/* (non-Javadoc)
			 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
			 */
			@Override
			public boolean belongsTo(Object family)
			{
				return family == LastValuesView.JOB_FAMILY;
			}
		};
		IWorkbenchSiteProgressService siteService =
	      (IWorkbenchSiteProgressService)viewPart.getSite().getAdapter(IWorkbenchSiteProgressService.class);
		siteService.schedule(job, 0, true);
	}
	
	
	/**
	 * Schedule alarm viewer update
	 */
	/*private void scheduleAlarmViewerUpdate()
	{
		new UIJob("Update last values view") {
			@Override
			public IStatus runInUIThread(IProgressMonitor monitor)
			{
				synchronized(alarmList)
				{
					alarmViewer.refresh();
				}
				return Status.OK_STATUS;
			}
		}.schedule();
	}*/
	

	/**
	 * Create pop-up menu for alarm list
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
		Menu menu = menuMgr.createContextMenu(dataViewer.getControl());
		dataViewer.getControl().setMenu(menu);

		// Register menu for extension.
		if (viewPart != null)
			viewPart.getSite().registerContextMenu(menuMgr, dataViewer);
	}
	

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}
}
