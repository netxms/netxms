/**
 * NetXMS - open source network management system
 * Copyright (C) 2022-2023 Raden Solutions
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

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerConsoleListener;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.TextConsole;
import org.netxms.nxmc.base.widgets.TextConsole.IOConsoleOutputStream;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Server debug console
 */
public class ServerConsole extends View
{
   private final I18n i18n = LocalizationHelper.getI18n(ServerConsole.class);

   private NXCSession session;
   private ServerConsoleListener listener;
   private TextConsole console;
   private Combo commandInput;
   private IOConsoleOutputStream outputStream = null;
   private boolean connected = false;
   private Action actionClearOutput;
   private Action actionScrollLock;
   private Action actionCopy;
   private Action actionSave;
   private Action actionConnect;
   private Action actionDisconnect;

   /**
    * Create server console view
    */
   public ServerConsole()
   {
      super(LocalizationHelper.getI18n(ServerConsole.class).tr("Server Debug Console"), ResourceManager.getImageDescriptor("icons/tool-views/server-debug-console.png"), "tools.server-console", false);
      session = Registry.getSession();
      listener = new ServerConsoleListener() {
         @Override
         public void onConsoleOutput(String text)
         {
            if (connected && !console.isDisposed())
               outputStream.safeWrite(text.replaceAll("\n", "\n"));
         }
      };
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View origin)
   {
      super.postClone(origin);
      ServerConsole view = (ServerConsole)origin;
      console.setText(view.console.getText());   
      for(String s : view.commandInput.getItems())
         commandInput.add(s);
      commandInput.select(commandInput.getItems().length - 1);
      if (view.connected)
         connect();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      commandInput.add("");
      commandInput.select(0);
      outputStream.safeWrite("\n\n\u001b[34;1m*** Press ENTER to connect ***\u001b[0m\n");
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

      console = new TextConsole(parent, SWT.NONE);
      console.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      outputStream = console.newOutputStream();

      Label separator = new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Composite commandArea = new Composite(parent, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = 8;
      commandArea.setLayout(layout);
      commandArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      Label label = new Label(commandArea, SWT.NONE);
      label.setText(i18n.tr("Command:"));
      label.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));

      commandInput = new Combo(commandArea, SWT.BORDER);
      commandInput.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      commandInput.addKeyListener(new KeyListener() {
         @Override
         public void keyReleased(KeyEvent e)
         {
         }

         @Override
         public void keyPressed(KeyEvent e)
         {
            if (e.keyCode == 13)
               sendCommand();
         }
      });

      createActions();
      setConnectionState(false);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionClearOutput = new Action(i18n.tr("Clear &output"), SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            console.clear();
         }
      };
      addKeyBinding("M1+L", actionClearOutput);

      actionScrollLock = new Action(i18n.tr("&Scroll lock"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            console.setAutoScroll(!actionScrollLock.isChecked());
         }
      };
      actionScrollLock.setImageDescriptor(ResourceManager.getImageDescriptor("icons/scroll-lock.png"));
      actionScrollLock.setChecked(false);
      addKeyBinding("SCROLLLOCK", actionScrollLock);

      actionCopy = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY_TO_CLIPBOARD) {
         @Override
         public void run()
         {
            WidgetHelper.copyToClipboard(console.getCleanText());
         }
      };
      addKeyBinding("M1+C", actionCopy);

      actionSave = new Action(i18n.tr("&Save..."), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            DateFormat df = new SimpleDateFormat("yyyyMMdd-HHmmss");
            WidgetHelper.saveTextToFile(ServerConsole.this, "netxmsd-console-output-" + df.format(new Date()) + ".log", new String[] { "*.log", "*.*"}, new String[] { i18n.tr("Log files"), i18n.tr("All files")}, console.getCleanText());
         }
      };
      addKeyBinding("M1+S", actionSave);

      actionConnect = new Action(i18n.tr("Co&nnect"), ResourceManager.getImageDescriptor("icons/connect.png")) {
         @Override
         public void run()
         {
            connect();
         }
      };
      addKeyBinding("ENTER", actionConnect);

      actionDisconnect = new Action(i18n.tr("&Disconnect"), ResourceManager.getImageDescriptor("icons/disconnect.png")) {
         @Override
         public void run()
         {
            disconnect();
         }
      };
      addKeyBinding("M1+D", actionDisconnect);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionConnect);
      manager.add(actionDisconnect);
      manager.add(new Separator());
      manager.add(actionCopy);
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionClearOutput);
      manager.add(actionScrollLock);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionConnect);
      manager.add(actionDisconnect);
      manager.add(new Separator());
      manager.add(actionCopy);
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionClearOutput);
      manager.add(actionScrollLock);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeConsoleListener(listener);
      disconnect();
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      commandInput.setFocus();
   }

   /**
    * Connect to console
    */
   private void connect()
   {
      new Job(i18n.tr("Connecting to server debug console"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.openConsole();
            session.addConsoleListener(listener);
            runInUIThread(new Runnable() {               
               @Override
               public void run()
               {
                  setConnectionState(true);
                  commandInput.setFocus();
               }
            });
            outputStream.safeWrite("\n\n\u001b[32;1m*** CONNECTED ***\u001b[0m\n\n\u001b[1mNetXMS Server Remote Console V" + session.getServerVersion() + " Ready\n\n\u001b[0m");
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot connect to server debug console");
         }
      }.start();
   }

   /**
    * Disconnect from console
    */
   private void disconnect()
   {
      if (!connected)
         return;

      setConnectionState(false);
      new Job(i18n.tr("Disconnecting from server debug console"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.closeConsole();
            outputStream.safeWrite("\n\u001b[31;1m*** DISCONNECTED ***\u001b[0m\n");
            outputStream.safeWrite("\n\n\u001b[34;1m*** Press ENTER to connect ***\u001b[0m\n");
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot disconnect from server debug console");
         }
      }.start();
   }

   /**
    * Set new connection state.
    *
    * @param connected new connection state
    */
   private void setConnectionState(boolean connected)
   {
      this.connected = connected;
      actionConnect.setEnabled(!connected);
      actionDisconnect.setEnabled(connected);
      commandInput.setEnabled(connected);
   }

   /**
    * Send command to the server
    */
   private void sendCommand()
   {
      final String command = commandInput.getText().trim();
      if (command.isEmpty())
      {
         outputStream.safeWrite("\n");
         commandInput.select(commandInput.getItems().length - 1);
         return;
      }
      String[] items = commandInput.getItems();
      if (items.length == 1 || items[items.length - 2].compareToIgnoreCase(command) != 0)
         commandInput.add(command, items.length - 1);
      commandInput.select(commandInput.getItems().length - 1);

      outputStream.safeWrite("\u001b[33;1;7m " + command + " \u001b[0m\n");
      if (connected)
      {
         new Job(i18n.tr("Processing server debug console command"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               if (session.processConsoleCommand(command))
               {
                  runInUIThread(new Runnable() {               
                     @Override
                     public void run()
                     {
                        setConnectionState(false);
                     }
                  });
                  session.closeConsole();
                  outputStream.safeWrite("\n\u001b[31;1m*** DISCONNECTED ***\u001b[0m\n");
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot execute command on server");
            }
         }.start();
      }
      else
      {
         outputStream.safeWrite("\u001b[31;1mNOT CONNECTED\u001b[0m\n");
      }
   }
}
