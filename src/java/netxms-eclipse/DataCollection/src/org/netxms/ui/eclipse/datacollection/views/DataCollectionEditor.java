/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.views;

import java.util.Iterator;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DCONotificationCallback;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.Template;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.api.DataCollectionObjectEditor;
import org.netxms.ui.eclipse.datacollection.views.helpers.DciComparator;
import org.netxms.ui.eclipse.datacollection.views.helpers.DciFilter;
import org.netxms.ui.eclipse.datacollection.views.helpers.DciLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.DialogData;
import org.netxms.ui.eclipse.tools.ExtendedPropertyDialog;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Data collection configuration view
 * 
 */
public class DataCollectionEditor extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.datacollection.view.data_collection_editor"; //$NON-NLS-1$
	public static final String JOB_FAMILY = "DataCollectionEditorJob"; //$NON-NLS-1$

	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_ORIGIN = 1;
	public static final int COLUMN_DESCRIPTION = 2;
	public static final int COLUMN_PARAMETER = 3;
	public static final int COLUMN_DATATYPE = 4;
	public static final int COLUMN_INTERVAL = 5;
	public static final int COLUMN_RETENTION = 6;
	public static final int COLUMN_STATUS = 7;
	public static final int COLUMN_THRESHOLD = 8;
	public static final int COLUMN_TEMPLATE = 9;

	private Composite content;
	private FilterText filterText;
	private SortableTableViewer viewer;
	private NXCSession session;
	private IDialogSettings settings;
	private AbstractObject object;
	private DataCollectionConfiguration dciConfig = null;
	private DciFilter filter;
	private Action actionCreateItem;
	private Action actionCreateTable;
	private Action actionEdit;
	private Action actionDelete;
	private Action actionCopy;
	private Action actionMove;
	private Action actionConvert;
	private Action actionDuplicate;
	private Action actionActivate;
	private Action actionDisable;
	private Action actionShowFilter;
	private RefreshAction actionRefresh;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;
	private boolean hideModificationWarnings;
	private DCONotificationCallback notificationCallback;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = (NXCSession)ConsoleSharedData.getSession();
		AbstractObject obj = session.findObjectById(Long.parseLong(site.getSecondaryId()));
		settings = Activator.getDefault().getDialogSettings();
		object = ((obj != null) && ((obj instanceof DataCollectionTarget) || (obj instanceof Template))) ? obj : null;
		setPartName(Messages.get().DataCollectionEditor_PartNamePrefix + ((object != null) ? object.getObjectName() : Messages.get().DataCollectionEditor_Error));
	
		notificationCallback = new DCONotificationCallback() {
         
         @Override
         public void notifyDCOChange()
         {
            getSite().getShell().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(dciConfig.getItems());
               }
            }); 
         }
      };
	}

	/**
    * @param b
    * @param defval
    * @return
    */
	private boolean getBooleanFromSettings(String name, boolean defval)
   {
	   String v = settings.get(name);
      return (v != null) ? Boolean.valueOf(v) : defval;
   }
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		content = new Composite(parent, SWT.NONE);
		content.setLayout(new FormLayout());
		
		// Create filter area
		filterText = new FilterText(content, SWT.NONE);
		filterText.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				onFilterModify();
			}
		});
		
		final String[] names = { Messages.get().DataCollectionEditor_ColID, Messages.get().DataCollectionEditor_ColOrigin, Messages.get().DataCollectionEditor_ColDescription, Messages.get().DataCollectionEditor_ColParameter, Messages.get().DataCollectionEditor_ColDataType, Messages.get().DataCollectionEditor_ColPollingInterval, Messages.get().DataCollectionEditor_ColRetentionTime, Messages.get().DataCollectionEditor_ColStatus, Messages.get().DataCollectionEditor_ColThresholds, Messages.get().DataCollectionEditor_ColTemplate };
		final int[] widths = { 60, 100, 250, 200, 90, 90, 90, 100, 200, 150 };
		viewer = new SortableTableViewer(content, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new DciLabelProvider());
		viewer.setComparator(new DciComparator((DciLabelProvider)viewer.getLabelProvider()));
		filter = new DciFilter();
		viewer.addFilter(filter);
		WidgetHelper.restoreTableViewerSettings(viewer, settings, "DataCollectionEditor"); //$NON-NLS-1$
		
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionEdit.setEnabled(selection.size() == 1);
					actionDelete.setEnabled(selection.size() > 0);
					actionCopy.setEnabled(selection.size() > 0);
					actionMove.setEnabled(selection.size() > 0);
					actionConvert.setEnabled(selection.size() > 0);
					actionDuplicate.setEnabled(selection.size() > 0);
					
					Iterator<?> it = selection.iterator();
					boolean canActivate = false;
					boolean canDisable = false;
					while(it.hasNext() && (!canActivate || !canDisable))
					{
						DataCollectionObject dci = (DataCollectionObject)it.next();
						if (dci.getStatus() != DataCollectionObject.ACTIVE)
							canActivate = true;
						if (dci.getStatus() != DataCollectionObject.DISABLED)
							canDisable = true;
					}
					actionActivate.setEnabled(canActivate);
					actionDisable.setEnabled(canDisable);
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
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, settings, "DataCollectionEditor"); //$NON-NLS-1$
	         settings.put("DataCollectionEditor.hideModificationWarnings", hideModificationWarnings);
			}
		});

		// Setup layout
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(filterText);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		viewer.getTable().setLayoutData(fd);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		filterText.setLayoutData(fd);
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            actionShowFilter.setChecked(false);
         }
      });

		// Request server to open data collection configuration
		new ConsoleJob(Messages.get().DataCollectionEditor_OpenJob_Title + object.getObjectName(), this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				dciConfig = session.openDataCollectionConfiguration(object.getObjectId(), notificationCallback);
				dciConfig.setUserData(viewer);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(dciConfig.getItems());
					}
				});
			}

			@Override
			protected void jobFailureHandler()
			{
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						DataCollectionEditor.this.getViewSite().getPage().hideView(DataCollectionEditor.this);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().DataCollectionEditor_OpenJob_Error + object.getObjectName();
			}
		}.start();
		
		// Set initial focus to filter input line
		if (actionShowFilter.isChecked())
			filterText.setFocus();
		else
			enableFilter(false);	// Will hide filter area correctly
		
		hideModificationWarnings = getBooleanFromSettings("DataCollectionEditor.hideModificationWarnings", false);
		
		activateContext();
	}
	
	/**
	 * Activate context
	 */
	private void activateContext()
	{
		IContextService contextService = (IContextService)getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.datacollection.context.LastValues"); //$NON-NLS-1$
		}
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
		manager.add(actionShowFilter);
		manager.add(new Separator());
		manager.add(actionCreateItem);
		manager.add(actionCreateTable);
		manager.add(actionEdit);
		manager.add(actionDelete);
		manager.add(actionCopy);
		manager.add(actionMove);
		manager.add(actionConvert);
		manager.add(actionDuplicate);
		manager.add(new Separator());
		manager.add(actionActivate);
		manager.add(actionDisable);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_SECONDARY));
		manager.add(new Separator());
		manager.add(actionRefresh);
		manager.add(actionExportAllToCsv);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
      manager.add(actionShowFilter);
		manager.add(actionCreateItem);
		manager.add(actionRefresh);
		manager.add(actionExportAllToCsv);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager manager)
	{
		manager.add(actionCreateItem);
		manager.add(actionCreateTable);
		manager.add(actionEdit);
		manager.add(actionDelete);
		manager.add(actionCopy);
		manager.add(actionMove);
		manager.add(actionConvert);
		manager.add(actionDuplicate);
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(actionActivate);
		manager.add(actionDisable);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_SECONDARY));
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
		
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				viewer.setInput(dciConfig.getItems());
			}
		};

		actionCreateItem = new Action(Messages.get().DataCollectionEditor_NewParam, Activator.getImageDescriptor("icons/new.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				createItem();
			}
		};

		actionCreateTable = new Action(Messages.get().DataCollectionEditor_NewTable) {
			@Override
			public void run()
			{
				createTable();
			}
		};

		actionEdit = new Action(Messages.get().DataCollectionEditor_ActionEdit, SharedIcons.EDIT) {
			@Override
			public void run()
			{
				editSelectedObject();
			}
		};
		actionEdit.setText(Messages.get().DataCollectionEditor_Edit);
		actionEdit.setImageDescriptor(Activator.getImageDescriptor("icons/edit.png")); //$NON-NLS-1$
		actionEdit.setEnabled(false);

		actionDelete = new Action(Messages.get().DataCollectionEditor_Delete, Activator.getImageDescriptor("icons/delete.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				deleteItems();
			}
		};
		actionDelete.setEnabled(false);

		actionCopy = new Action(Messages.get().DataCollectionEditor_Copy) {
			@Override
			public void run()
			{
				copyItems(false);
			}
		};
		actionCopy.setEnabled(false);

		actionMove = new Action(Messages.get().DataCollectionEditor_Move) {
			@Override
			public void run()
			{
				copyItems(true);
			}
		};
		actionMove.setEnabled(false);

		actionConvert = new Action(Messages.get().DataCollectionEditor_Convert) {
			@Override
			public void run()
			{
				convertToTemplate();
			}
		};
		actionConvert.setEnabled(false);

		actionDuplicate = new Action(Messages.get().DataCollectionEditor_Duplicate) {
			@Override
			public void run()
			{
				duplicateItems();
			}
		};
		actionDuplicate.setEnabled(false);

		actionActivate = new Action(Messages.get().DataCollectionEditor_Activate, Activator.getImageDescriptor("icons/active.gif")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				setItemStatus(DataCollectionObject.ACTIVE);
            actionActivate.setEnabled(false);
            actionDisable.setEnabled(true);
			}
		};
		actionActivate.setEnabled(false);

		actionDisable = new Action(Messages.get().DataCollectionEditor_Disable, Activator.getImageDescriptor("icons/disabled.gif")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				setItemStatus(DataCollectionObject.DISABLED);
            actionActivate.setEnabled(true);
            actionDisable.setEnabled(false);
			}
		};
		actionDisable.setEnabled(false);

		actionShowFilter = new Action(Messages.get().DataCollectionEditor_ShowFilter, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				enableFilter(actionShowFilter.isChecked());
			}
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(getBooleanFromSettings("DataCollectionEditor.showFilter", true));
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter"); //$NON-NLS-1$
		handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
		
		actionExportToCsv = new ExportToCsvAction(this, viewer, true); 
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
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
		if (dciConfig != null)
		{
			new ConsoleJob(Messages.get().DataCollectionEditor_UnlockJob_Title + object.getObjectName(), null, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					dciConfig.close();
					dciConfig = null;
				}

				@Override
				protected String getErrorMessage()
				{
					return Messages.get().DataCollectionEditor_UnlockJob_Error + object.getObjectName();
				}
			}.start();
		}
		super.dispose();
	}

	/**
	 * Change status for selected items
	 * 
	 * @param newStatus New status
	 */
	private void setItemStatus(final int newStatus)
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() <= 0)
			return;
		
		new ConsoleJob(Messages.get().DataCollectionEditor_ChStatusJob_Title + object.getObjectName(), this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final long[] itemList = new long[selection.size()];
				int pos = 0;
				for(Object dci : selection.toList())
				{
					itemList[pos++] = ((DataCollectionObject)dci).getId();
				}
				dciConfig.setObjectStatus(itemList, newStatus);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						for(Object dci : selection.toList())
						{
							((DataCollectionObject)dci).setStatus(newStatus);
							viewer.update(dci, null);
                     new DataCollectionObjectEditor((DataCollectionObject)dci).modify();
						}
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().DataCollectionEditor_ChStatusJob_Error + object.getObjectName();
			}
		}.start();
	}

	/**
	 * Delete currently selected DCIs
	 */
	private void deleteItems()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() <= 0)
			return;
		
		if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.get().DataCollectionEditor_DeleteConfirmTitle,
		                               Messages.get().DataCollectionEditor_DeleteConfirmText))
			return;
		
		new ConsoleJob(Messages.get().DataCollectionEditor_DeleteJob_Title + object.getObjectName(), this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(Object dci : selection.toList())
				{
					dciConfig.deleteObject(((DataCollectionObject)dci).getId());
				}
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(dciConfig.getItems());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().DataCollectionEditor_DeleteJob_Error + object.getObjectName();
			}
		}.start();
	}
	
	/**
	 * Create new data collection item
	 */
	private void createItem()
	{
      DataCollectionItem dci = new DataCollectionItem(dciConfig, 0);
      if((object instanceof AbstractNode) && !((AbstractNode)object).hasAgent())
      {
         if(((AbstractNode)object).hasSnmpAgent())
         {
            dci.setOrigin(DataCollectionObject.SNMP);
         }
         else
         {
            dci.setOrigin(DataCollectionObject.INTERNAL);
         }
      }
      viewer.add(dci);
      viewer.setSelection(new StructuredSelection(dci), true);
      actionEdit.run();
	}
	
	/**
	 * Create new data collection table
	 */
	private void createTable()
	{		
		DataCollectionTable dci = new DataCollectionTable(dciConfig, 0);
      if((object instanceof AbstractNode) && !((AbstractNode)object).hasAgent())
      {
         if(((AbstractNode)object).hasSnmpAgent())
         {
            dci.setOrigin(DataCollectionObject.SNMP);
         }
         else
         {
            dci.setOrigin(DataCollectionObject.INTERNAL);
         }
      }
      viewer.add(dci);
		viewer.setSelection(new StructuredSelection(dci), true);
		actionEdit.run();
	}
	
	/**
	 * Edit selected object
	 */
	private void editSelectedObject()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		DataCollectionObject dco = (DataCollectionObject)selection.getFirstElement();
		
		DialogData data = null;
		if (!hideModificationWarnings && dco.getTemplateId() != 0)
		{
		   String message = DataCollectionObjectEditor.createModificationWarningMessage(dco);
		   if (message != null)
		   {
            data = MessageDialogHelper.openWarningWithCheckbox(getSite().getShell(), "Warning", "Don't show this message again", message);
            hideModificationWarnings = data.getSaveSelection();
		   }
		}
		
		if ((data == null) || data.isOkPressed())
		{
         ExtendedPropertyDialog dlg = ExtendedPropertyDialog.createDialogOn(getSite().getShell(), null, dco, ""); //$NON-NLS-1$
         dlg.createAllPages();
         int result = dlg.open();
         if (((DataCollectionObject)selection.getFirstElement()).isNewItem() && result != SWT.OK)
         {
            viewer.remove(selection.getFirstElement());
         }
		}
	}
	
	/**
	 * Duplicate selected item(s)
	 */
	private void duplicateItems()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		Iterator<?> it = selection.iterator();
		final long[] dciList = new long[selection.size()];
		for(int i = 0; (i < dciList.length) && it.hasNext(); i++)
			dciList[i] = ((DataCollectionObject)it.next()).getId();
		
		new ConsoleJob(Messages.get().DataCollectionEditor_DupJob_Title + object.getObjectName(), this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				dciConfig.copyObjects(dciConfig.getNodeId(), dciList);
				dciConfig.close();
				dciConfig.open(notificationCallback);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(dciConfig.getItems());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().DataCollectionEditor_DupJob_Error + object.getObjectName();
			}
		}.start();
	}
	
	/**
	 * Copy items to another node
	 */
	private void copyItems(final boolean doMove)
	{
		final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), null, ObjectSelectionDialog.createNodeAndTemplateSelectionFilter(true));
		if (dlg.open() != Window.OK)
			return;

		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		Iterator<?> it = selection.iterator();
		final long[] dciList = new long[selection.size()];
		for(int i = 0; (i < dciList.length) && it.hasNext(); i++)
			dciList[i] = ((DataCollectionObject)it.next()).getId();
		
		new ConsoleJob(Messages.get().DataCollectionEditor_CopyJob_Title + object.getObjectName(), this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(AbstractObject o : dlg.getSelectedObjects(AbstractNode.class))
					dciConfig.copyObjects(o.getObjectId(), dciList);
				for(AbstractObject o : dlg.getSelectedObjects(Template.class))
					dciConfig.copyObjects(o.getObjectId(), dciList);
				for(AbstractObject o : dlg.getSelectedObjects(Cluster.class))
					dciConfig.copyObjects(o.getObjectId(), dciList);
				for(AbstractObject o : dlg.getSelectedObjects(MobileDevice.class))
					dciConfig.copyObjects(o.getObjectId(), dciList);
            for(AbstractObject o : dlg.getSelectedObjects(Sensor.class))
               dciConfig.copyObjects(o.getObjectId(), dciList);
				if (doMove)
				{
					for(long id : dciList)
						dciConfig.deleteObject(id);
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							viewer.setInput(dciConfig.getItems());
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().DataCollectionEditor_CopyJob_Error + object.getObjectName();
			}
		}.start();
	}

	/**
	 * Convert selected item(s) to template items
	 */
	private void convertToTemplate()
	{
		final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), null, ObjectSelectionDialog.createTemplateSelectionFilter());
		dlg.showFilterToolTip(false);
		if (dlg.open() != Window.OK)
			return;
		
		AbstractObject[] objects = dlg.getSelectedObjects(Template.class);
		if (objects.length == 0)
			return;
		final Template template = (Template)objects[0];
		
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		Iterator<?> it = selection.iterator();
		final long[] dciList = new long[selection.size()];
		for(int i = 0; (i < dciList.length) && it.hasNext(); i++)
			dciList[i] = ((DataCollectionObject)it.next()).getId();
		
		new ConsoleJob(Messages.get().DataCollectionEditor_ConvertJob_TitlePrefix + object.getObjectName() + Messages.get().DataCollectionEditor_ConvertJob_TitleSuffix, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				monitor.beginTask(Messages.get().DataCollectionEditor_ConvertJob_TaskName, 4);
				
				boolean needApply = true;
				for(long id : template.getChildIdList())
				{
					if (id == dciConfig.getNodeId())
					{
						needApply = false;
						break;
					}
				}
				monitor.worked(1);
				
				dciConfig.copyObjects(template.getObjectId(), dciList);
				for(long id : dciList)
					dciConfig.deleteObject(id);
				dciConfig.close();
				monitor.worked(1);
						
				if (needApply)
				{
					boolean success = false;
					int retries = 5;
					do
					{
						try
						{
							session.applyTemplate(template.getObjectId(), dciConfig.getNodeId());
							success = true;
						}
						catch(NXCException e)
						{
							if (e.getErrorCode() != RCC.COMPONENT_LOCKED)
								throw e;
							Thread.sleep(200);
						}
						retries--;
					} while(!success && (retries > 0));
				}
				monitor.worked(1);
				
				boolean success = false;
				int retries = 5;
				do
				{
					try
					{
						Thread.sleep(500);
						dciConfig.open(notificationCallback);
						success = true;
					}
					catch(NXCException e)
					{
						if (e.getErrorCode() != RCC.COMPONENT_LOCKED)
							throw e;
					}
					retries--;
				} while(!success && (retries > 0));
				
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(dciConfig.getItems());
					}
				});
				monitor.done();
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().DataCollectionEditor_ConvertJob_ErrorPrefix + object.getObjectName() + Messages.get().DataCollectionEditor_ConvertJob_ErrorSuffix;
			}
		}.start();
	}

	/**
	 * Enable or disable filter
	 * 
	 * @param enable New filter state
	 */
	private void enableFilter(boolean enable)
	{
		filterText.setVisible(enable);
		FormData fd = (FormData)viewer.getTable().getLayoutData();
		fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
		content.layout();
		if (enable)
		{
			filterText.setFocus();
		}
		else
		{
			filterText.setText(""); //$NON-NLS-1$
			onFilterModify();
		}
      settings.put("DataCollectionEditor.showFilter", enable);
	}

	/**
	 * Handler for filter modification
	 */
	private void onFilterModify()
	{
		final String text = filterText.getText();
		filter.setFilterString(text);
		viewer.refresh(false);
	}
}
