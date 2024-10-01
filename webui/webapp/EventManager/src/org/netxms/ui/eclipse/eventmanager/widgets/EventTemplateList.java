/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import java.util.HashMap;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
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
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.events.EventReference;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.eventmanager.Activator;
import org.netxms.ui.eclipse.eventmanager.Messages;
import org.netxms.ui.eclipse.eventmanager.dialogs.EditEventTemplateDialog;
import org.netxms.ui.eclipse.eventmanager.dialogs.EventReferenceViewDialog;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventTemplateComparator;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventTemplateFilter;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventTemplateLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Event template list widget
 */
public class EventTemplateList extends Composite implements SessionListener
{
   // Columns
   public static final int COLUMN_CODE = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_SEVERITY = 2;
   public static final int COLUMN_FLAGS = 3;
   public static final int COLUMN_MESSAGE = 4;
   public static final int COLUMN_TAGS = 5;

   private HashMap<Integer, EventTemplate> eventTemplates;
   private SortableTableViewer viewer;
   private FilterText filterControl;
   private Action actionNew;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionDuplicate;
   private Action actionFindReferences;
   private Action actionShowFilter;
   private Action actionRefresh;
   private NXCSession session;
   private EventTemplateFilter filter;
   private boolean filterEnabled;
   private ViewPart viewPart;
   
   /**
    * Constructor for event template list with specific columns to show
    * 
    * @param viewPart the viewPart
    * @param parent parent composite
    * @param style style
    * @param columnsToShow which columns to show
    */
   public EventTemplateList(ViewPart viewPart, Composite parent, int style, final String configPrefix)
   {
      this(parent, style, configPrefix, false);
      this.viewPart = viewPart;
   }

   /**
    * Constructor for event template list
    * 
    * @param viewPart the viewPart
    * @param parent parent composite
    * @param style style
    */
   public EventTemplateList(Composite parent, int style, final String configPrefix, boolean isDialog)
   {
      super(parent, style);
      
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      filterEnabled = isDialog ? true : settings.getBoolean(configPrefix + ".filterEnabled"); //$NON-NLS-1$
      
      session = ConsoleSharedData.getSession();
      
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
      
      final String[] names = { Messages.get().EventConfigurator_ColCode, Messages.get().EventConfigurator_ColName, Messages.get().EventConfigurator_ColSeverity, Messages.get().EventConfigurator_ColFlags, Messages.get().EventConfigurator_ColMessage, Messages.get().EventConfigurator_ColTags };
      final int[] widths = { 70, 200, 90, 50, 400, 300 };
      final String[] dialogNames = { Messages.get().EventConfigurator_ColCode, Messages.get().EventConfigurator_ColName, Messages.get().EventConfigurator_ColTags };
      final int[] dialogWidths = { 70, 200, 300 };

      viewer = new SortableTableViewer(parent, isDialog ? dialogNames : names,
            isDialog ? dialogWidths : widths,
            0, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);

      WidgetHelper.restoreTableViewerSettings(viewer, settings, configPrefix);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new EventTemplateLabelProvider(isDialog));
      viewer.setComparator(new EventTemplateComparator(isDialog));
      filter = new EventTemplateFilter();
      viewer.addFilter(filter);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = event.getStructuredSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
               actionFindReferences.setEnabled(selection.size() == 1);
            }
         }
      });
      viewer.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            session.removeListener(EventTemplateList.this);

            WidgetHelper.saveTableViewerSettings(viewer, settings, configPrefix);
            settings.put(configPrefix + ".filterEnabled", filterEnabled);
         }
      });

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
      new ConsoleJob(Messages.get().EventConfigurator_OpenJob_Title, null, Activator.PLUGIN_ID, null) {
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().EventConfigurator_OpenJob_Error;
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<EventTemplate> list = session.getEventTemplates();
            runInUIThread(() -> {
               eventTemplates = new HashMap<>(list.size());
               for(final EventTemplate t : list)
                  eventTemplates.put(t.getCode(), t);
               viewer.setInput(eventTemplates.values().toArray());
            });
         }
      }.start();
   }

   /**
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      if (isDisposed())
         return;

      switch(n.getCode())
      {
         case SessionNotification.EVENT_TEMPLATE_MODIFIED:
            getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  EventTemplate old = eventTemplates.get((int)n.getSubCode());
                  if (old != null)
                  {
                     old.setAll((EventTemplate)n.getObject());
                     viewer.refresh();
                  }
                  else
                  {
                     eventTemplates.put((int)n.getSubCode(), new EventTemplate((EventTemplate)n.getObject()));
                     viewer.setInput(eventTemplates.values().toArray());
                  }
               }
            });
            break;
         case SessionNotification.EVENT_TEMPLATE_DELETED:
            getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  eventTemplates.remove((int)n.getSubCode());
                  viewer.setInput(eventTemplates.values().toArray());
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

      actionDuplicate = new Action("D&uplicate...") {
         @Override
         public void run()
         {
            duplicateEventTemplate();
         }
      };

      actionFindReferences = new Action("Find &references...") {
         @Override
         public void run()
         {
            findReferences();
         }
      };

      actionShowFilter = new Action(Messages.get().EventConfigurator_ShowFilter, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
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
      mgr.add(actionDuplicate);
      mgr.add(actionEdit);
      mgr.add(actionDelete);
      mgr.add(new Separator());
      mgr.add(actionFindReferences);
   }

   /**
    * Create new event template
    */
   protected void createNewEventTemplate()
   {
      final EventTemplate tmpl = new EventTemplate(0);
      EditEventTemplateDialog dlg = new EditEventTemplateDialog(getShell().getShell(), tmpl, false);
      if (dlg.open() == Window.OK)
      {
         modifyEventTemplate(tmpl);
      }
   }

   /**
    * Create copy of existing template
    */
   protected void duplicateEventTemplate()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final EventTemplate tmpl = new EventTemplate((EventTemplate)selection.getFirstElement());
      tmpl.setCode(0);
      tmpl.setName(tmpl.getName() + "_COPY");
      EditEventTemplateDialog dlg = new EditEventTemplateDialog(getShell().getShell(), tmpl, true);
      if (dlg.open() == Window.OK)
      {
         modifyEventTemplate(tmpl);
      }
   }

   /**
    * Modify event object in server
    * 
    * @param tmpl to modify
    */
   protected void modifyEventTemplate(final EventTemplate tmpl)
   {      
      new ConsoleJob(Messages.get().EventConfigurator_UpdateJob_Title, null, Activator.PLUGIN_ID, null) {
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().EventConfigurator_UpdateJob_Error;
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyEventObject(tmpl);
         }
      }.start();
   }

   /**
    * Edit currently selected event template
    */
   protected void editEventTemplate()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      final EventTemplate tmpl = new EventTemplate((EventTemplate)selection.getFirstElement());
      EditEventTemplateDialog dlg = new EditEventTemplateDialog(getShell().getShell(), tmpl, false);
      if (dlg.open() == Window.OK)
      {
         modifyEventTemplate(tmpl);
      }
   }

   /**
    * Delete selected event templates
    */
   protected void deleteEventTemplate()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();

      final String message = ((selection.size() > 1) ? Messages.get().EventConfigurator_DeleteConfirmation_Plural : Messages.get().EventConfigurator_DeleteConfirmation_Singular);
      final Shell shell = viewPart.getSite().getShell();
      if (!MessageDialogHelper.openQuestion(shell, Messages.get().EventConfigurator_DeleteConfirmationTitle, message))
         return;

      final EventTemplate[] deleteList = new EventTemplate[selection.size()];
      int i = 0;
      for(Object o : selection.toList())
         deleteList[i++] = (EventTemplate)o;

      new ConsoleJob(Messages.get().EventConfigurator_DeleteJob_Title, null, Activator.PLUGIN_ID) {
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().EventConfigurator_DeleteJob_Error;
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            boolean skipReferencesCheck = false;
            for(EventTemplate e : deleteList)
            {
               if (!skipReferencesCheck)
               {
                  final List<EventReference> eventReferences = session.getEventReferences(e.getCode());
                  if (!eventReferences.isEmpty())
                  {
                     final int[] result = new int[1];
                     getDisplay().syncExec(new Runnable() {
                        @Override
                        public void run()
                        {
                           EventReferenceViewDialog dlg = new EventReferenceViewDialog(getShell(), e.getName(), eventReferences, true, deleteList.length > 1);
                           result[0] = dlg.open();
                        }
                     });
                     if ((result[0] == IDialogConstants.NO_ID) || (result[0] == IDialogConstants.CANCEL_ID))
                        continue;
                     if (result[0] == IDialogConstants.NO_TO_ALL_ID)
                        return;
                     if (result[0] == IDialogConstants.YES_TO_ALL_ID)
                        skipReferencesCheck = true;
                  }
               }
               session.deleteEventTemplate(e.getCode());
            }
         }
      }.start();
   }

   /**
    * Find references to selected event template
    */
   private void findReferences()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final long eventCode = ((EventTemplate)selection.getFirstElement()).getCode();
      final String eventName = ((EventTemplate)selection.getFirstElement()).getName();
      new ConsoleJob("Get event template references", null, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<EventReference> eventReferences = session.getEventReferences(eventCode);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!eventReferences.isEmpty())
                  {
                     EventReferenceViewDialog dlg = new EventReferenceViewDialog(getShell(), eventName, eventReferences, false, false);
                     dlg.open();
                  }
                  else
                  {
                     MessageDialogHelper.openInformation(getShell(), "Event References", "No references found.");
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get event template references";
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
   
   /**
    * Get underlying tree viewer
    * @return viewer
    */
   public SortableTableViewer getViewer()
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
}
