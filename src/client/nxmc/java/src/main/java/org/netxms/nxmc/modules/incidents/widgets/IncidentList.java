/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.incidents.widgets;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
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
import org.netxms.client.constants.IncidentState;
import org.netxms.client.events.IncidentSummary;
import org.netxms.client.users.User;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.incidents.dialogs.CreateIncidentDialog;
import org.netxms.nxmc.modules.incidents.dialogs.EditIncidentCommentDialog;
import org.netxms.nxmc.modules.incidents.views.IncidentDetails;
import org.netxms.nxmc.modules.incidents.widgets.helpers.IncidentComparator;
import org.netxms.nxmc.modules.incidents.widgets.helpers.IncidentListFilter;
import org.netxms.nxmc.modules.incidents.widgets.helpers.IncidentListLabelProvider;
import org.netxms.nxmc.modules.users.dialogs.UserSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Incident list widget
 */
public class IncidentList extends Composite
{
   // Column indices
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_STATE = 1;
   public static final int COLUMN_TITLE = 2;
   public static final int COLUMN_SOURCE_OBJECT = 3;
   public static final int COLUMN_ASSIGNED_USER = 4;
   public static final int COLUMN_CREATED_TIME = 5;
   public static final int COLUMN_LAST_CHANGE_TIME = 6;
   public static final int COLUMN_ALARMS = 7;

   private final I18n i18n = LocalizationHelper.getI18n(IncidentList.class);

   private View view;
   private NXCSession session;
   private SessionListener sessionListener;
   private SortableTableViewer viewer;
   private IncidentListFilter filter;
   private Map<Long, IncidentSummary> incidents = new HashMap<>();

   private Action actionCreate;
   private Action actionBlock;
   private Action actionInProgress;
   private Action actionResolve;
   private Action actionReopen;
   private Action actionClose;
   private Action actionAssign;
   private Action actionAddComment;

   /**
    * Create incident list widget
    *
    * @param view owning view
    * @param parent parent composite
    * @param style widget style
    * @param configPrefix configuration prefix for saving settings
    */
   public IncidentList(View view, Composite parent, int style, String configPrefix)
   {
      super(parent, style);
      this.view = view;
      this.session = Registry.getSession();

      setLayout(new FillLayout());

      final String[] columnNames = {
         i18n.tr("ID"),
         i18n.tr("State"),
         i18n.tr("Title"),
         i18n.tr("Source"),
         i18n.tr("Assignee"),
         i18n.tr("Created"),
         i18n.tr("Last Change"),
         i18n.tr("Alarms")
      };
      final int[] columnWidths = { 80, 100, 300, 150, 120, 150, 150, 70 };

      viewer = new SortableTableViewer(this, columnNames, columnWidths, COLUMN_ID, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new IncidentListLabelProvider(viewer));
      viewer.setComparator(new IncidentComparator());

      filter = new IncidentListFilter();
      viewer.addFilter(filter);

      viewer.enableColumnReordering();
      WidgetHelper.restoreTableViewerSettings(viewer, configPrefix);
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, configPrefix);
         }
      });

      createActions();
      createContextMenu();

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            updateActionStates();
         }
      });

      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            if (selection.size() == 1)
            {
               IncidentSummary incident = (IncidentSummary)selection.getFirstElement();
               view.openView(new IncidentDetails(incident.getId(), incident.getSourceObjectId()));
            }
         }
      });

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            // Handle incident update notifications when implemented
         }
      };
      session.addListener(sessionListener);

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            session.removeListener(sessionListener);
         }
      });

      refresh();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCreate = new Action(i18n.tr("&Create incident..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createIncident();
         }
      };

      actionBlock = new Action(i18n.tr("&Block..."), ResourceManager.getImageDescriptor("icons/incidents/incident-blocked.png")) {
         @Override
         public void run()
         {
            blockIncident();
         }
      };

      actionInProgress = new Action(i18n.tr("In &progress"), ResourceManager.getImageDescriptor("icons/incidents/incident-in-progress.png")) {
         @Override
         public void run()
         {
            changeIncidentState(IncidentState.IN_PROGRESS);
         }
      };

      actionResolve = new Action(i18n.tr("&Resolve"), ResourceManager.getImageDescriptor("icons/incidents/incident-resolved.png")) {
         @Override
         public void run()
         {
            resolveIncidents();
         }
      };

      actionReopen = new Action(i18n.tr("Re&open"), ResourceManager.getImageDescriptor("icons/incidents/incident-open.png")) {
         @Override
         public void run()
         {
            changeIncidentState(IncidentState.OPEN);
         }
      };

      actionClose = new Action(i18n.tr("C&lose"), ResourceManager.getImageDescriptor("icons/incidents/incident-closed.png")) {
         @Override
         public void run()
         {
            closeIncidents();
         }
      };

      actionAssign = new Action(i18n.tr("&Assign...")) {
         @Override
         public void run()
         {
            assignIncident();
         }
      };

      actionAddComment = new Action(i18n.tr("Add co&mment..."), ResourceManager.getImageDescriptor("icons/new_comment.png")) {
         @Override
         public void run()
         {
            addComment();
         }
      };
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager menuManager = new MenuManager();
      menuManager.setRemoveAllWhenShown(true);
      menuManager.addMenuListener((manager) -> fillContextMenu(manager));

      Menu menu = menuManager.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    */
   private void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionInProgress);
      manager.add(actionBlock);
      manager.add(actionResolve);
      manager.add(actionReopen);
      manager.add(actionClose);
      manager.add(new Separator());
      manager.add(actionAssign);
      manager.add(actionAddComment);
   }

   /**
    * Update action states based on selection
    */
   private void updateActionStates()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      boolean hasSelection = !selection.isEmpty();
      boolean singleSelection = selection.size() == 1;

      IncidentSummary selectedIncident = singleSelection ? (IncidentSummary)selection.getFirstElement() : null;
      IncidentState state = (selectedIncident != null) ? selectedIncident.getState() : null;

      boolean canBlock = singleSelection && (state != null) && (state != IncidentState.BLOCKED) && (state != IncidentState.RESOLVED) && (state != IncidentState.CLOSED);
      boolean canSetInProgress = singleSelection && ((state == IncidentState.OPEN) || (state == IncidentState.BLOCKED));
      boolean canResolve = hasSelection;
      boolean canReopen = singleSelection && (state == IncidentState.RESOLVED);

      actionBlock.setEnabled(canBlock);
      actionInProgress.setEnabled(canSetInProgress);
      actionResolve.setEnabled(canResolve);
      actionReopen.setEnabled(canReopen);
      actionClose.setEnabled(hasSelection);
      actionAssign.setEnabled(singleSelection);
      actionAddComment.setEnabled(singleSelection);
   }

   /**
    * Refresh incident list
    */
   public void refresh()
   {
      new Job(i18n.tr("Loading incidents"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<IncidentSummary> incidentList = session.getIncidents(0);
            runInUIThread(() -> {
               incidents.clear();
               for(IncidentSummary i : incidentList)
                  incidents.put(i.getId(), i);
               viewer.setInput(incidents.values().toArray());
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load incidents");
         }
      }.start();
   }

   /**
    * Create new incident
    */
   private void createIncident()
   {
      CreateIncidentDialog dialog = new CreateIncidentDialog(getShell());
      if (dialog.open() == Window.OK)
      {
         final long objectId = dialog.getObjectId();
         final String title = dialog.getTitle();
         final String initialComment = dialog.getInitialComment();

         new Job(i18n.tr("Creating incident"), view) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.createIncident(objectId, title, initialComment, 0);
               runInUIThread(() -> refresh());
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot create incident");
            }
         }.start();
      }
   }

   /**
    * Block selected incident
    */
   private void blockIncident()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final IncidentSummary incident = (IncidentSummary)selection.getFirstElement();

      EditIncidentCommentDialog dialog = new EditIncidentCommentDialog(getShell(), null,
            i18n.tr("Block Incident"), i18n.tr("Reason for blocking"));
      if (dialog.open() != Window.OK)
         return;

      final String comment = dialog.getText();
      if (comment.trim().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"),
               i18n.tr("Comment is required when blocking an incident"));
         return;
      }

      new Job(i18n.tr("Blocking incident"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.changeIncidentState(incident.getId(), IncidentState.BLOCKED, comment);
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot block incident");
         }
      }.start();
   }

   /**
    * Change state of selected incident
    *
    * @param newState new incident state
    */
   private void changeIncidentState(IncidentState newState)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final IncidentSummary incident = (IncidentSummary)selection.getFirstElement();

      new Job(i18n.tr("Changing incident state"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.changeIncidentState(incident.getId(), newState, null);
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot change incident state");
         }
      }.start();
   }

   /**
    * Resolve selected incidents
    */
   private void resolveIncidents()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Resolve Incidents"),
            i18n.tr("Are you sure you want to resolve selected incident(s)? All linked alarms will also be resolved.")))
         return;

      final Object[] selectedIncidents = selection.toArray();
      new Job(i18n.tr("Resolving incidents"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for (Object o : selectedIncidents)
            {
               session.resolveIncident(((IncidentSummary)o).getId());
            }
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot resolve incident");
         }
      }.start();
   }

   /**
    * Close selected incidents
    */
   private void closeIncidents()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Close Incidents"),
            i18n.tr("Are you sure you want to close selected incident(s)?")))
         return;

      final Object[] selectedIncidents = selection.toArray();
      new Job(i18n.tr("Closing incidents"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for (Object o : selectedIncidents)
            {
               session.closeIncident(((IncidentSummary)o).getId());
            }
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot close incident");
         }
      }.start();
   }

   /**
    * Assign incident to user
    */
   private void assignIncident()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final IncidentSummary incident = (IncidentSummary)selection.getFirstElement();

      UserSelectionDialog dialog = new UserSelectionDialog(getShell(), User.class);
      dialog.enableMultiSelection(false);
      if (dialog.open() != Window.OK)
         return;

      final int userId = dialog.getSelection()[0].getId();
      new Job(i18n.tr("Assigning incident"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.assignIncident(incident.getId(), userId);
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot assign incident");
         }
      }.start();
   }

   /**
    * Add comment to incident
    */
   private void addComment()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final IncidentSummary incident = (IncidentSummary)selection.getFirstElement();

      EditIncidentCommentDialog dialog = new EditIncidentCommentDialog(getShell(), null);
      if (dialog.open() == Window.OK)
      {
         final String comment = dialog.getText();
         new Job(i18n.tr("Adding incident comment"), view) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.addIncidentComment(incident.getId(), comment);
               runInUIThread(() -> refresh());
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot add comment");
            }
         }.start();
      }
   }

   /**
    * Get viewer
    *
    * @return viewer
    */
   public SortableTableViewer getViewer()
   {
      return viewer;
   }

   /**
    * Get filter
    *
    * @return filter
    */
   public IncidentListFilter getFilter()
   {
      return filter;
   }

   /**
    * Get action for creating incidents
    *
    * @return create action
    */
   public Action getActionCreate()
   {
      return actionCreate;
   }

   /**
    * Get action for resetting column order
    *
    * @return reset column order action
    */
   public Action getActionResetColumnOrder()
   {
      return viewer.getResetColumnOrderAction();
   }
}
