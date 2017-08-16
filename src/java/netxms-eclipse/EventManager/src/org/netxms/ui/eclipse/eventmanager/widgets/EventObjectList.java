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
package org.netxms.ui.eclipse.eventmanager.widgets;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.util.LocalSelectionTransfer;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITreeSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
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
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.events.EventGroup;
import org.netxms.client.events.EventObject;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.eventmanager.Activator;
import org.netxms.ui.eclipse.eventmanager.Messages;
import org.netxms.ui.eclipse.eventmanager.dialogs.EditEventGroupDialog;
import org.netxms.ui.eclipse.eventmanager.dialogs.EditEventTemplateDialog;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventObjectContentProvider;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventObjectComparator;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventObjectFilter;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventObjectLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Event object list widget
 */
public class EventObjectList extends Composite implements SessionListener
{
   public static final String JOB_FAMILY = "EventObjectListJob"; //$NON-NLS-1$
   
   // Columns
   public static final int COLUMN_CODE = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_SEVERITY = 2;
   public static final int COLUMN_FLAGS = 3;
   public static final int COLUMN_MESSAGE = 4;
   public static final int COLUMN_DESCRIPTION = 5;
   
   private HashMap<Long, EventObject> eventObjects;
   private SortableTreeViewer viewer;
   private FilterText filterControl;
   private Action actionNew;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionRemove;
   private Action actionShowFilter;
   private Action actionRefresh;
   private Action actionShowGroups;
   private Action actionNewGroup;
   private NXCSession session;
   private EventObjectFilter filter;
   private boolean filterEnabled;
   private boolean showGroups;
   private ViewPart viewPart ;
   
   /**
    * Constructor for event object list with specific columns to show
    * 
    * @param viewPart the viewPart
    * @param parent parent composite
    * @param style style
    * @param columnsToShow which columns to show
    */
   public EventObjectList(ViewPart viewPart, Composite parent, int style, final String configPrefix)
   {
      this(parent, style, configPrefix, false, true);
      this.viewPart = viewPart;
   }
   
   /**
    * Constructor for event object list
    * 
    * @param viewPart the viewPart
    * @param parent parent composite
    * @param style style
    */
   public EventObjectList(Composite parent, int style, final String configPrefix, boolean isDialog, boolean groupsVisible)
   {
      super(parent, style);
      
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      if (isDialog)
         filterEnabled = true;
      else
         filterEnabled = settings.getBoolean(configPrefix + ".filterEnabled"); //$NON-NLS-1$
      
      if (groupsVisible)
         showGroups = true;
      else
         showGroups = settings.getBoolean(configPrefix + ".showGroups");
      
      session = (NXCSession)ConsoleSharedData.getSession();
      
      parent.setLayout(new FormLayout());
      
      // Create filter area
      filterControl = new FilterText(parent, SWT.NONE, null, !isDialog);
      filterControl.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterControl.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
         }
      });
      
      final String[] names = { Messages.get().EventConfigurator_ColCode, Messages.get().EventConfigurator_ColName, Messages.get().EventConfigurator_ColSeverity, Messages.get().EventConfigurator_ColFlags, Messages.get().EventConfigurator_ColMessage, Messages.get().EventConfigurator_ColDescription };
      final int[] widths = { 70, 200, 90, 50, 400, 400 };
      
      viewer = new SortableTreeViewer(parent, isDialog ? Arrays.copyOfRange(names, 0, 2) : names,
                                              isDialog ? Arrays.copyOfRange(widths, 0, 2) : widths,
                                              0, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      
      WidgetHelper.restoreTreeViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configPrefix);
      viewer.setContentProvider(new EventObjectContentProvider());
      viewer.setLabelProvider(new EventObjectLabelProvider());
      viewer.setComparator(new EventObjectComparator());
      filter = new EventObjectFilter();
      viewer.addFilter(filter);
      viewer.addSelectionChangedListener(new ISelectionChangedListener()
      {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            ITreeSelection selection = (ITreeSelection)event.getSelection();
            if (selection != null)
            {
               if (selection.getPaths().length > 0)
                  actionRemove.setEnabled(selection.getPaths()[0].getParentPath().getLastSegment() != null);
               actionEdit.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
            }
         }
      });
      viewer.getTree().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTreeViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configPrefix);

            IDialogSettings settings = Activator.getDefault().getDialogSettings();
            settings.put(configPrefix + ".filterEnabled", filterEnabled);
            settings.put(configPrefix + ".showGroups", showGroups);
         }
      });
      
      enableDragSupport();
      enableDropSupport();
      
      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterControl);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterControl.setLayoutData(fd);
      
      if (viewPart != null)
         activateContext();
      createActions();
      createPopupMenu();

      refreshView();
      session.addListener(this);
      
      enableFilter(filterEnabled);

      // Set initial focus to filter input line
      if (filterEnabled)
         filterControl.setFocus();
   }
   
   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)viewPart.getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.eventmanager.contexts.EventConfigurator"); //$NON-NLS-1$
      }
   }
   
   /**
    * Refresh view
    */
   private void refreshView()
   {
      new ConsoleJob(Messages.get().EventConfigurator_OpenJob_Title, null, Activator.PLUGIN_ID, JOB_FAMILY) {
         
         /* (non-Javadoc)
          * @see org.netxms.ui.eclipse.jobs.ConsoleJob#getErrorMessage()
          */
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().EventConfigurator_OpenJob_Error;
         }

         /* (non-Javadoc)
          * @see org.netxms.ui.eclipse.jobs.ConsoleJob#runInternal(org.eclipse.core.runtime.IProgressMonitor)
          */
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<EventObject> list = session.getEventObjects();
            runInUIThread(new Runnable() {
               
               /* (non-Javadoc)
                * @see java.lang.Runnable#run()
                */
               @Override
               public void run()
               {
                  eventObjects = new HashMap<Long, EventObject>(list.size());
                  for(final EventObject o: list)
                  {
                     if ((o instanceof EventGroup) && !showGroups)
                        continue;
                     
                     eventObjects.put(o.getCode(), o);
                  }
                  viewer.setInput(eventObjects.values().toArray());
               }
            });
         }
      }.start();
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
    * Enable drop support
    */
   public void enableDropSupport()
   {
      final Transfer[] transfers = new Transfer[] { LocalSelectionTransfer.getTransfer() };
      viewer.addDropSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new ViewerDropAdapter(viewer) {

         @Override
         public boolean performDrop(Object data)
         {
            TreeSelection selection = (TreeSelection)data;
            List<?> movableSelection = selection.toList();
            for(int i = 0; i < movableSelection.size(); i++)
            {
               if (((EventObject)movableSelection.get(i)).getCode() != ((EventGroup)getCurrentTarget()).getCode())
                  ((EventGroup)getCurrentTarget()).addChild(((EventObject)movableSelection.get(i)).getCode());               
            }            
            modifyEventObject((EventGroup)getCurrentTarget(), false);
            
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

            if (!(target instanceof EventGroup))
               return false;

            return true;
         }
         
      });
   }
   
   /* (non-Javadoc)
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.EVENT_TEMPLATE_MODIFIED:
            viewer.getControl().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  EventObject oldObj = eventObjects.get(n.getSubCode());
                  if (oldObj != null)
                  {
                     oldObj.setAll((EventObject)n.getObject());
                     viewer.refresh();
                  }
                  else
                  {
                     eventObjects.put(n.getSubCode(), (EventObject)n.getObject());
                     viewer.setInput(eventObjects.values().toArray());
                  }
               }
            });
            break;
         case SessionNotification.EVENT_TEMPLATE_DELETED:
            viewer.getControl().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  eventObjects.remove(n.getSubCode());
                  viewer.setInput(eventObjects.values().toArray());
               }
            });
            break;
      }
   }
   
   /**
    * Create actions
    */
   private void createActions()
   {
      actionNew = new Action(Messages.get().EventConfigurator_NewEvent, SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewEventTemplate();
         }
      };
      actionNew.setActionDefinitionId("org.netxms.ui.eclipse.eventmanager.commands.new_event_template"); //$NON-NLS-1$

      actionEdit = new Action(Messages.get().EventConfigurator_Properties, SharedIcons.EDIT) { //$NON-NLS-1$
         @Override
         public void run()
         {
            editEventTemplate();
         }
      };
      actionEdit.setEnabled(false);

      actionDelete = new Action(Messages.get().EventConfigurator_Delete, SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteEventTemplate();
         }
      };
      actionDelete.setEnabled(false);

      actionShowFilter = new Action(Messages.get().EventConfigurator_ShowFilter, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      
      actionRemove = new Action("&Remove from group") {
         @Override
         public void run()
         {
            removeFromGroup();
         }
      };
      
      actionShowGroups = new Action("&Show event groups", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            showEventGroups();
         }
      };
      actionShowGroups.setChecked(showGroups);
      
      actionNewGroup = new Action("&Add new event group") {
         @Override
         public void run()
         {
            createNewEventGroup();
         }
      };
      
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(filterEnabled);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.eventmanager.commands.show_filter"); //$NON-NLS-1$
      
      if (viewPart != null)
      {
         final IHandlerService handlerService = (IHandlerService)viewPart.getSite().getService(IHandlerService.class);
         
         actionNew.setActionDefinitionId("org.netxms.ui.eclipse.eventmanager.commands.new_event_template"); //$NON-NLS-1$
         handlerService.activateHandler(actionNew.getActionDefinitionId(), new ActionHandler(actionNew));
         
         actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.eventmanager.commands.show_filter"); //$NON-NLS-1$
         handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
         
         actionRefresh = new RefreshAction(viewPart) {
            @Override
            public void run()
            {
               refreshView();
            }
         };
      }
      else
      {
         actionRefresh = new RefreshAction() {
            @Override
            public void run()
            {
               refreshView();
            }
         };
      }
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);

      // Register menu for extension.
      if (viewPart != null)
         viewPart.getSite().registerContextMenu(menuMgr, viewer);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
      mgr.add(actionNew);
      mgr.add(actionDelete);
      mgr.add(actionNewGroup);
      mgr.add(actionRemove);
      mgr.add(new Separator());
      mgr.add(actionShowGroups);
      mgr.add(actionEdit);
   }

   /**
    * Create new event template
    */
   protected void createNewEventTemplate()
   {
      final EventTemplate etmpl = new EventTemplate(0);
      EditEventTemplateDialog dlg = new EditEventTemplateDialog(getShell().getShell(), etmpl, false);
      if (dlg.open() == Window.OK)
         modifyEventObject(etmpl, true);
   }
   
   /**
    * Modify event object in server
    * 
    * @param obj to modify
    */
   protected void modifyEventObject(final EventObject obj, final boolean updateParent)
   {      
      new ConsoleJob(Messages.get().EventConfigurator_UpdateJob_Title, null, Activator.PLUGIN_ID, JOB_FAMILY) {
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().EventConfigurator_UpdateJob_Error;
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyEventObject(obj);
            
            if (updateParent)
            {
               runInUIThread(new Runnable() {
                  
                  /* (non-Javadoc)
                   * @see java.lang.Runnable#run()
                   */
                  @Override
                  public void run()
                  {
                     ITreeSelection selection = (ITreeSelection)viewer.getSelection();
                     if (selection.size() == 1 && selection.getFirstElement() instanceof EventGroup)
                     {
                        ((EventGroup)selection.getFirstElement()).addChild(obj.getCode());
                        modifyEventObject((EventGroup)selection.getFirstElement(), false);
                     }
                  }
               });
            }
         }
      }.start();
   }

   /**
    * Edit currently selected event template
    */
   protected void editEventTemplate()
   {
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      if (selection.getFirstElement() instanceof EventGroup)
      {
         final EventGroup group = new EventGroup((EventGroup)selection.getFirstElement());
         EditEventGroupDialog dlg = new EditEventGroupDialog(getShell().getShell(), group);
         if (dlg.open() == Window.OK)
            modifyEventObject(group, false);
         else
            return;
      }
      
      final EventTemplate etmpl = new EventTemplate((EventTemplate)selection.getFirstElement());
      EditEventTemplateDialog dlg = new EditEventTemplateDialog(getShell().getShell(), etmpl, false);
      if (dlg.open() == Window.OK)
         modifyEventObject(etmpl, false);
   }

   /**
    * Delete selected event templates
    */
   protected void deleteEventTemplate()
   {
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();

      final String message = ((selection.size() > 1) ? Messages.get().EventConfigurator_DeleteConfirmation_Plural : Messages.get().EventConfigurator_DeleteConfirmation_Singular);
      final Shell shell = viewPart.getSite().getShell();
      if (!MessageDialogHelper.openQuestion(shell, Messages.get().EventConfigurator_DeleteConfirmationTitle, message))
         return;
      
      new ConsoleJob(Messages.get().EventConfigurator_DeleteJob_Title, null, Activator.PLUGIN_ID, JOB_FAMILY) {
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().EventConfigurator_DeleteJob_Error;
         }

         @SuppressWarnings("unchecked")
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            Iterator<EventObject> it = selection.iterator();
            while(it.hasNext())
            {
               session.deleteEventObject(it.next().getCode());
            }
         }
      }.start();
   }
   
   /**
    * Remove event object from group
    */
   @SuppressWarnings("unchecked")
   protected void removeFromGroup()
   {
      ITreeSelection selection = (ITreeSelection)viewer.getSelection();
      List<EventObject> list = selection.toList();
      for(int i = 0; i < selection.size(); i++)
      {
         EventGroup parent = (EventGroup)selection.getPathsFor(list.get(i))[0].getParentPath().getLastSegment();
         EventObject child = list.get(i);
         parent.removeChild(child.getCode());
         modifyEventObject(parent, false);
      }
   }
   
   /**
    * Create new Event group 
    */
   protected void createNewEventGroup()
   {
      EventGroup group = new EventGroup();
         
      EditEventGroupDialog dlg = new EditEventGroupDialog(getShell().getShell(), group);
      if (dlg.open() == Window.OK)
         modifyEventObject(group, true);
   } 
   
   /**
    * Enable/Disable event group viewing
    */
   protected void showEventGroups()
   {
      if (showGroups)
         showGroups = false;
      else
         showGroups = true;
      
      actionShowGroups.setChecked(showGroups);
      actionNewGroup.setEnabled(showGroups);
      actionRefresh.run();
   }
   
   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      filterEnabled = enable;
      filterControl.setVisible(filterEnabled);
      FormData fd = (FormData)viewer.getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterControl) : new FormAttachment(0, 0);
      viewer.getControl().getParent().layout();
      if (enable)
      {
         filterControl.setFocus();
      }
      else
      {
         filterControl.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
      actionShowFilter.setChecked(enable);
   }
   
   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      filter.setFilterText(filterControl.getText());
      viewer.refresh();
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      settings.put("EventConfigurator.filterEnabled", filterEnabled); //$NON-NLS-1$
      
      session.removeListener(this);
      super.dispose();
   }
   
   /**
    * Get underlying tree viewer
    * @return viewer
    */
   public SortableTreeViewer getViewer()
   {
      return viewer;
   }
   
   /**
    * Get create new template action
    * 
    * @return new template action
    */
   public Action getActionNewTemplate()
   {
      return actionNew;
   }
   
   /**
    * Get edit event object action
    * 
    * @return edit action
    */
   public Action getActionEdit()
   {
      return actionEdit;
   }
   
   /**
    * Get delete action
    * 
    * @return delete action
    */
   public Action getActionDelete()
   {
      return actionDelete;
   }
   
   /**
    * Get remove from group action
    * 
    * @return remove from group action
    */
   public Action getActionRemoveFromGroup()
   {
      return actionRemove;
   }
   
   /**
    * Get show filter action
    *  
    * @return show filter action
    */
   public Action getActionShowFilter()
   {
      return actionShowFilter;
   }
   
   /**
    * Get refresh action
    * 
    * @return refresh action
    */
   public Action getActionRefresh()
   {
      return actionRefresh;
   }
   
   /**
    * Get show groups action
    * 
    * @return show groups action
    */
   public Action getActionShowGroups()
   {
      return actionShowGroups;
   }
   
   /**
    * Get create new group action
    * 
    * @return create new group action
    */
   public Action getActionNewGroup()
   {
      return actionNewGroup;
   }
}
