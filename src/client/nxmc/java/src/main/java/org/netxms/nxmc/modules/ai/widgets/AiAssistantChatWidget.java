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
package org.netxms.nxmc.modules.ai.widgets;

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
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.BrowserFunction;
import org.eclipse.swt.browser.ProgressEvent;
import org.eclipse.swt.browser.ProgressListener;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.ai.AiQuestion;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * AI assistant chat widget - contains the chat browser, input field, and message handling.
 */
public class AiAssistantChatWidget extends Composite implements SessionListener
{
   private static final Logger logger = LoggerFactory.getLogger(AiAssistantChatWidget.class);
   private static final List<Extension> extensions =
         List.of(AutolinkExtension.create(), FootnotesExtension.builder().inlineFootnotes(true).build(), ImageAttributesExtension.create(), InsExtension.create(),
               TaskListItemsExtension.create(), StrikethroughExtension.create(), TablesExtension.create());

   private final I18n i18n = LocalizationHelper.getI18n(AiAssistantChatWidget.class);

   private NXCSession session;
   private View view;
   private long chatId;
   private long boundIncidentId;
   private boolean withContext;
   private Object context = null;
   private boolean contextChanged = false;
   private Browser chatBrowser;
   private CLabel contextName;
   private Text chatInput;
   private StringBuilder chatContent;
   private AiQuestion pendingQuestion;
   private String currentMessageId;
   private Image imageUnknownContext;
   private BaseObjectLabelProvider objectLabelProvider;

   /**
    * Create AI assistant chat widget
    *
    * @param parent parent composite
    * @param style widget style
    * @param view owning view
    * @param boundIncidentId incident ID to bind the chat to (0 for general chat)
    */
   public AiAssistantChatWidget(Composite parent, int style, View view, long boundIncidentId, boolean withContext)
   {
      super(parent, style);
      this.view = view;
      this.boundIncidentId = boundIncidentId;
      this.withContext = withContext;
      this.session = Registry.getSession();
      this.chatContent = new StringBuilder();

      imageUnknownContext = ResourceManager.getImage("icons/ai/unknown-context.png");
      objectLabelProvider = new BaseObjectLabelProvider();

      initializeHtmlTemplate();
      createContent();

      session.addListener(this);
      addDisposeListener((e) -> {
         session.removeListener(AiAssistantChatWidget.this);
         imageUnknownContext.dispose();
         objectLabelProvider.dispose();
      });
   }

   /**
    * Initialize HTML template for chat display
    */
   private void initializeHtmlTemplate()
   {
      chatContent.append("<!DOCTYPE html>");
      chatContent.append("<html><head>");
      chatContent.append("<style>");
      chatContent.append("body { font: 14px \"Metropolis Regular\", \"Segoe UI\", \"Liberation Sans\", \".AppleSystemUIFont\", Verdana, Helvetica, sans-serif; margin: 0; padding: 10px; background-color: ");
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
      // Question card styles
      chatContent.append(".question-card { background-color: #f8f9fa; border: 2px solid #0d6efd; border-radius: 12px; padding: 16px; margin: 15px 0; max-width: 80%; }");
      chatContent.append(".question-header { font-weight: bold; color: #0d6efd; margin-bottom: 10px; font-size: 13px; }");
      chatContent.append(".question-text { margin-bottom: 12px; }");
      chatContent.append(".question-context { background-color: #e9ecef; padding: 8px 12px; border-radius: 4px; font-family: monospace; margin-bottom: 12px; white-space: pre-wrap; }");
      chatContent.append(".question-buttons { display: flex; gap: 10px; margin-bottom: 8px; }");
      chatContent.append(".question-btn { padding: 8px 20px; border: none; border-radius: 6px; cursor: pointer; font-weight: 500; font-size: 14px; }");
      chatContent.append(".question-btn-positive { background-color: #198754; color: white; }");
      chatContent.append(".question-btn-positive:hover { background-color: #157347; }");
      chatContent.append(".question-btn-negative { background-color: #dc3545; color: white; }");
      chatContent.append(".question-btn-negative:hover { background-color: #bb2d3b; }");
      chatContent.append(".question-btn-option { background-color: #0d6efd; color: white; display: block; width: 100%; text-align: left; margin-bottom: 5px; }");
      chatContent.append(".question-btn-option:hover { background-color: #0b5ed7; }");
      chatContent.append(".question-timer { font-size: 12px; color: #6c757d; }");
      chatContent.append(".question-answered { opacity: 0.6; }");
      chatContent.append(".question-answered .question-buttons { display: none; }");
      chatContent.append(".question-btn:disabled { opacity: 0.5; cursor: not-allowed; }");
      chatContent.append("</style>");
      chatContent.append("<script>");
      chatContent.append("var questionTimers = {};");
      chatContent.append("var answeredQuestions = {};");
      chatContent.append("function answerQuestion(questionId, positive, selectedOption) {");
      chatContent.append("  if (answeredQuestions[questionId]) return;");  // Prevent double-click
      chatContent.append("  answeredQuestions[questionId] = true;");
      chatContent.append("  if (questionTimers[questionId]) { clearInterval(questionTimers[questionId]); delete questionTimers[questionId]; }");
      chatContent.append("  var card = document.getElementById('question_' + questionId);");
      chatContent.append("  if (card) {");
      chatContent.append("    card.classList.add('question-answered');");
      chatContent.append("    var buttons = card.querySelectorAll('button');");
      chatContent.append("    buttons.forEach(function(btn) { btn.disabled = true; });");
      chatContent.append("    var timer = card.querySelector('.question-timer');");
      chatContent.append("    if (timer) { timer.textContent = 'Answered'; }");
      chatContent.append("  }");
      chatContent.append("  window.javaCallback(questionId, positive, selectedOption);");
      chatContent.append("}");
      chatContent.append("function startQuestionTimer(questionId, seconds) {");
      chatContent.append("  if (questionTimers[questionId]) return;");  // Prevent duplicate timers
      chatContent.append("  var timerEl = document.getElementById('timer_' + questionId);");
      chatContent.append("  if (!timerEl) return;");
      chatContent.append("  var remaining = seconds;");
      chatContent.append("  function updateTimer() {");
      chatContent.append("    var mins = Math.floor(remaining / 60);");
      chatContent.append("    var secs = remaining % 60;");
      chatContent.append("    timerEl.textContent = mins + ':' + (secs < 10 ? '0' : '') + secs;");
      chatContent.append("    if (remaining <= 0) {");
      chatContent.append("      clearInterval(questionTimers[questionId]);");
      chatContent.append("      delete questionTimers[questionId];");
      chatContent.append("      answerQuestion(questionId, false, -1);");
      chatContent.append("    }");
      chatContent.append("    remaining--;");
      chatContent.append("  }");
      chatContent.append("  updateTimer();");
      chatContent.append("  questionTimers[questionId] = setInterval(updateTimer, 1000);");
      chatContent.append("}");
      chatContent.append("</script>");
      chatContent.append("</head><body>");
      chatContent.append("<div id='chat-container'>");
      chatContent.append("</div>");
      chatContent.append("</body></html>");
   }

   /**
    * Create widget content
    */
   private void createContent()
   {
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      setLayout(layout);

      // Create browser for chat display
      chatBrowser = WidgetHelper.createBrowser(this, SWT.NONE, null);
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

      // Create BrowserFunction for JS-to-Java callback
      new BrowserFunction(chatBrowser, "javaCallback") {
         @Override
         public Object function(Object[] arguments)
         {
            if (arguments.length >= 3)
            {
               long questionId = ((Number)arguments[0]).longValue();
               boolean positive = (Boolean)arguments[1];
               int selectedOption = ((Number)arguments[2]).intValue();
               handleQuestionResponse(questionId, positive, selectedOption);
            }
            return null;
         }
      };

      // Initialize with welcome message
      addAssistantMessage("Hello! I'm Iris, your AI assistant. I can help you with setting up your monitoring environment, day-to-day operations, and analyzing problems. Feel free to ask any questions!");
      updateBrowserContent();

      Label separator = new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Composite commandArea = new Composite(this, SWT.NONE);
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

      if (withContext)
      {
         Composite contextArea = new Composite(commandArea, SWT.NONE);
         contextArea.setBackground(commandArea.getBackground());
         layout = new GridLayout();
         layout.marginHeight = 0;
         layout.marginWidth = 0;
         layout.numColumns = 2;
         contextArea.setLayout(layout);
         contextArea.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
         contextArea.moveAbove(chatInput);

         Label contextLabel = new Label(contextArea, SWT.NONE);
         contextLabel.setBackground(commandArea.getBackground());
         contextLabel.setText(i18n.tr("Context:"));
         contextLabel.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));

         contextName = new CLabel(contextArea, SWT.LEFT);
         contextName.setBackground(commandArea.getBackground());
         contextName.setText(i18n.tr("None"));
         contextName.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }

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
   }

   /**
    * Create chat session on the server
    * 
    * @param focusInput true to set focus to input field after creation
    */
   public void createChatSession(final boolean focusInput)
   {
      Job job = new Job(i18n.tr("Creating AI assistant chat"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            chatId = session.createAiAssistantChat(boundIncidentId);
            runInUIThread(() -> {
               chatInput.setEnabled(true);
               if (focusInput)
                  chatInput.setFocus();
            });
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
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(SessionNotification n)
   {
      if ((n.getCode() == SessionNotification.AI_QUESTION) && (n.getSubCode() == chatId))
      {
         getDisplay().asyncExec(() -> displayQuestion((AiQuestion)n.getObject()));
      }
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
      currentMessageId = addAssistantMessage(i18n.tr("Thinking..."));
      final String contextString = buildContextString();
      contextChanged = false;
      Job job = new Job(i18n.tr("Processing AI assistant query"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final String answer = session.updateAiAssistantChat(chatId, message, contextString);
               runInUIThread(() -> {
                  updateMessage(currentMessageId, answer);
                  currentMessageId = null;
               });
            }
            catch(Exception e)
            {
               runInUIThread(() -> {
                  updateMessage(currentMessageId, "Error: " + getErrorMessage());
                  currentMessageId = null;
               });
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

      // If original message was removed (due to question interruption), try current thinking message
      if (startPos == -1 && currentMessageId != null && !currentMessageId.equals(messageId))
      {
         searchPattern = "id='" + currentMessageId + "'>";
         startPos = currentContent.indexOf(searchPattern);
      }

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
    * Remove a message from the chat
    *
    * @param messageId ID of the message to remove
    */
   private void removeMessage(String messageId)
   {
      String currentContent = chatContent.toString();

      // Find the outer message div containing the bubble with given ID
      String searchPattern = "id='" + messageId + "'>";
      int bubbleStart = currentContent.indexOf(searchPattern);
      if (bubbleStart != -1)
      {
         // Find the start of outer message div (search backwards for "<div class='message")
         int messageStart = currentContent.lastIndexOf("<div class='message", bubbleStart);
         if (messageStart != -1)
         {
            // Find the end of the outer message div (two closing </div>s after bubble)
            int messageEnd = currentContent.indexOf("</div></div>", bubbleStart);
            if (messageEnd != -1)
            {
               messageEnd += 12; // Include "</div></div>"
               chatContent.delete(messageStart, messageEnd);
            }
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
    * Clear all chat messages and reset to initial state
    */
   public void clearChat()
   {
      chatContent.setLength(0);
      initializeHtmlTemplate();
      addAssistantMessage("Hello! I'm Iris, your AI assistant. I can help you with setting up your monitoring environment, day-to-day operations, and analyzing problems. Feel free to ask any questions!");
   }

   /**
    * Clear chat on the server and locally
    */
   public void clearChatSession()
   {
      new Job(i18n.tr("Clearing AI assistant chat"), view) {
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

   /**
    * Display a question from the AI agent
    *
    * @param question the question to display
    */
   private void displayQuestion(AiQuestion question)
   {
      pendingQuestion = question;
      chatInput.setEnabled(false);

      // Remove "Thinking..." message if present
      if (currentMessageId != null)
      {
         removeMessage(currentMessageId);
      }

      StringBuilder html = new StringBuilder();
      html.append("<div class='question-card' id='question_").append(question.getId()).append("'>");
      html.append("<div class='question-header'>Iris is asking:</div>");
      html.append("<div class='question-text'>").append(escapeHtml(question.getText())).append("</div>");

      String context = question.getContext();
      if (context != null && !context.isEmpty())
      {
         html.append("<div class='question-context'>").append(escapeHtml(context)).append("</div>");
      }

      html.append("<div class='question-buttons'>");
      if (question.isMultipleChoice())
      {
         List<String> options = question.getOptions();
         for (int i = 0; i < options.size(); i++)
         {
            html.append("<button class='question-btn question-btn-option' onclick='answerQuestion(")
                .append(question.getId()).append(", true, ").append(i).append(")'>")
                .append(escapeHtml(options.get(i))).append("</button>");
         }
      }
      else
      {
         html.append("<button class='question-btn question-btn-positive' onclick='answerQuestion(")
             .append(question.getId()).append(", true, -1)'>")
             .append(question.getPositiveLabel()).append("</button>");
         html.append("<button class='question-btn question-btn-negative' onclick='answerQuestion(")
             .append(question.getId()).append(", false, -1)'>")
             .append(question.getNegativeLabel()).append("</button>");
      }
      html.append("</div>");

      html.append("<div class='question-timer'>Expires in <span id='timer_")
          .append(question.getId()).append("'>").append(question.getTimeoutSeconds() / 60)
          .append(":").append(String.format("%02d", question.getTimeoutSeconds() % 60)).append("</span></div>");
      html.append("</div>");

      // Add inline script to start timer when page loads
      html.append("<script>startQuestionTimer(").append(question.getId()).append(", ")
          .append(question.getTimeoutSeconds()).append(");</script>");

      // Insert before closing </div></body></html>
      int insertPos = chatContent.lastIndexOf("</div></body></html>");
      chatContent.insert(insertPos, html.toString());

      updateBrowserContent();
   }

   /**
    * Handle user response to a question
    *
    * @param questionId question ID
    * @param positive true for positive response
    * @param selectedOption selected option for multiple choice (-1 for binary)
    */
   private void handleQuestionResponse(long questionId, boolean positive, int selectedOption)
   {
      if (pendingQuestion == null || pendingQuestion.getId() != questionId)
      {
         logger.warn("Received response for unknown question: " + questionId);
         return;
      }

      final AiQuestion question = pendingQuestion;
      pendingQuestion = null;

      // Mark question as answered in the HTML content
      markQuestionAnswered(questionId);

      // Add "Thinking..." message back while waiting for model's final response
      if (currentMessageId != null)
      {
         currentMessageId = addAssistantMessage(i18n.tr("Thinking..."));
      }

      Job job = new Job(i18n.tr("Sending response to AI assistant"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.answerAiQuestion(chatId, question.getId(), positive, selectedOption);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot send response to AI assistant");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Mark a question as answered in the HTML content
    *
    * @param questionId the question ID to mark as answered
    */
   private void markQuestionAnswered(long questionId)
   {
      String currentContent = chatContent.toString();

      // Find the question card and add the answered class
      String searchPattern = "id='question_" + questionId + "'";
      int cardStart = currentContent.indexOf(searchPattern);
      if (cardStart == -1)
         return;

      // Find the class attribute of the question-card div
      int divStart = currentContent.lastIndexOf("<div class='question-card'", cardStart);
      if (divStart == -1)
         return;

      // Replace the class to include question-answered
      String oldClass = "<div class='question-card'";
      String newClass = "<div class='question-card question-answered'";
      int classPos = currentContent.indexOf(oldClass, divStart);
      if (classPos != -1 && classPos < cardStart + 50)
      {
         chatContent.replace(classPos, classPos + oldClass.length(), newClass);
      }

      // Update timer text to "Answered"
      currentContent = chatContent.toString();
      String timerSearch = "id='timer_" + questionId + "'>";
      int timerStart = currentContent.indexOf(timerSearch);
      if (timerStart != -1)
      {
         int timerTextStart = timerStart + timerSearch.length();
         int timerTextEnd = currentContent.indexOf("</span>", timerTextStart);
         if (timerTextEnd != -1)
         {
            // Find the parent div with class question-timer and replace entire content
            int timerDivStart = currentContent.lastIndexOf("<div class='question-timer'>", timerStart);
            int timerDivEnd = currentContent.indexOf("</div>", timerStart);
            if (timerDivStart != -1 && timerDivEnd != -1)
            {
               chatContent.replace(timerDivStart, timerDivEnd + 6, "<div class='question-timer'>Answered</div>");
            }
         }
      }
   }

   /**
    * Get the chat input control
    *
    * @return chat input text control
    */
   public Text getChatInput()
   {
      return chatInput;
   }

   /**
    * Get the chat ID
    *
    * @return chat ID
    */
   public long getChatId()
   {
      return chatId;
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#setFocus()
    */
   @Override
   public boolean setFocus()
   {
      return chatInput.setFocus();
   }

   /**
    * Set chat context.
    *
    * @param context new context object
    */
   public void setContext(Object context)
   {
      if (!withContext || this.context == context)
         return;

      this.contextChanged = true;
      this.context = context;
      if (context == null)
      {
         contextName.setText(i18n.tr("None"));
         contextName.setImage(null);
         return;
      }

      if (context instanceof AbstractObject)
      {
         AbstractObject object = (AbstractObject)context;
         contextName.setImage(objectLabelProvider.getImage(object));
         contextName.setText(object.getNameWithAlias());
         return;
      }

      contextName.setImage(imageUnknownContext);
      contextName.setText(context.toString());
   }

   /**
    * Build context string for sending to server.
    *
    * @return context string or null
    */
   private String buildContextString()
   {
      if (context == null || !contextChanged)
         return null;

      if (context instanceof AbstractObject)
      {
         AbstractObject object = (AbstractObject)context;
         return "{\"type\":\"object\", \"object_name\":\"" + escapeForJson(object.getObjectName()) + "\", \"object_id\":" + object.getObjectId() + "}";
      }

      return context.toString();
   }

   /**
    * Escape text for JSON string.
    *
    * @param text text to escape
    * @return escaped text
    */
   private static String escapeForJson(String text)
   {
      return text.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n").replace("\r", "\\r");
   }
}
