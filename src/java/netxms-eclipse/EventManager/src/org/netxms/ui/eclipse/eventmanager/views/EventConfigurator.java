/**
 * 
 */
package org.netxms.ui.eclipse.eventmanager.views;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

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
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.eventmanager.Activator;
import org.netxms.ui.eclipse.eventmanager.EventTemplateComparator;
import org.netxms.ui.eclipse.eventmanager.EventTemplateLabelProvider;
import org.netxms.ui.eclipse.eventmanager.dialogs.EditEventTemplateDialog;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
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
	public static final int COLUMN_CODE = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_SEVERITY = 2;
	public static final int COLUMN_FLAGS = 3;
	public static final int COLUMN_MESSAGE = 4;
	public static final int COLUMN_DESCRIPTION = 5;

	private HashMap<Long, EventTemplate> eventTemplates;
	private TableViewer viewer;
	private boolean databaseLocked = false;
	private Action actionNew;
	private Action actionEdit;
	private Action actionDelete;
	private RefreshAction actionRefresh;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { "Code", "Name", "Severity", "Flags", "Message", "Description" };
		final int[] widths = { 70, 200, 90, 50, 400, 400 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new EventTemplateLabelProvider());
		viewer.setComparator(new EventTemplateComparator());
		viewer.addSelectionChangedListener(new ISelectionChangedListener()
		{
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionEdit.setEnabled(selection.size() == 1);
					actionDelete.setEnabled(selection.size() > 0);
				}
			}
		});
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				actionEdit.run();
			}
		});

		makeActions();
		contributeToActionBars();
		createPopupMenu();

		// Request server to lock event database, and on success refresh view
		Job job = new Job("Open event configuration")
		{
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;

				try
				{
					final NXCSession session = NXMCSharedData.getInstance().getSession();
					session.lockEventConfiguration();
					databaseLocked = true;
					final List<EventTemplate> list = session.getEventTemplates();
					new UIJob("Update event list")
					{
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							EventConfigurator.this.updateLocalCopy(list);
							viewer.setInput(list.toArray());
							return Status.OK_STATUS;
						}
					}.schedule();
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID,
								           (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
											  "Cannot open event configuration for modification: " + e.getMessage(), null);
					new UIJob("Close event configurator")
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
		job.setUser(true);
		runJob(job);
	}
	
	/**
	 * Update local copy of event template list.
	 * 
	 * @param list List of event templates received from server
	 */
	private void updateLocalCopy(List<EventTemplate> list)
	{
		eventTemplates = new HashMap<Long, EventTemplate>(list.size());
		for(final EventTemplate t: list)
		{
			eventTemplates.put(t.getCode(), t);
		}
	}

	/**
	 * Run job via site service
	 * 
	 * @param job Job to run
	 */
	private void runJob(final Job job)
	{
		IWorkbenchSiteProgressService siteService = (IWorkbenchSiteProgressService) getSite().getAdapter(IWorkbenchSiteProgressService.class);
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
		manager.add(actionNew);
		manager.add(actionDelete);
		manager.add(actionEdit);
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
		manager.add(actionNew);
		manager.add(actionEdit);
		manager.add(actionDelete);
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
				Job job = new Job("Refresh event template list") {
					@Override
					protected IStatus run(IProgressMonitor monitor)
					{
						IStatus status;
						try
						{
							final List<EventTemplate> list = NXMCSharedData.getInstance().getSession().getEventTemplates();
							new UIJob("Update event list")
							{
								@Override
								public IStatus runInUIThread(IProgressMonitor monitor)
								{
									EventConfigurator.this.updateLocalCopy(list);
									viewer.setInput(list.toArray());
									return Status.OK_STATUS;
								}
							}.schedule();
							status = Status.OK_STATUS;
						}
						catch(Exception e)
						{
							status = new Status(Status.ERROR, Activator.PLUGIN_ID,
										           (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
													  "Cannot reload event configuration: " + e.getMessage(), null);
						}
						return status;
					}
				};
				job.setUser(true);
				runJob(job);
			}
		};

		actionNew = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				createNewEventTemplate();
			}
		};
		actionNew.setText("&New...");
		actionNew.setImageDescriptor(Activator.getImageDescriptor("icons/new.png"));

		actionEdit = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				editEventTemplate();
			}
		};
		actionEdit.setText("&Edit...");
		actionEdit.setImageDescriptor(Activator.getImageDescriptor("icons/edit.png"));
		actionEdit.setEnabled(false);

		actionDelete = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				deleteEventTemplate();
			}
		};
		actionDelete.setText("&Delete");
		actionDelete.setImageDescriptor(Activator.getImageDescriptor("icons/delete.png"));
		actionDelete.setEnabled(false);
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
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(actionDelete);
		mgr.add(new Separator());
		mgr.add(actionEdit);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/**
	 * Create new event template
	 */
	protected void createNewEventTemplate()
	{
		final NXCSession session = NXMCSharedData.getInstance().getSession();
		
		Job job = new Job("Create new event template") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					final long code = session.generateEventCode();
					new UIJob("Edit created event template") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							final EventTemplate object = new EventTemplate(code);
							EditEventTemplateDialog dlg = new EditEventTemplateDialog(getSite().getShell(), object, false);
							if (dlg.open() == Window.OK)
							{
								finishCreation(object);
							}
							return Status.OK_STATUS;
						}
					}.schedule();
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID,
								(e instanceof NXCException) ? ((NXCException) e).getErrorCode() : 0,
								"Cannot get code for new event template: " + e.getMessage(), null);
				}
				return status;
			}
		};
		job.setUser(true);
		runJob(job);
	}
	
	/**
	 * Finish creation of new event template object (called after user press OK in properties dialog).
	 * Called in UI thread.
	 * 
	 * @param object New event template object
	 */
	private void finishCreation(final EventTemplate object)
	{
		Job job = new Job("Finish event template creation") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					final NXCSession session = NXMCSharedData.getInstance().getSession();
					session.modifyEventTemplate(object);
					new UIJob("Update event template list") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							eventTemplates.put(object.getCode(), object);
							viewer.setInput(eventTemplates.values().toArray());
							viewer.setSelection(new StructuredSelection(object), true);
							return Status.OK_STATUS;
						}
					}.schedule();
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID,
								(e instanceof NXCException) ? ((NXCException) e).getErrorCode() : 0,
								"Cannot create event template: " + e.getMessage(), null);
				}
				return status;
			}
		};
		job.setUser(true);
		runJob(job);
	}

	/**
	 * Edit currently selected event template
	 */
	protected void editEventTemplate()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		final EventTemplate object = (EventTemplate)selection.getFirstElement();
		final EventTemplate originalObject = new EventTemplate(object);
		EditEventTemplateDialog dlg = new EditEventTemplateDialog(getSite().getShell(), object, false);
		if (dlg.open() == Window.OK)
		{
			Job job = new Job("Update event template") {
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;
					
					try
					{
						final NXCSession session = NXMCSharedData.getInstance().getSession();
						session.modifyEventTemplate(object);
						new UIJob("Update event template list") {
							@Override
							public IStatus runInUIThread(IProgressMonitor monitor)
							{
								viewer.update(object, null);
								return Status.OK_STATUS;
							}
						}.schedule();
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						new UIJob("Restore event template object") {
							@Override
							public IStatus runInUIThread(IProgressMonitor monitor)
							{
								object.setAll(originalObject);
								return Status.OK_STATUS;
							}
						}.schedule();
						status = new Status(Status.ERROR, Activator.PLUGIN_ID,
									(e instanceof NXCException) ? ((NXCException) e).getErrorCode() : 0,
									"Cannot update event template: " + e.getMessage(), null);
					}
					return status;
				}
			};
			job.setUser(true);
			runJob(job);
		}
	}

	/**
	 * Delete selected event templates
	 */
	protected void deleteEventTemplate()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();

		final String message = "Do you really wish to delete selected event template" + ((selection.size() > 1) ? "s?" : "?");
		final Shell shell = getViewSite().getShell();
		if (!MessageDialog.openQuestion(shell, "Confirm event template deletion", message))
		{
			return;
		}
		
		Job job = new Job("Delete event templates") {
			@SuppressWarnings("unchecked")
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;

				try
				{
					final NXCSession session = NXMCSharedData.getInstance().getSession();
					Iterator it = selection.iterator();
					while(it.hasNext())
					{
						Object object = it.next();
						if (object instanceof EventTemplate)
						{
							session.deleteEventTemplate(((EventTemplate)object).getCode());
						}
						else
						{
							throw new NXCException(RCC.INTERNAL_ERROR);
						}
					}
					
					// If event templates was successfully deleted on server,
					// delete them from internal list and refresh viewer
					new UIJob("Update event template list") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							Iterator it = selection.iterator();
							while(it.hasNext())
							{
								EventTemplate evt = (EventTemplate)it.next();
								EventConfigurator.this.eventTemplates.remove(evt.getCode());
							}
							EventConfigurator.this.viewer.setInput(EventConfigurator.this.eventTemplates.values().toArray());
							return Status.OK_STATUS;
						}
					}.schedule();
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID,
							(e instanceof NXCException) ? ((NXCException) e).getErrorCode() : 0,
							"Cannot delete event template: " + e.getMessage(), null);
				}
				return status;
			}

			/* (non-Javadoc)
			 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
			 */
			@Override
			public boolean belongsTo(Object family)
			{
				return family == EventConfigurator.JOB_FAMILY;
			}
		};
		runJob(job);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if (databaseLocked)
		{
			new Job("Unlock event configuration")
			{
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;

					try
					{
						NXMCSharedData.getInstance().getSession().unlockEventConfiguration();
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID,
								(e instanceof NXCException) ? ((NXCException) e).getErrorCode() : 0,
								"Cannot unlock event configuration: " + e.getMessage(), null);
					}
					return status;
				}
			}.schedule();
		}
		super.dispose();
	}
}
