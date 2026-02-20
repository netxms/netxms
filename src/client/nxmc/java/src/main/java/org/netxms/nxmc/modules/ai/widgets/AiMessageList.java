/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai.widgets;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.ai.AiMessage;
import org.netxms.client.constants.AiMessageStatus;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.widgets.helpers.AiMessageListComparator;
import org.netxms.nxmc.modules.ai.widgets.helpers.AiMessageListFilter;
import org.netxms.nxmc.modules.ai.widgets.helpers.AiMessageListLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * AI message list widget
 */
public class AiMessageList extends Composite
{
   // Column indices
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_STATUS = 2;
   public static final int COLUMN_TITLE = 3;
   public static final int COLUMN_RELATED_OBJECT = 4;
   public static final int COLUMN_CREATED = 5;
   public static final int COLUMN_EXPIRES = 6;

   private final I18n i18n = LocalizationHelper.getI18n(AiMessageList.class);

   private View view;
   private NXCSession session;
   private SessionListener sessionListener;
   private SortableTableViewer viewer;
   private AiMessageListFilter filter;
   private Map<Long, AiMessage> messages = new HashMap<>();

   private Action actionMarkRead;
   private Action actionApprove;
   private Action actionReject;
   private Action actionDelete;

   private Consumer<AiMessage> selectionHandler;
   private Consumer<AiMessage> messageUpdateHandler;

   /**
    * Create AI message list widget
    *
    * @param view owning view
    * @param parent parent composite
    * @param style widget style
    * @param configPrefix configuration prefix for saving settings
    */
   public AiMessageList(View view, Composite parent, int style, String configPrefix)
   {
      super(parent, style);
      this.view = view;
      this.session = Registry.getSession();

      setLayout(new FillLayout());

      final String[] columnNames = {
         i18n.tr("ID"),
         i18n.tr("Type"),
         i18n.tr("Status"),
         i18n.tr("Title"),
         i18n.tr("Related Object"),
         i18n.tr("Created"),
         i18n.tr("Expires")
      };
      final int[] columnWidths = { 70, 80, 80, 300, 150, 150, 150 };

      viewer = new SortableTableViewer(this, columnNames, columnWidths, COLUMN_ID, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      AiMessageListLabelProvider labelProvider = new AiMessageListLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new AiMessageListComparator());

      filter = new AiMessageListFilter(labelProvider);
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
            if (selectionHandler != null)
            {
               IStructuredSelection selection = viewer.getStructuredSelection();
               AiMessage selectedMessage = selection.size() == 1 ? (AiMessage)selection.getFirstElement() : null;
               selectionHandler.accept(selectedMessage);
            }
         }
      });

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.AI_MESSAGE_CHANGED)
            {
               getDisplay().asyncExec(() -> {
                  AiMessage message = (AiMessage)n.getObject();
                  if (message != null)
                  {
                     messages.put(message.getId(), message);
                     updateMessageList();
                     if (messageUpdateHandler != null)
                        messageUpdateHandler.accept(message);
                  }
               });
            }
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
      actionMarkRead = new Action(i18n.tr("Mark as &read"), ResourceManager.getImageDescriptor("icons/ai/mark-as-read.png")) {
         @Override
         public void run()
         {
            markSelectedAsRead();
         }
      };

      actionApprove = new Action(i18n.tr("&Approve"), SharedIcons.APPROVE) {
         @Override
         public void run()
         {
            approveSelected();
         }
      };

      actionReject = new Action(i18n.tr("Re&ject"), SharedIcons.REJECT) {
         @Override
         public void run()
         {
            rejectSelected();
         }
      };

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteSelected();
         }
      };

      updateActionStates();
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
      manager.add(actionMarkRead);
      manager.add(new Separator());
      manager.add(actionApprove);
      manager.add(actionReject);
      manager.add(new Separator());
      manager.add(actionDelete);
   }

   /**
    * Update action states based on selection
    */
   private void updateActionStates()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      boolean hasSelection = !selection.isEmpty();
      boolean singleSelection = selection.size() == 1;

      AiMessage selectedMessage = singleSelection ? (AiMessage)selection.getFirstElement() : null;
      boolean isPendingApproval = (selectedMessage != null) &&
                                  selectedMessage.isApprovalRequest() &&
                                  (selectedMessage.getStatus() == AiMessageStatus.PENDING);

      actionMarkRead.setEnabled(hasSelection);
      actionApprove.setEnabled(isPendingApproval);
      actionReject.setEnabled(isPendingApproval);
      actionDelete.setEnabled(hasSelection);
   }

   /**
    * Refresh message list
    */
   public void refresh()
   {
      new Job(i18n.tr("Loading AI messages"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<AiMessage> messageList = session.getAiMessages();
            runInUIThread(() -> {
               messages.clear();
               for(AiMessage m : messageList)
                  messages.put(m.getId(), m);
               updateMessageList();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load AI messages");
         }
      }.start();
   }

   /**
    * Update message list in viewer
    */
   private void updateMessageList()
   {
      viewer.setInput(messages.values().toArray());
   }

   /**
    * Set selection handler
    *
    * @param handler handler to be called when selection changes
    */
   public void setSelectionHandler(Consumer<AiMessage> handler)
   {
      this.selectionHandler = handler;
   }

   /**
    * Set message update handler
    *
    * @param handler handler to be called when a message is updated via notification
    */
   public void setMessageUpdateHandler(Consumer<AiMessage> handler)
   {
      this.messageUpdateHandler = handler;
   }

   /**
    * Mark selected messages as read
    */
   private void markSelectedAsRead()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final Object[] selectedMessages = selection.toArray();
      new Job(i18n.tr("Marking messages as read"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for (Object o : selectedMessages)
            {
               session.markAiMessageAsRead(((AiMessage)o).getId());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot mark message as read");
         }
      }.start();
   }

   /**
    * Approve selected message
    */
   private void approveSelected()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      approveMessage((AiMessage)selection.getFirstElement());
   }

   /**
    * Approve a message
    *
    * @param message message to approve
    */
   private void approveMessage(AiMessage message)
   {
      new Job(i18n.tr("Approving AI message"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.approveAiMessage(message.getId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot approve AI message");
         }
      }.start();
   }

   /**
    * Reject selected message
    */
   private void rejectSelected()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      rejectMessage((AiMessage)selection.getFirstElement());
   }

   /**
    * Reject a message
    *
    * @param message message to reject
    */
   private void rejectMessage(AiMessage message)
   {
      new Job(i18n.tr("Rejecting AI message"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.rejectAiMessage(message.getId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot reject AI message");
         }
      }.start();
   }

   /**
    * Delete selected messages
    */
   private void deleteSelected()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getShell(), i18n.tr("Confirm Delete"),
            i18n.tr("Do you really want to delete selected message(s)?")))
         return;

      final Object[] selectedMessages = selection.toArray();
      new Job(i18n.tr("Deleting AI messages"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for (Object o : selectedMessages)
            {
               session.deleteAiMessage(((AiMessage)o).getId());
            }
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete AI message");
         }
      }.start();
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
   public AiMessageListFilter getFilter()
   {
      return filter;
   }

   /**
    * Get action for marking messages as read
    *
    * @return mark read action
    */
   public Action getActionMarkRead()
   {
      return actionMarkRead;
   }

   /**
    * Get action for approving messages
    *
    * @return approve action
    */
   public Action getActionApprove()
   {
      return actionApprove;
   }

   /**
    * Get action for rejecting messages
    *
    * @return reject action
    */
   public Action getActionReject()
   {
      return actionReject;
   }

   /**
    * Get action for deleting messages
    *
    * @return delete action
    */
   public Action getActionDelete()
   {
      return actionDelete;
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
