/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.TextOutputListener;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.interfaces.PollingTarget;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.StyledText;
import org.netxms.nxmc.base.widgets.helpers.StyleRange;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Forced node poll view
 */
public class ObjectPollerView extends AdHocObjectView implements TextOutputListener
{
   private static final Color COLOR_ERROR = new Color(Display.getCurrent(), 192, 0, 0);
   private static final Color COLOR_WARNING = new Color(Display.getCurrent(), 255, 128, 0);
   private static final Color COLOR_INFO = new Color(Display.getCurrent(), 0, 128, 0);
   private static final Color COLOR_LOCAL = new Color(Display.getCurrent(), 0, 0, 192);

   private final I18n i18n = LocalizationHelper.getI18n(ObjectPollerView.class);

   private PollingTarget target;
   private ObjectPollType pollType;
   private StyledText textArea;
   private Action actionRestart;
   private Action actionClearOutput;
   private boolean startPollOnOpen = false;

   private static String getViewName(ObjectPollType type)
   {
      String[] names = {
         "",
         LocalizationHelper.getI18n(ObjectPollerView.class).tr("Status Poll"), 
         LocalizationHelper.getI18n(ObjectPollerView.class).tr("Configuration Poll (Full)"),
         LocalizationHelper.getI18n(ObjectPollerView.class).tr("Interface Poll"), 
         LocalizationHelper.getI18n(ObjectPollerView.class).tr("Topology Poll"),
         LocalizationHelper.getI18n(ObjectPollerView.class).tr("Configuration Poll"), 
         LocalizationHelper.getI18n(ObjectPollerView.class).tr("Instance Discovery Poll"), 
         LocalizationHelper.getI18n(ObjectPollerView.class).tr("Routing Table Poll"),
         LocalizationHelper.getI18n(ObjectPollerView.class).tr("Network Discovery Poll"),
         LocalizationHelper.getI18n(ObjectPollerView.class).tr("Automatic Binding Poll"),
         LocalizationHelper.getI18n(ObjectPollerView.class).tr("Map Update Poll")
      };
      return names[type.getValue()];
   }

   /**
    * Create object poll view.
    *
    * @param object object to poll
    * @param type poll type
    * @param contextId context ID
    * @param startPollOnOpen true to start poll after open
    */
   public ObjectPollerView(AbstractObject object, ObjectPollType pollType, long contextId, boolean startPollOnOpen)
   {
      super(getViewName(pollType), ResourceManager.getImageDescriptor("icons/object-views/poller_view.png"), "objects.poll." + pollType.toString().toLowerCase(), object.getObjectId(), contextId, false);
      target = (PollingTarget)object;
      this.pollType = pollType;
      this.startPollOnOpen = startPollOnOpen;
   }

   /**
    * Default constructor
    */
   protected ObjectPollerView()
   {
      super(null, ResourceManager.getImageDescriptor("icons/object-views/poller_view.png"), null, 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      ObjectPollerView view = (ObjectPollerView)super.cloneView();
      view.target = target;
      view.pollType = pollType;
      return view;
   }   

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View origin)
   {
      super.postClone(origin);
      textArea.replaceContent(((ObjectPollerView)origin).textArea);
      Registry.getPollManager().followPoll(target, pollType, this);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      textArea = new StyledText(parent, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      textArea.setEditable(false);
      textArea.setScrollOnAppend(true);
      textArea.setFont(JFaceResources.getTextFont());

      createActions();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      if (startPollOnOpen)
      {
         startPoll();
         startPollOnOpen = false;
      }
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

      actionClearOutput = new Action(i18n.tr("&Clear output"), SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            textArea.setText("");
         }
      };
      addKeyBinding("M1+L", actionClearOutput);
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
      actionRestart.setEnabled(false);      
      Registry.getPollManager().startNewPoll(target, pollType, this);
   }

   /**
    * @see org.netxms.client.TextOutputListener#messageReceived(java.lang.String)
    */
   @Override
   public void messageReceived(String text)
   {
      getDisplay().asyncExec(() -> {
         if (!textArea.isDisposed())
            addPollerMessage(text);
      });
   }

   /**
    * @see org.netxms.client.TextOutputListener#setStreamId(long)
    */
   @Override
   public void setStreamId(long streamId)
   {
   }

   /**
    * @see org.netxms.client.TextOutputListener#onFailure(java.lang.Exception)
    */
   @Override
   public void onFailure(Exception exception)
   {
      getDisplay().asyncExec(() -> {
         if (!textArea.isDisposed())
            actionRestart.setEnabled(true);
      });
   }

   /**
    * @see org.netxms.client.TextOutputListener#onSuccess()
    */
   @Override
   public void onSuccess()
   {
      getDisplay().asyncExec(() -> {
         if (!textArea.isDisposed())
            actionRestart.setEnabled(true);
      });
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#dispose()
    */
   @Override
   public void dispose()
   {
      Registry.getPollManager().removePollListener(target, pollType, this);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("pollType", pollType.toString());
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);

      pollType = ObjectPollType.valueOf(memento.getAsString("pollType"));
      setName(getViewName(pollType));

      long objectId = memento.getAsLong("objectId", 0);
      target = session.findObjectById(objectId, PollingTarget.class);
      if (target == null)
         throw new ViewNotRestoredException(i18n.tr("Object no longer exists or is not accessible"));
   }
}
