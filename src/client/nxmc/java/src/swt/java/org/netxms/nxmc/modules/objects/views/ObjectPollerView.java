/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StyleRange;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputListener;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.interfaces.PollingTarget;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Forced node poll view
 */
public class ObjectPollerView extends AdHocObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectPollerView.class);

   private static final String[] POLL_NAME = {
         "",
         i18n.tr("Status Poll"), 
         i18n.tr("Configuration Poll (Full)"),
         i18n.tr("Interface Poll"), 
         i18n.tr("Topology Poll"),
         i18n.tr("Configuration Poll"), 
         i18n.tr("Instance Discovery Poll"), 
         i18n.tr("Routing Table Poll"),
         i18n.tr("Network Discovery Poll"),
         i18n.tr("Automatic Binding Poll")
      };
   private static final Color COLOR_ERROR = new Color(Display.getCurrent(), 192, 0, 0);
   private static final Color COLOR_WARNING = new Color(Display.getCurrent(), 255, 128, 0);
   private static final Color COLOR_INFO = new Color(Display.getCurrent(), 0, 128, 0);
   private static final Color COLOR_LOCAL = new Color(Display.getCurrent(), 0, 0, 192);

   private NXCSession session;
   private PollingTarget target;
   private ObjectPollType pollType;
   private Display display;
   private StyledText textArea;
   private boolean pollActive = false;
   private Action actionRestart;
   private Action actionClearOutput;

   /**
    * Create object poll view.
    *
    * @param object object to poll
    * @param type poll type
    */
   public ObjectPollerView(AbstractObject object, ObjectPollType type)
   {
      super(POLL_NAME[type.getValue()], ResourceManager.getImageDescriptor("icons/object-views/poller_view.png"), "ObjectPoll." + type, object.getObjectId(), false);

      session = Registry.getSession();
      display = Display.getCurrent();

      target = (PollingTarget)object;
      pollType = type;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      textArea = new StyledText(parent, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      textArea.setEditable(false);
      textArea.setFont(JFaceResources.getTextFont());

      createActions();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRestart = new Action(i18n.tr("&Restart poll"), SharedIcons.RESTART) {
         @Override
         public void run()
         {
            startPoll();
         }
      };
      actionRestart.setActionDefinitionId("org.netxms.ui.eclipse.objectmanager.commands.restart_poller"); 

      actionClearOutput = new Action(i18n.tr("&Clear output"), SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            textArea.setText(""); //$NON-NLS-1$
         }
      };
      actionClearOutput.setActionDefinitionId("org.netxms.ui.eclipse.objectmanager.commands.clear_output");
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      super.fillLocalToolBar(manager);
      manager.add(actionRestart);
      manager.add(actionClearOutput);
   }

   /**
    * Add poller message to text area
    * 
    * @param message poller message
    */
   private void addPollerMessage(String message)
   {
      Date now = new Date();
      textArea.append("[" + DateFormatFactory.getDateTimeFormat().format(now) + "] ");

      int index = message.indexOf(0x7F);
      if (index != -1)
      {
         textArea.append(message.substring(0, index));
         char code = message.charAt(index + 1);
         int lastPos = textArea.getCharCount();
         final String msgPart = message.substring(index + 2);
         textArea.append(msgPart);

         StyleRange style = new StyleRange();
         style.start = lastPos;
         style.length = msgPart.length();
         style.foreground = getTextColor(code);
         textArea.setStyleRange(style);
      }
      else
      {
         textArea.append(message);
      }

      textArea.setCaretOffset(textArea.getCharCount());
      textArea.setTopIndex(textArea.getLineCount() - 1);
   }

   /**
    * Get color from color code
    * 
    * @param code
    * @return
    */
   private Color getTextColor(char code)
   {
      switch(code)
      {
         case 'e':
            return COLOR_ERROR;
         case 'w':
            return COLOR_WARNING;
         case 'i':
            return COLOR_INFO;
         case 'l':
            return COLOR_LOCAL;
      }
      return null;
   }

   /**
    * Start poll
    */
   public void startPoll()
   {
      if (pollActive)
         return;
      pollActive = true;
      actionRestart.setEnabled(false);

      addPollerMessage("\u007Fl**** Poll request sent to server ****\r\n"); //$NON-NLS-1$

      final TextOutputListener listener = new TextOutputListener() {
         @Override
         public void messageReceived(final String message)
         {
            display.asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  if (!textArea.isDisposed())
                     addPollerMessage(message);
               }
            });
         }

         @Override
         public void setStreamId(long streamId)
         {
         }

         @Override
         public void onError()
         {
         }
      };

      Job job = new Job(String.format(i18n.tr("Node poll: %s [%d]"), target.getObjectName(), target.getObjectId()), this) {

         @Override
         protected void run(IProgressMonitor monitor) 
         {
            try
            {
               session.pollObject(target.getObjectId(), pollType, listener);
               onPollComplete(true, null);
            }
            catch(Exception e)
            {
               onPollComplete(false, e.getMessage());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      };
      job.setSystem(true);
      job.start();
   }

   /**
    * Poll completion handler
    * 
    * @param success
    * @param errorMessage
    */
   private void onPollComplete(final boolean success, final String errorMessage)
   {
      display.asyncExec(new Runnable() {
         @Override
         public void run()
         {
            if (textArea.isDisposed())
               return;

            if (success)
            {
               addPollerMessage("\u007Fl**** Poll completed successfully ****\r\n\r\n"); //$NON-NLS-1$
            }
            else
            {
               addPollerMessage(String.format("\u007FePOLL ERROR: %s\r\n", errorMessage)); //$NON-NLS-1$
               addPollerMessage("\u007Fl**** Poll failed ****\r\n\r\n"); //$NON-NLS-1$
            }
            pollActive = false;
            actionRestart.setEnabled(true);
         }
      });
   }
}
