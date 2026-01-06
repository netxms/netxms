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
package org.netxms.nxmc.modules.ai.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiMessage;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.widgets.AiMessageContentPanel;
import org.netxms.nxmc.modules.ai.widgets.AiMessageContentPanel.AiMessageActionHandler;
import org.netxms.nxmc.modules.ai.widgets.AiMessageList;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * AI messages view - displays messages from AI background tasks in a two-pane layout
 */
public class AiMessagesView extends View implements AiMessageActionHandler
{
   private final I18n i18n = LocalizationHelper.getI18n(AiMessagesView.class);

   private NXCSession session;
   private SashForm splitter;
   private AiMessageList messageList;
   private AiMessageContentPanel contentPanel;
   private ExportToCsvAction actionExportToCsv;

   /**
    * Create AI messages view
    */
   public AiMessagesView()
   {
      super(LocalizationHelper.getI18n(AiMessagesView.class).tr("AI Messages"),
            ResourceManager.getImageDescriptor("icons/tool-views/ai-messages.png"),
            "tools.ai-messages", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      parent.setLayout(new FillLayout());

      splitter = new SashForm(parent, SWT.VERTICAL);

      // Upper pane: message list
      messageList = new AiMessageList(this, splitter, SWT.BORDER, "AiMessagesView");
      messageList.setSelectionHandler((message) -> contentPanel.setMessage(message));
      messageList.setMessageUpdateHandler((message) -> contentPanel.updateMessage(message));

      // Lower pane: message content with markdown viewer and action buttons
      contentPanel = new AiMessageContentPanel(splitter, SWT.BORDER, this);

      // Set sash weights (60% list, 40% content)
      splitter.setWeights(new int[] { 60, 40 });

      setFilterClient(messageList.getViewer(), messageList.getFilter());
      actionExportToCsv = new ExportToCsvAction(this, messageList.getViewer(), false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(messageList.getActionMarkRead());
      manager.add(new Separator());
      manager.add(messageList.getActionApprove());
      manager.add(messageList.getActionReject());
      manager.add(new Separator());
      manager.add(messageList.getActionDelete());
      manager.add(new Separator());
      manager.add(actionExportToCsv);
      super.fillLocalMenu(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(messageList.getActionApprove());
      manager.add(messageList.getActionReject());
      manager.add(new Separator());
      manager.add(messageList.getActionDelete());
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      messageList.refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.ai.widgets.AiMessageContentPanel.AiMessageActionHandler#approveMessage(org.netxms.client.ai.AiMessage)
    */
   @Override
   public void approveMessage(AiMessage message)
   {
      new Job(i18n.tr("Approving AI message"), this) {
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
    * @see org.netxms.nxmc.modules.ai.widgets.AiMessageContentPanel.AiMessageActionHandler#rejectMessage(org.netxms.client.ai.AiMessage)
    */
   @Override
   public void rejectMessage(AiMessage message)
   {
      new Job(i18n.tr("Rejecting AI message"), this) {
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
    * @see org.netxms.nxmc.modules.ai.widgets.AiMessageContentPanel.AiMessageActionHandler#markMessageAsRead(org.netxms.client.ai.AiMessage)
    */
   @Override
   public void markMessageAsRead(AiMessage message)
   {
      new Job(i18n.tr("Marking AI message as read"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.markAiMessageAsRead(message.getId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot mark AI message as read");
         }
      }.start();
   }
}
