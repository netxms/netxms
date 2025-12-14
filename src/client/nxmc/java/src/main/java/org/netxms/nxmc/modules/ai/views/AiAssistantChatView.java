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

import java.util.List;
import org.commonmark.Extension;
import org.commonmark.ext.autolink.AutolinkExtension;
import org.commonmark.ext.footnotes.FootnotesExtension;
import org.commonmark.ext.gfm.strikethrough.StrikethroughExtension;
import org.commonmark.ext.gfm.tables.TablesExtension;
import org.commonmark.ext.image.attributes.ImageAttributesExtension;
import org.commonmark.ext.ins.InsExtension;
import org.commonmark.ext.task.list.items.TaskListItemsExtension;
import org.commonmark.node.Node;
import org.commonmark.parser.Parser;
import org.commonmark.renderer.html.HtmlRenderer;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.ProgressEvent;
import org.eclipse.swt.browser.ProgressListener;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * AI assistant chat view
 */
public class AiAssistantChatView extends View
{
   private static final Logger logger = LoggerFactory.getLogger(AiAssistantChatView.class);
   private static final List<Extension> extensions =
         List.of(AutolinkExtension.create(), FootnotesExtension.builder().inlineFootnotes(true).build(), ImageAttributesExtension.create(), InsExtension.create(),
               TaskListItemsExtension.create(), StrikethroughExtension.create(), TablesExtension.create());

   private final I18n i18n = LocalizationHelper.getI18n(AiAssistantChatView.class);

   private NXCSession session;
   private long chatId;
   private Browser chatBrowser;
   private Text chatInput;
   private Action actionClearChat;
   private StringBuilder chatContent;

   /**
    * Create server console view
    */
   public AiAssistantChatView()
   {
      super(LocalizationHelper.getI18n(AiAssistantChatView.class).tr("AI Assistant"), ResourceManager.getImageDescriptor("icons/tool-views/ai-assistant.png"), "tools.ai-assistant", false);
      session = Registry.getSession();
      chatContent = new StringBuilder();
      initializeHtmlTemplate();
   }

   /**
    * Initialize HTML template for chat display
    */
   private void initializeHtmlTemplate()
   {
      chatContent.append("<!DOCTYPE html>");
      chatContent.append("<html><head>");
      chatContent.append("<style>");
      chatContent.append("body { font: 14px \"Metropolis Regular\", \"Segoe UI\", \"Liberation Sans\", Verdana, Helvetica, sans-serif; margin: 0; padding: 10px; background-color: ");
      chatContent.append(ColorConverter.rgbToCss(ThemeEngine.getBackgroundColor("AiAssistant.ChatView").getRGB()));
      chatContent.append("; }");
      chatContent.append(".message { margin-bottom: 15px; display: flex; }");
      chatContent.append(".user-message { justify-content: flex-end; }");
      chatContent.append(".assistant-message { justify-content: flex-start; }");
      chatContent.append(".bubble { max-width: 70%; padding: 8px 12px; border-radius: 8px; border: 1px solid; word-wrap: break-word; color: #202020; }");
      chatContent.append(".user-bubble { background-color: ");
      chatContent.append(ColorConverter.rgbToCss(ThemeEngine.getBackgroundColor("AiAssistant.UserMessage").getRGB()));
      chatContent.append("; border-color: ");
      chatContent.append(ColorConverter.rgbToCss(ThemeEngine.getForegroundColor("AiAssistant.UserMessage").getRGB()));
      chatContent.append("; }");
      chatContent.append(".assistant-bubble { background-color: ");
      chatContent.append(ColorConverter.rgbToCss(ThemeEngine.getBackgroundColor("AiAssistant.AssistantMessage").getRGB()));
      chatContent.append("; border-color: ");
      chatContent.append(ColorConverter.rgbToCss(ThemeEngine.getForegroundColor("AiAssistant.AssistantMessage").getRGB()));
      chatContent.append("; }");
      chatContent.append(".sender { font-size: 12px; margin-bottom: 5px; opacity: 0.7; }");
      chatContent.append("</style>");
      chatContent.append("</head><body>");
      chatContent.append("<div id='chat-container'>");
      chatContent.append("</div>");
      chatContent.append("</body></html>");
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

      // Create browser for chat display
      chatBrowser = WidgetHelper.createBrowser(parent, SWT.NONE, null);
      chatBrowser.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      chatBrowser.addProgressListener(new ProgressListener() {
         @Override
         public void completed(ProgressEvent event)
         {
            chatBrowser.execute("window.scrollTo(0, document.body.scrollHeight);");
         }

         @Override
         public void changed(ProgressEvent event)
         {
         }
      });

      // Initialize with welcome message
      addAssistantMessage("Hello! I'm Iris, your AI assistant. I can help you with setting up your monitoring environment, day-to-day operations, and analyzing problems. Feel free to ask any questions!");
      updateBrowserContent();

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
      final String assistantMessageId = addAssistantMessage(i18n.tr("Thinking..."));
      Job job = new Job(i18n.tr("Processing AI assistant query"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final String answer = session.updateAiAssistantChat(chatId, message);
               runInUIThread(() -> updateMessage(assistantMessageId, answer));
            }
            catch(Exception e)
            {
               runInUIThread(() -> updateMessage(assistantMessageId, "Error: " + getErrorMessage()));
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
   private void addUserMessage(String message)
   {
      addMessage(message, true, null);
   }

   /**
    * Add assistant message to the chat
    */
   private String addAssistantMessage(String message)
   {
      String messageId = "msg_" + System.currentTimeMillis();
      addMessage(message, false, messageId);
      return messageId;
   }

   /**
    * Add a message to the chat
    * 
    * @param message the message text
    * @param isUser true if this is a user message, false for assistant
    * @param messageId unique identifier for the message (null for user messages)
    */
   private void addMessage(String message, boolean isUser, String messageId)
   {
      String escapedMessage = prepareMessageText(message);
      String messageClass = isUser ? "user-message" : "assistant-message";
      String bubbleClass = isUser ? "user-bubble" : "assistant-bubble";
      String sender = isUser ? i18n.tr("User") : i18n.tr("AI Assistant");
      String id = messageId != null ? " id='" + messageId + "'" : "";

      String messageHtml = "<div class='message " + messageClass + "'>" +
                          "<div class='bubble " + bubbleClass + "'" + id + ">" +
                          "<div class='sender'>" + sender + "</div>" +
                          escapedMessage +
                          "</div></div>";

      // Insert before closing </div></body></html>
      int insertPos = chatContent.lastIndexOf("</div></body></html>");
      chatContent.insert(insertPos, messageHtml);
      
      updateBrowserContent();
   }

   /**
    * Update existing message in the chat
    *
    * @param messageId ID of the message to update
    * @param newText new text content
    */
   private void updateMessage(String messageId, String newText)
   {
      String escapedMessage = prepareMessageText(newText);
      String currentContent = chatContent.toString();
      
      // Find the message by ID and replace its content
      String searchPattern = "id='" + messageId + "'>";
      int startPos = currentContent.indexOf(searchPattern);
      if (startPos != -1)
      {
         int contentStart = currentContent.indexOf("</div>", startPos) + 6; // After sender div
         int contentEnd = currentContent.indexOf("</div></div>", contentStart);
         
         if (contentEnd != -1)
         {
            // Replace the old content
            chatContent.replace(contentStart, contentEnd, escapedMessage);
            updateBrowserContent();
         }
      }
   }

   /**
    * Update browser content with current chat HTML
    */
   private void updateBrowserContent()
   {
      chatBrowser.setText(chatContent.toString());
   }

   /**
    * Prepare message text for HTML display
    */
   private String prepareMessageText(String text)
   {
      String html;
      try
      {
         Parser parser = Parser.builder().extensions(extensions).build();
         Node document = parser.parse(text);
         HtmlRenderer renderer = HtmlRenderer.builder().extensions(extensions).build();
         html = renderer.render(document);
      }
      catch(Exception e)
      {
         logger.error("Exception in Markdown renderer", e);
         html = "<div class=\"notification notification-error\">Markdown rendering error</div><div>Original document source:</div><code>" + escapeHtml(text) +
               "</code></div></body></html>";
      }
      return html;
   }

   /**
    * Escape HTML special characters in text.
    *
    * @param text text to escape
    * @return escaped text
    */
   private static String escapeHtml(String text)
   {
      return text.replace("&", "&amp;")
                 .replace("<", "&lt;")
                 .replace(">", "&gt;")
                 .replace("\"", "&quot;")
                 .replace("'", "&#x27;")
                 .replace("\n", "<br>");
   }

   /**
    * Clear all chat messages
    */
   private void clearChat()
   {
      chatContent.setLength(0);
      initializeHtmlTemplate();
      addAssistantMessage("Hello! I'm Iris, your AI assistant. I can help you with setting up your monitoring environment, day-to-day operations, and analyzing problems. Feel free to ask any questions!");
   }
}
