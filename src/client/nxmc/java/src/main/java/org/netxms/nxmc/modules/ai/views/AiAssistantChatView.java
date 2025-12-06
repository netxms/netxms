/**
 * NetXMS - open source network management system
 * Copyright (C) 2025 Raden Solutions
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
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageBubble;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * AI assistant chat view
 */
public class AiAssistantChatView extends View
{
   private final I18n i18n = LocalizationHelper.getI18n(AiAssistantChatView.class);

   private NXCSession session;
   private long chatId;
   private ScrolledComposite chatScrolledComposite;
   private Composite chatContainer;
   private Text chatInput;
   private Action actionClearChat;

   /**
    * Create server console view
    */
   public AiAssistantChatView()
   {
      super(LocalizationHelper.getI18n(AiAssistantChatView.class).tr("AI Assistant"), ResourceManager.getImageDescriptor("icons/tool-views/ai-assistant.png"), "tools.ai-assistant", false);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View origin)
   {
      super.postClone(origin);

      // FIXME: clone chat session on server side
      Job job = new Job(i18n.tr("Creating AI assistant chat"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            chatId = session.createAiAssistantChat();
            runInUIThread(() -> chatInput.setEnabled(true));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create AI assistant chat");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      parent.setLayout(layout);

      // Create scrolled composite for chat messages
      chatScrolledComposite = new ScrolledComposite(parent, SWT.V_SCROLL);
      chatScrolledComposite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      chatScrolledComposite.setExpandHorizontal(true);
      chatScrolledComposite.setExpandVertical(true);

      // Create container for chat messages
      chatContainer = new Composite(chatScrolledComposite, SWT.NONE);
      GridLayout chatLayout = new GridLayout();
      chatLayout.marginWidth = 10;
      chatLayout.marginHeight = 10;
      chatLayout.verticalSpacing = 10;
      chatContainer.setLayout(chatLayout);
      chatContainer.setBackground(ThemeEngine.getBackgroundColor("AiAssistant.ChatView"));

      chatScrolledComposite.setContent(chatContainer);
      WidgetHelper.setScrollBarIncrement(chatScrolledComposite, SWT.VERTICAL, 20);
      chatScrolledComposite.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            updateScrolledComposite();
         }
      });

      // Add welcome message
      addAssistantMessage("Hello! I'm Iris, your AI assistant. I can help you with setting up your monitoring environment, day-to-day operations, and analyzing problems. Feel free to ask any questions!");

      Label separator = new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Composite commandArea = new Composite(parent, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 8;
      layout.numColumns = 2;
      commandArea.setLayout(layout);
      commandArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      chatInput = new Text(commandArea, SWT.NONE);
      chatInput.setMessage(i18n.tr("Write a message..."));
      chatInput.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      chatInput.setEnabled(false); // Disable input until chat creation confirmation is received
      chatInput.addKeyListener(new KeyListener() {
         @Override
         public void keyReleased(KeyEvent e)
         {
         }

         @Override
         public void keyPressed(KeyEvent e)
         {
            if (e.keyCode == 13)
               sendQuery();
         }
      });
      commandArea.setBackground(chatInput.getBackground());

      Label sendButton = new Label(commandArea, SWT.NONE);
      sendButton.setImage(SharedIcons.IMG_SEND);
      sendButton.setToolTipText(i18n.tr("Send"));
      sendButton.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));
      sendButton.setBackground(commandArea.getBackground());
      sendButton.addMouseListener(new MouseAdapter() {
         @Override
         public void mouseUp(MouseEvent e)
         {
            sendQuery();
         }
      });

      createActions();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      Job job = new Job(i18n.tr("Creating AI assistant chat"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            chatId = session.createAiAssistantChat();
            runInUIThread(() -> chatInput.setEnabled(true));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create AI assistant chat");
         }
      };
      job.setUser(false);
      job.start();
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
            new Job(i18n.tr("Clearing AI assistant chat"), AiAssistantChatView.this) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  session.clearAiAssistantChat(chatId);
                  runInUIThread(() -> clearChat());
               }

               @Override
               protected String getErrorMessage()
               {
                  return i18n.tr("Cannot clear AI assistant chat");
               }
            }.start();
         }
      };
      addKeyBinding("M1+L", actionClearChat);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionClearChat);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionClearChat);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      chatInput.setFocus();
   }

   /**
    * Send query to the server
    */
   private void sendQuery()
   {
      final String message = chatInput.getText().trim();
      chatInput.setText("");
      if (message.isEmpty())
         return;

      addUserMessage(message);
      chatInput.setEnabled(false);
      final MessageBubble assistantMessage = addAssistantMessage(i18n.tr("Thinking..."));
      Job job = new Job(i18n.tr("Processing AI assistant query"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final String answer = session.updateAiAssistantChat(chatId, message);
               runInUIThread(() -> updateMessage(assistantMessage, answer));
            }
            catch(Exception e)
            {
               runInUIThread(() -> updateMessage(assistantMessage, "Error: " + getErrorMessage()));
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get response from AI assistant");
         }

         /**
          * @see org.netxms.nxmc.base.jobs.Job#jobFinalize()
          */
         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> {
               chatInput.setEnabled(true);
               chatInput.setFocus();
            });
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Add user message to the chat
    */
   private MessageBubble addUserMessage(String message)
   {
      return addMessage(message, true);
   }

   /**
    * Add assistant message to the chat
    */
   private MessageBubble addAssistantMessage(String message)
   {
      return addMessage(message, false);
   }

   /**
    * Add a message to the chat
    * 
    * @param message the message text
    * @param isUser true if this is a user message, false for assistant
    */
   private MessageBubble addMessage(String message, boolean isUser)
   {
      Composite messageContainer = new Composite(chatContainer, SWT.NONE);
      GridLayout messageLayout = new GridLayout();
      messageLayout.marginWidth = 0;
      messageLayout.marginHeight = 0;
      messageLayout.numColumns = 5;
      messageLayout.makeColumnsEqualWidth = true;
      messageContainer.setLayout(messageLayout);
      messageContainer.setBackground(chatContainer.getBackground());

      GridData messageContainerData = new GridData(SWT.FILL, SWT.TOP, true, false);
      messageContainer.setLayoutData(messageContainerData);

      MessageBubble messageBubble;
      if (isUser)
      {
         Label spacer = new Label(messageContainer, SWT.NONE);
         spacer.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
         spacer.setBackground(chatContainer.getBackground());

         messageBubble = new MessageBubble(messageContainer, MessageBubble.Type.USER, i18n.tr("User"), message);
      }
      else
      {
         messageBubble = new MessageBubble(messageContainer, MessageBubble.Type.ASSISTANT, i18n.tr("AI Assistant"), message);

         Label spacer = new Label(messageContainer, SWT.NONE);
         spacer.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
         spacer.setBackground(chatContainer.getBackground());
      }

      GridData bubbleData = new GridData();
      bubbleData.horizontalAlignment = isUser ? SWT.RIGHT : SWT.LEFT;
      bubbleData.grabExcessHorizontalSpace = true;
      bubbleData.horizontalSpan = 4;
      messageBubble.setLayoutData(bubbleData);

      chatContainer.layout(true, true);
      updateScrolledComposite();
      scrollToBottom();
      return messageBubble;
   }

   /**
    * Update existing message in the chat.
    *
    * @param messageBubble message bubble to update
    * @param newText new text
    */
   private void updateMessage(MessageBubble messageBubble, String newText)
   {
      messageBubble.setText(newText);
      chatContainer.layout(true, true);
      updateScrolledComposite();
      scrollToBottom();
   }

   /**
    * Update the scrolled composite size
    */
   private void updateScrolledComposite()
   {
      Rectangle r = chatScrolledComposite.getClientArea();
      chatScrolledComposite.setMinSize(chatContainer.computeSize(r.width, SWT.DEFAULT));
   }

   /**
    * Scroll to the bottom of the chat
    */
   private void scrollToBottom()
   {
      Display.getDefault().asyncExec(() -> {
         if (!chatScrolledComposite.isDisposed())
         {
            Rectangle r = chatScrolledComposite.getClientArea();
            Point contentSize = chatContainer.computeSize(r.width, SWT.DEFAULT);
            Point viewportSize = chatScrolledComposite.getSize();
            if (contentSize.y > viewportSize.y)
            {
               chatScrolledComposite.setOrigin(0, contentSize.y - viewportSize.y);
            }
         }
      });
   }

   /**
    * Clear all chat messages
    */
   private void clearChat()
   {
      for(Control control : chatContainer.getChildren())
      {
         control.dispose();
      }
      chatContainer.layout(true, true);
      updateScrolledComposite();
      
      // Re-add welcome message
      addAssistantMessage("Hello! I'm Iris, your AI assistant. I can help you with setting up your monitoring environment, day-to-day operations, and analyzing problems. Feel free to ask any questions!");
   }
}