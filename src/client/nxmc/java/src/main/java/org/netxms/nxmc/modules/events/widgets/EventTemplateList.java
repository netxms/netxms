/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.events.widgets;

import java.util.HashMap;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.events.EventReference;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.RefreshAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.dialogs.EditEventTemplateDialog;
import org.netxms.nxmc.modules.events.dialogs.EventReferenceViewDialog;
import org.netxms.nxmc.modules.events.widgets.helpers.EventTemplateComparator;
import org.netxms.nxmc.modules.events.widgets.helpers.EventTemplateFilter;
import org.netxms.nxmc.modules.events.widgets.helpers.EventTemplateLabelProvider;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Event template list widget
 */
public class EventTemplateList extends Composite implements SessionListener
{
   private final I18n i18n = LocalizationHelper.getI18n(EventTemplateList.class);

   // Columns
   public static final int COLUMN_CODE = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_SEVERITY_OR_TAGS = 2; // Tags in dialog, severity in full list
   public static final int COLUMN_FLAGS = 3;
   public static final int COLUMN_MESSAGE = 4;
   public static final int COLUMN_TAGS = 5;

   private HashMap<Integer, EventTemplate> eventTemplates;
   private SortableTableViewer viewer;
   private Action actionNew;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionDuplicate;
   private Action actionFindReferences;
   private Action actionRefresh;
   private NXCSession session;
   private EventTemplateFilter filter;
   private View view;

   /**
    * Constructor for event template list.
    * 
    * @param view owning view
    * @param parent parent composite
    * @param style widget style bits
    * @param configPrefix configuration prefix
    */
   public EventTemplateList(View view, Composite parent, int style, final String configPrefix)
   {
      this(parent, style, configPrefix, false);
      this.view = view;
   }

   /**
    * Constructor for event template list.
    * 
    * @param parent parent composite
    * @param style widget style bits
    * @param configPrefix configuration prefix
    * @param isDialog true if widget is part of dialog
    */
   public EventTemplateList(Composite parent, int style, final String configPrefix, boolean isDialog)
   {
      super(parent, style);
      setLayout(new FillLayout());

      session = Registry.getSession();      

      final String[] names = { i18n.tr("Code"), i18n.tr("Name"), i18n.tr("Severity"), i18n.tr("Flags"), i18n.tr("Message"), i18n.tr("Tags") };
      final int[] widths = { 70, 200, 90, 50, 400, 300 };
      final String[] dialogNames = { i18n.tr("Code"), i18n.tr("Name"), i18n.tr("Tags") };
      final int[] dialogWidths = { 70, 200, 300 };

      viewer = new SortableTableViewer(this, isDialog ? dialogNames : names,
            isDialog ? dialogWidths : widths,
            0, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);

      WidgetHelper.restoreTableViewerSettings(viewer, configPrefix);
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
            WidgetHelper.saveTableViewerSettings(viewer, configPrefix);
         }
      });      

      createActions();
      createPopupMenu();

      refreshView();
      session.addListener(this);
   }

   /**
    * Refresh view
    */
   private void refreshView()
   {
      new Job(i18n.tr("Reading list of configured event templates"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<EventTemplate> list = session.getEventTemplates();
            runInUIThread(() -> {
               eventTemplates = new HashMap<>(list.size());
               for(final EventTemplate t : list)
                  eventTemplates.put(t.getCode(), t);
               viewer.setInput(eventTemplates.values().toArray());
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of configured event templates");
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
      actionNew = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewEventTemplate();
         }
      };

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) { //$NON-NLS-1$
         @Override
         public void run()
         {
            editEventTemplate();
         }
      };
      actionEdit.setEnabled(false);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteEventTemplate();
         }
      };
      actionDelete.setEnabled(false);

      actionDuplicate = new Action(i18n.tr("D&uplicate...")) {
         @Override
         public void run()
         {
            duplicateEventTemplate();
         }
      };

      actionFindReferences = new Action(i18n.tr("Find &references...")) {
         @Override
         public void run()
         {
            findReferences();
         }
      };

      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {
            refreshView();
         }
      };
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
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
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
      new Job(i18n.tr("Updating event template"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyEventObject(tmpl);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update event template");
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

      final String message = (selection.size() > 1) ? i18n.tr("Do you really wish to delete selected event templates?") : i18n.tr("Do you really wish to delete selected event template?");
      if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Confirm event template deletion"), message))
         return;

      final EventTemplate[] deleteList = new EventTemplate[selection.size()];
      int i = 0;
      for(Object o : selection.toList())
         deleteList[i++] = (EventTemplate)o;

      new Job(i18n.tr("Deleting event template"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete event template");
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
      new Job(i18n.tr("Get event template references"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
                     MessageDialogHelper.openInformation(getShell(), i18n.tr("Event References"), i18n.tr("No references found."));
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get event template references");
         }
      }.start();
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
    * Get viewer filter
    * @return filter
    */
   public AbstractViewerFilter getFilter()
   {
      return filter;
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
    * Get refresh action
    * 
    * @return refresh action
    */
   public Action getActionRefresh()
   {
      return actionRefresh;
   }
}
