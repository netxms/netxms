/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.ChatBot;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.dialogs.ChatBotPropertiesDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ChatBotFilter;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ChatBotLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ChatBotListComparator;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Chat bots view
 */
public class ChatBots extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(ChatBots.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DESCRIPTION = 1;
   public static final int COLUMN_DRIVER = 2;
   public static final int COLUMN_HEALTH = 3;
   public static final int COLUMN_SESSIONS = 4;
   public static final int COLUMN_LAST_MESSAGE = 5;
   public static final int COLUMN_ERROR_MESSAGE = 6;

   private NXCSession session;
   private SessionListener listener;
   private SortableTableViewer viewer;
   private Action actionNewBot;
   private Action actionEditBot;
   private Action actionDeleteBot;

   /**
    * Create chat bots view
    */
   public ChatBots()
   {
      super(LocalizationHelper.getI18n(ChatBots.class).tr("Chat Bots"), ResourceManager.getImageDescriptor("icons/config-views/nchannels.png"), "config.chat-bots", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final int[] widths = { 160, 250, 100, 80, 80, 150, 400 };
      final String[] names = { i18n.tr("Name"), i18n.tr("Description"), i18n.tr("Driver"), i18n.tr("Health"), i18n.tr("Sessions"), i18n.tr("Last message"), i18n.tr("Error message") };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI, "ChatBotList");
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ChatBotLabelProvider());
      viewer.setComparator(new ChatBotListComparator());
      ChatBotFilter filter = new ChatBotFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
      viewer.addDoubleClickListener((e) -> editBot());
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            actionEditBot.setEnabled(selection.size() == 1);
            actionDeleteBot.setEnabled(selection.size() > 0);
         }
      });

      createActions();
      createContextMenu();

      final Display display = viewer.getControl().getDisplay();
      listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.CHAT_BOT_CHANGED)
            {
               display.asyncExec(() -> refresh());
            }
         }
      };
      session.addListener(listener);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(listener);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionNewBot = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewBot();
         }
      };
      addKeyBinding("M1+N", actionNewBot);

      actionEditBot = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editBot();
         }
      };
      actionEditBot.setEnabled(false);
      addKeyBinding("M1+E", actionEditBot);

      actionDeleteBot = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteBots();
         }
      };
      actionDeleteBot.setEnabled(false);
      addKeyBinding("M1+D", actionDeleteBot);
   }

   /**
    * Create pop-up menu for chat bot list
    */
   private void createContextMenu()
   {
      MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener((m) -> fillContextMenu(m));

      Menu menu = manager.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    *
    * @param manager Menu manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionNewBot);
      manager.add(actionEditBot);
      manager.add(actionDeleteBot);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNewBot);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      Action resetAction = viewer.getResetColumnOrderAction();
      if (resetAction != null)
         manager.add(resetAction);
      Action showAllAction = viewer.getShowAllColumnsAction();
      if (showAllAction != null)
         manager.add(showAllAction);
      Action autoSizeAction = viewer.getAutoSizeColumnsAction();
      if (autoSizeAction != null)
      {
         manager.add(new Separator());
         manager.add(autoSizeAction);
      }
      manager.add(new Separator());
      manager.add(actionNewBot);
   }

   /**
    * Refresh
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Reading list of chat bots"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<ChatBot> bots = session.getChatBots();
            runInUIThread(() -> viewer.setInput(bots));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of chat bots");
         }
      }.start();
   }

   /**
    * Create new chat bot
    */
   private void createNewBot()
   {
      final ChatBotPropertiesDialog dlg = new ChatBotPropertiesDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      final ChatBot bot = dlg.getChatBot();

      new Job(i18n.tr("Creating chat bot"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.createChatBot(bot);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create chat bot");
         }
      }.start();
   }

   /**
    * Edit selected chat bot
    */
   private void editBot()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final ChatBot bot = (ChatBot)selection.getFirstElement();
      final String oldName = bot.getName();
      final ChatBotPropertiesDialog dlg = new ChatBotPropertiesDialog(getWindow().getShell(), bot);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Updating chat bot"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (!bot.getName().equals(oldName))
            {
               session.renameChatBot(oldName, bot.getName());
            }
            session.updateChatBot(bot);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update chat bot");
         }
      }.start();
   }

   /**
    * Delete selected chat bots
    */
   private void deleteBots()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Chat Bots"),
            i18n.tr("Are you sure you want to delete selected chat bots?")))
         return;

      final List<String> bots = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
      {
         if (o instanceof ChatBot)
            bots.add(((ChatBot)o).getName());
      }

      new Job(i18n.tr("Deleting chat bot"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(String name : bots)
               session.deleteChatBot(name);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete chat bot");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}
