/**
 * NetXMS - open source network management system
 * Copyright (C) 2025-2026 Raden Solutions
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

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.actions.ExportConversationAction;
import org.netxms.nxmc.modules.ai.widgets.AiAssistantChatWidget;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * AI assistant chat view
 */
public class AiAssistantChatView extends View
{
   private final I18n i18n = LocalizationHelper.getI18n(AiAssistantChatView.class);

   private long boundIncidentId;
   private AiAssistantChatWidget chatWidget;
   private Action actionClearChat;
   private Action actionExportConversation;

   /**
    * Create AI assistant chat view
    */
   public AiAssistantChatView()
   {
      super(LocalizationHelper.getI18n(AiAssistantChatView.class).tr("AI Assistant"), SharedIcons.AI_ASSISTANT, "tools.ai-assistant", false);
      boundIncidentId = 0;
   }

   /**
    * Create AI assistant chat view bound to an incident
    *
    * @param incidentId the incident ID to bind the chat to
    */
   public AiAssistantChatView(long incidentId)
   {
      super(LocalizationHelper.getI18n(AiAssistantChatView.class).tr("AI Assistant - Incident #{0}", incidentId),
            SharedIcons.AI_ASSISTANT, "tools.ai-assistant." + incidentId, false);
      boundIncidentId = incidentId;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View origin)
   {
      super.postClone(origin);
      chatWidget.createChatSession(false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      parent.setLayout(new FillLayout());
      chatWidget = new AiAssistantChatWidget(parent, SWT.NONE, this, boundIncidentId, false);
      createActions();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      chatWidget.createChatSession(true);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionClearChat = new Action(i18n.tr("&Clear chat"), SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            chatWidget.clearChatSession();
         }
      };
      addKeyBinding("M1+L", actionClearChat);

      actionExportConversation = new ExportConversationAction(this, chatWidget);
      addKeyBinding("M1+M2+E", actionExportConversation);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportConversation);
      manager.add(actionClearChat);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportConversation);
      manager.add(actionClearChat);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      chatWidget.getChatInput().setFocus();
   }
}
