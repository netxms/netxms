/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
package org.netxms.ui.eclipse.filemanager.views;

import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.util.LocalSelectionTransfer;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TreePath;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.jface.viewers.ViewerDropAdapter;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.DND;
import org.eclipse.swt.dnd.DragSourceAdapter;
import org.eclipse.swt.dnd.DragSourceEvent;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.dnd.TransferData;
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
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.ProgressListener;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerFile;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.filemanager.Activator;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.filemanager.dialogs.RenameFileDialog;
import org.netxms.ui.eclipse.filemanager.dialogs.StartClientToServerFileUploadDialog;
import org.netxms.ui.eclipse.filemanager.views.helpers.AgentFileComparator;
import org.netxms.ui.eclipse.filemanager.views.helpers.AgentFileFilter;
import org.netxms.ui.eclipse.filemanager.views.helpers.ServerFileLabelProvider;
import org.netxms.ui.eclipse.filemanager.views.helpers.ViewAgentFilesProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Editor for server files
 */
public class ViewAgentFiles extends ViewPart implements SessionListener
{
   public static final String ID = "org.netxms.ui.eclipse.usermanager.view.agent_files"; //$NON-NLS-1$
   
   private static final String TABLE_CONFIG_PREFIX = "AgentFilesEditor"; //$NON-NLS-1$

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_SIZE = 2;
   public static final int COLUMN_MODIFYED = 3;
	
   private boolean filterEnabled = false;
   private Composite content;
   private ServerFile[] files;
   private AgentFileFilter filter;
   private FilterText filterText;
   private SortableTreeViewer viewer;
   private NXCSession session;
   private Action actionRefreshAll;
   private Action actionUpload;
   private Action actionDelete;
   private Action actionRename;
   private Action actionRefreshDirectory;
   private Action actionShowFilter;
   private long objectId = 0;

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      
      session = (NXCSession)ConsoleSharedData.getSession();
      objectId = Long.parseLong(site.getSecondaryId());
      AbstractObject object = session.findObjectById(objectId);
      setPartName("File list " + ((object != null) ? object.getObjectName() : ("[" + Long.toString(objectId) + "]")));  //$NON-NLS-1$//$NON-NLS-2$
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
		filterText.setCloseAction(new Action() {
			@Override
			public void run()
			{
				enableFilter(false);
			}
		});
		
		final String[] columnNames = {Messages.get().ViewServerFile_FileName, Messages.get().ViewServerFile_FileType, Messages.get().ViewServerFile_FileSize, Messages.get().ViewServerFile_ModificationDate};
		final int[] columnWidths = { 300, 150, 300, 300 };
		viewer = new SortableTreeViewer(content, columnNames, columnWidths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		WidgetHelper.restoreTreeViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ViewAgentFilesProvider());
		viewer.setLabelProvider(new ServerFileLabelProvider());
		viewer.setComparator(new AgentFileComparator());
		filter = new AgentFileFilter();
		viewer.addFilter(filter);
		viewer.addSelectionChangedListener(new ISelectionChangedListener()
		{
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionDelete.setEnabled(selection.size() > 0);
				}
			}
		});
		viewer.getTree().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTreeViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
			}
		});
		enableDragSupport();
		enableDropSupport();
		
		// Setup layout
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(filterText);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		viewer.getTree().setLayoutData(fd);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		filterText.setLayoutData(fd);
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		session.addListener(this);
		
		filterText.setCloseAction(actionShowFilter);		
		
		// Set initial focus to filter input line
		if (filterEnabled)
			filterText.setFocus();
		else
			enableFilter(false);	// Will hide filter area correctly
		
		refreshFileList();
	}
	
	/**
    * Enable drag support in object tree
    */
   public void enableDragSupport()
   {
      Transfer[] transfers = new Transfer[] { LocalSelectionTransfer.getTransfer() };
      viewer.addDragSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new DragSourceAdapter() {
         @Override
         public void dragStart(DragSourceEvent event)
         {
            LocalSelectionTransfer.getTransfer().setSelection(viewer.getSelection());
            event.doit = true;
         }

         @Override
         public void dragSetData(DragSourceEvent event)
         {
            event.data = LocalSelectionTransfer.getTransfer().getSelection();
         }
      });
   }
   
   /**
    * Enable drop support in object tree
    */
   public void enableDropSupport()//SubtreeType infrastructure
   {      
      final Transfer[] transfers = new Transfer[] { LocalSelectionTransfer.getTransfer() };
      viewer.addDropSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new ViewerDropAdapter(viewer) {
         
         @Override
         public boolean performDrop(Object data) 
         {
            IStructuredSelection selection = (IStructuredSelection)data;
            List<?> movableSelection = selection.toList();
            for (int i = 0; i < movableSelection.size(); i++)
            {
               ServerFile movableObject = (ServerFile)movableSelection.get(i);     
               
               moveFile((ServerFile)getCurrentTarget(), movableObject);
               
            }
            return true;
         }

         @Override
         public boolean validateDrop(Object target, int operation, TransferData transferType)
         {
            if ((target == null) || !LocalSelectionTransfer.getTransfer().isSupportedType(transferType))
               return false;

            IStructuredSelection selection = (IStructuredSelection)LocalSelectionTransfer.getTransfer().getSelection();
            if (selection.isEmpty())
               return false;
            
            for(final Object object : selection.toList())
            {
               if(!(object instanceof ServerFile))
                  return false;
            }
            if(!(target instanceof ServerFile))
            {
               return false;
            }
            else
            {
               if(!((ServerFile)target).isDirectory())
               {
                  return false;
               }
            }
            return true;
         }
      });
   }

	/**
	 * Create actions
	 */
	private void createActions()
	{
		final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
		actionRefreshDirectory = new Action("Refresh this folder") {
			@Override
			public void run()
			{
			   refreshFileOrDirectory();
			}
		};
		
		actionRefreshAll = new RefreshAction(this) {
         @Override
         public void run()
         {
            refreshFileOrDirectory();
         }
      };
		
		actionUpload = new Action("Upload file on agent") {
			@Override
			public void run()
			{
				createFile();
			}
		};
		actionUpload.setImageDescriptor(SharedIcons.ADD_OBJECT);
		
		actionDelete = new Action("Delete file") {
			@Override
			public void run()
			{
				deleteFile();
			}
		};
		actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);
      
      actionRename = new Action("Rename file") {
         @Override
         public void run()
         {
            renameFile();
         }
      };
		
		actionShowFilter = new Action(Messages.get().ViewServerFile_ShowFilterAction, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				enableFilter(!filterEnabled);
				actionShowFilter.setChecked(filterEnabled);
			}
	    };
	    actionShowFilter.setChecked(filterEnabled);
	    actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter"); //$NON-NLS-1$
		final ActionHandler showFilterHandler = new ActionHandler(actionShowFilter);
		handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), showFilterHandler);
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
		//manager.add(actionUpload);
		//manager.add(actionDelete);
		manager.add(new Separator());
		manager.add(actionRefreshAll);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local tool bar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionShowFilter);
		manager.add(new Separator());
		//manager.add(actionUpload);
		//manager.add(actionDelete);
		manager.add(new Separator());
		manager.add(actionRefreshAll);
	}
	
	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
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
		mgr.add(actionShowFilter);
		mgr.add(new Separator());
		mgr.add(actionUpload);
		mgr.add(actionDelete);
      mgr.add(actionRename);
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
      mgr.add(actionRefreshDirectory);
	}

	/**
	 * Refresh file list
	 */
	private void refreshFileList()
	{
	   new ConsoleJob(Messages.get().SelectServerFileDialog_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				files = session.listAgentFiles(null, "/", objectId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(files);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().SelectServerFileDialog_JobError;
			}
		}.start();
	}
	
	/**
    * Refresh file list
    */
   private void refreshFileOrDirectory()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      
      final Object[] objects = selection.toArray();
      
      new ConsoleJob(Messages.get().SelectServerFileDialog_JobTitle, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               if(!((ServerFile)objects[i]).isDirectory())
                  objects[i] = ((ServerFile)objects[i]).getParent();
               
               final ServerFile sf = ((ServerFile)objects[i]);
               sf.setChildren(session.listAgentFiles(sf, sf.getFullName(), objectId));
               
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.refresh(sf);
                  }
               });
            }               
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().SelectServerFileDialog_JobError;
         }
      }.start();
   }
	
	/**
	 * Create new file
	 */
	private void createFile()
	{
		final StartClientToServerFileUploadDialog dlg = new StartClientToServerFileUploadDialog(getSite().getShell());
		if (dlg.open() == Window.OK)
		{
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			new ConsoleJob(Messages.get().UploadFileToServer_JobTitle, null, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(final IProgressMonitor monitor) throws Exception
				{
					session.uploadFileToServer(dlg.getLocalFile(), dlg.getRemoteFileName(), new ProgressListener() {
						private long prevWorkDone = 0;
						
						@Override
						public void setTotalWorkAmount(long workTotal)
						{
							monitor.beginTask(Messages.get().UploadFileToServer_TaskNamePrefix + dlg.getLocalFile().getAbsolutePath(), (int)workTotal);
						}
						
						@Override
						public void markProgress(long workDone)
						{
							monitor.worked((int)(workDone - prevWorkDone));
							prevWorkDone = workDone;
						}
					});
					monitor.done();
				}
				
				@Override
				protected String getErrorMessage()
				{
					return Messages.get().UploadFileToServer_JobError;
				}
			}.start();
		}
	}
	
	/**
	 * Delete selected file
	 */
	private void deleteFile()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.isEmpty())
			return;
		
		if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.get().ViewServerFile_DeleteConfirmation, Messages.get().ViewServerFile_DeletAck))
			return;
		
		final Object[] objects = selection.toArray();
		new ConsoleJob(Messages.get().ViewServerFile_DeletFileFromServerJob, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ViewServerFile_ErrorDeleteFileJob;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(int i = 0; i < objects.length; i++)
				{
				   final ServerFile sf = (ServerFile)objects[i];
					session.deleteAgentFile(objectId, sf.getFullName());
					
					runInUIThread(new Runnable() {
	               @Override
	               public void run()
	               {
	                  sf.getParent().removeChield(sf);
	                  viewer.refresh(sf.getParent());
	               }
	            });
				}
			}
		}.start();
	}
	
	/**
    * Rename selected file
    */
   private void renameFile()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;      
      
      
      final Object[] objects = selection.toArray();

      RenameFileDialog dlg = new RenameFileDialog(getSite().getShell(), ((ServerFile)objects[0]).getName());
      if(dlg.open() != Window.OK)
         return;
      
      final String newName = dlg.getNewName();
      
      new ConsoleJob(Messages.get().ViewServerFile_DeletFileFromServerJob, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
         @Override
         protected String getErrorMessage()
         {
            return "Error while rename file job";
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final ServerFile sf = (ServerFile)objects[0];
            session.renameAgentFile(objectId, sf.getFullName(), sf.getParent().getFullName()+"/"+newName);
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  sf.setName(newName);
                  viewer.refresh(sf, true);
               }
            });
         }
      }.start();
   }
   
   /**
    * Move selected file
    */
   private void moveFile(final ServerFile target, final ServerFile object)
   {               
      new ConsoleJob(Messages.get().ViewServerFile_DeletFileFromServerJob, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
         @Override
         protected String getErrorMessage()
         {
            return "Error while move file job";
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.renameAgentFile(objectId, object.getFullName(), target.getFullName()+"/"+object.getName());
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  object.getParent().removeChield(object);                  
                  viewer.refresh(object.getParent(), true);
                  object.setParent(target);
                  target.addChield(object);               
                  viewer.refresh(object.getParent(), true);                  
               }
            });
         }
      }.start();
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
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
	   if(n.getCode() == NXCNotification.FILE_LIST_CHANGED)
	   {
	      refreshFileList();
	   }
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		session.removeListener(this);
		super.dispose();
	}
	
	/**
	 * Enable or disable filter
	 * 
	 * @param enable New filter state
	 */
	private void enableFilter(boolean enable)
	{
		filterEnabled = enable;
		filterText.setVisible(filterEnabled);
		FormData fd = (FormData)viewer.getTree().getLayoutData();
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
