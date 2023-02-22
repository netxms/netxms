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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
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
   private static final I18n i18n = LocalizationHelper.getI18n(ServerConsole.class);

   private NXCSession session;
   private ServerConsoleListener listener;
   private TextConsole console;
   private Text commandInput;
   private IOConsoleOutputStream outputStream = null;
   private boolean connected = false;
   private Action actionClearOutput;
   private Action actionScrollLock;
   private Action actionCopy;
   private Action actionSave;

   /**
    * Create server console view
    */
   public ServerConsole()
   {
      super(i18n.tr("Server Debug Console"), ResourceManager.getImageDescriptor("icons/tool-views/server-debug-console.png"), "ServerConsole", false);
      session = Registry.getSession();
      listener = new ServerConsoleListener() {
         @Override
         public void onConsoleOutput(String text)
         {
            if (console.isDisposed())
            {
               session.removeConsoleListener(listener);
            }
            else if (connected)
            {
               outputStream.safeWrite(text.replaceAll("\n", "\r\n"));
            }
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

      commandInput = new Text(commandArea, SWT.BORDER);
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
            WidgetHelper.saveTextToFile(ServerConsole.this, "netxmsd-console-output-" + df.format(new Date()) + ".log", console.getCleanText());
         }
      };
      addKeyBinding("M1+S", actionSave);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
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
      manager.add(actionCopy);
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionClearOutput);
      manager.add(actionScrollLock);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      if (!connected)
         connect();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      connect();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#deactivate()
    */
   @Override
   public void deactivate()
   {
      disconnect();
      super.deactivate();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
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
            connected = true;
            outputStream.safeWrite("\u001b[1mNetXMS Server Remote Console V" + session.getServerVersion() + " Ready\r\n\r\n\u001b[0m");
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
      connected = false;
      session.removeConsoleListener(listener);
      new Job(i18n.tr("Disconnecting from server debug console"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.closeConsole();
            outputStream.safeWrite("\r\n\u001b[31;1m*** DISCONNECTED ***\u001b[0m\r\n");
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot disconnect from server debug console");
         }
      }.start();
   }

   /**
    * Send command to the server
    */
   private void sendCommand()
   {
      final String command = commandInput.getText().trim();
      commandInput.setText("");
      if (command.isEmpty())
         return;

      outputStream.safeWrite("\u001b[33;1;7m " + command + " \u001b[0m\r\n");
      if (connected)
      {
         new Job(i18n.tr("Processing server debug console command"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               if (session.processConsoleCommand(command))
               {
                  connected = false;
                  session.closeConsole();
                  outputStream.safeWrite("\r\n\u001b[31;1m*** DISCONNECTED ***\u001b[0m\r\n");
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
         outputStream.safeWrite("\u001b[31;1mNOT CONNECTED\u001b[0m\r\n");
      }
   }
}
