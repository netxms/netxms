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
package org.netxms.nxmc.modules.tools.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MarkdownViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * AI assistant chat view
 */
public class AiAssistantChatView extends View
{
   private final I18n i18n = LocalizationHelper.getI18n(AiAssistantChatView.class);

   private NXCSession session;
   private MarkdownViewer chatOutput;
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
      AiAssistantChatView view = (AiAssistantChatView)origin;
      chatOutput.setText(view.chatOutput.getText());
      chatInput.setText(view.chatInput.getText());
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

      chatOutput = new MarkdownViewer(parent, SWT.NONE);
      chatOutput.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      chatOutput.setText(
            "!v Hello! I’m Iris, your AI assistant. I can help you with setting up your monitoring environment, day-to-day operations, and analyzing problems. Feel free to ask any questions!\n\n");

      Label separator = new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Composite commandArea = new Composite(parent, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = 8;
      commandArea.setLayout(layout);
      commandArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      Label label = new Label(commandArea, SWT.NONE);
      label.setText(i18n.tr("Query:"));
      label.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));

      chatInput = new Text(commandArea, SWT.BORDER);
      chatInput.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
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

      createActions();
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
                  session.clearAiAssistantChat();
                  runInUIThread(() -> chatOutput.setText(""));
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
      final String prompt = chatInput.getText().trim();
      chatInput.setText("");
      if (prompt.isEmpty())
         return;

      chatOutput.setText(chatOutput.getText() + "\n! " + prompt + "\n");
      chatInput.setEnabled(false);
      new Job(i18n.tr("Processing server debug console command"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final String answer = session.queryAiAssistant(prompt);
               runInUIThread(() -> {
                  chatOutput.setText(chatOutput.getText() + "\n" + answer + "\n");
               });
            }
            catch(Exception e)
            {
               runInUIThread(() -> {
                  chatOutput.setText(chatOutput.getText() + "\n!! " + getErrorMessage() + "\n");
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
      }.start();
   }
}
