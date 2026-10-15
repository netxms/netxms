/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.terminal.connector.ITerminalConnector;
import org.eclipse.terminal.connector.ITerminalControl;
import org.eclipse.terminal.connector.TerminalConnectors;
import org.eclipse.terminal.connector.TerminalState;
import org.eclipse.terminal.connector.provider.AbstractTerminalConnector;
import org.eclipse.terminal.control.ITerminalListener;
import org.eclipse.terminal.control.ITerminalViewControl;
import org.eclipse.terminal.control.TerminalTitleRequestor;
import org.eclipse.terminal.control.TerminalViewControlFactory;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ssh.SshConnectionSettings;
import org.netxms.nxmc.modules.objects.ssh.TerminalTransport;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * SSH terminal view (desktop): native VT100/xterm terminal control connected to a node via selected transport.
 */
public class SshTerminalView extends AdHocObjectView
{
   private static final Logger logger = LoggerFactory.getLogger(SshTerminalView.class);

   private final I18n i18n = LocalizationHelper.getI18n(SshTerminalView.class);

   private SshConnectionSettings settings;
   private ITerminalViewControl terminal;
   private TransportConnector connector;
   private Action actionConnect;
   private Action actionDisconnect;
   private Action actionClear;
   private Action actionCopy;
   private Action actionPaste;

   /**
    * Create SSH terminal view.
    *
    * @param node target node
    * @param contextId context ID
    * @param settings connection settings
    */
   public SshTerminalView(AbstractNode node, long contextId, SshConnectionSettings settings)
   {
      super(LocalizationHelper.getI18n(SshTerminalView.class).tr("SSH Terminal"), ResourceManager.getImageDescriptor("icons/object-tools/terminal.png"), "objects.ssh-terminal", node.getObjectId(), contextId, false);
      this.settings = settings;
   }

   /**
    * Create SSH terminal view (for view restoration framework; such views are never restored).
    */
   public SshTerminalView()
   {
      super(LocalizationHelper.getI18n(SshTerminalView.class).tr("SSH Terminal"), ResourceManager.getImageDescriptor("icons/object-tools/terminal.png"), "objects.ssh-terminal", 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.AdHocObjectView#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      throw new ViewNotRestoredException("SSH terminal sessions cannot be restored");
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      parent.setLayout(new FillLayout());

      connector = new TransportConnector();
      ITerminalConnector terminalConnector = TerminalConnectors.create("nxmc-ssh", "SSH", () -> connector);
      terminal = TerminalViewControlFactory.makeControl(new ITerminalListener() {
         @Override
         public void setState(TerminalState state)
         {
            getDisplay().asyncExec(() -> updateActions(state));
         }

         @Override
         public void setTerminalSelectionChanged()
         {
         }

         @Override
         public void setTerminalTitle(String title, TerminalTitleRequestor requestor)
         {
         }
      }, parent, new ITerminalConnector[] { terminalConnector }, true);
      terminal.setConnector(terminalConnector);
      terminal.setCharset(StandardCharsets.UTF_8);

      createActions();
      updateActions(TerminalState.CLOSED);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      connect();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionConnect = new Action(i18n.tr("&Connect"), ResourceManager.getImageDescriptor("icons/connect.png")) {
         @Override
         public void run()
         {
            connect();
         }
      };

      actionDisconnect = new Action(i18n.tr("&Disconnect"), ResourceManager.getImageDescriptor("icons/disconnect.png")) {
         @Override
         public void run()
         {
            terminal.disconnectTerminal();
         }
      };

      actionClear = new Action(i18n.tr("C&lear terminal"), SharedIcons.CLEAR) {
         @Override
         public void run()
         {
            terminal.clearTerminal();
         }
      };

      actionCopy = new Action(i18n.tr("&Copy"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            terminal.copy();
         }
      };

      actionPaste = new Action(i18n.tr("&Paste"), SharedIcons.PASTE) {
         @Override
         public void run()
         {
            terminal.paste();
         }
      };
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionConnect);
      manager.add(actionDisconnect);
      manager.add(new Separator());
      manager.add(actionClear);
      manager.add(actionCopy);
      manager.add(actionPaste);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionConnect);
      manager.add(actionDisconnect);
      manager.add(new Separator());
      manager.add(actionClear);
      manager.add(actionCopy);
      manager.add(actionPaste);
   }

   /**
    * Update action state according to terminal state
    *
    * @param state terminal state
    */
   private void updateActions(TerminalState state)
   {
      if (terminal.isDisposed())
         return;
      actionConnect.setEnabled(state == TerminalState.CLOSED);
      actionDisconnect.setEnabled(state != TerminalState.CLOSED);
   }

   /**
    * Connect terminal (dialog for settings was already shown by caller)
    */
   private void connect()
   {
      if (terminal.getState() != TerminalState.CLOSED)
         return;
      clearMessages();
      terminal.connectTerminal();
      terminal.setFocus();
   }

   /**
    * Confirm unknown host key (called from transport thread)
    *
    * @param message message from SSH client
    * @return true if key accepted
    */
   private boolean confirmHostKey(String message)
   {
      final boolean[] result = new boolean[1];
      getDisplay().syncExec(() -> {
         result[0] = MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Host Key Verification"), message);
      });
      return result[0];
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      terminal.setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if (terminal != null)
         terminal.disposeTerminal();
      super.dispose();
   }

   /**
    * Terminal connector feeding terminal control from terminal transport
    */
   private class TransportConnector extends AbstractTerminalConnector
   {
      private volatile TerminalTransport transport = null;
      private volatile boolean connected = false;
      private int columns = 80;
      private int rows = 24;
      private final OutputStream remoteStream = new OutputStream() {
         @Override
         public void write(int b) throws IOException
         {
            write(new byte[] { (byte)b }, 0, 1);
         }

         @Override
         public void write(byte[] b, int off, int len) throws IOException
         {
            TerminalTransport t = transport;
            if (connected && (t != null))
            {
               OutputStream out = t.getOutputStream();
               out.write(b, off, len);
               out.flush();
            }
         }
      };

      /**
       * @see org.eclipse.terminal.connector.provider.AbstractTerminalConnector#connect(org.eclipse.terminal.connector.ITerminalControl)
       */
      @Override
      public void connect(ITerminalControl control)
      {
         super.connect(control);
         control.setState(TerminalState.CONNECTING);

         final NXCSession session = Registry.getSession();
         final AbstractNode node = (AbstractNode)getObject();
         final TerminalTransport t = settings.createTransport(session, node, SshTerminalView.this::confirmHostKey);
         transport = t;

         Thread thread = new Thread(() -> {
            try
            {
               t.connect(columns, rows);
               connected = true;
               control.setState(TerminalState.CONNECTED);

               InputStream in = t.getInputStream();
               OutputStream out = control.getRemoteToTerminalOutputStream();
               byte[] buffer = new byte[16384];
               int bytes;
               while((bytes = in.read(buffer)) > 0)
               {
                  out.write(buffer, 0, bytes);
                  out.flush();
               }
               if (connected)
               {
                  out.write("\r\n\033[33m*** Connection closed ***\033[0m\r\n".getBytes(StandardCharsets.UTF_8));
                  out.flush();
               }
            }
            catch(Exception e)
            {
               String message = (e.getLocalizedMessage() != null) ? e.getLocalizedMessage() : e.getClass().getName();
               if (connected)
               {
                  logger.debug("SSH terminal session error", e);
                  getDisplay().asyncExec(() -> {
                     if (!terminal.isDisposed())
                        addMessage(MessageArea.ERROR, i18n.tr("Connection error: {0}", message));
                  });
               }
               else
               {
                  logger.debug("SSH terminal connection failure", e);
                  control.setMsg(i18n.tr("Cannot connect to {0}: {1}", node.getObjectName(), message));
               }
            }
            finally
            {
               connected = false;
               t.close();
               control.setState(TerminalState.CLOSED);
            }
         }, "SshTerminal-" + node.getObjectId());
         thread.setDaemon(true);
         thread.start();
      }

      /**
       * @see org.eclipse.terminal.connector.provider.AbstractTerminalConnector#doDisconnect()
       */
      @Override
      protected void doDisconnect()
      {
         connected = false;
         TerminalTransport t = transport;
         if (t != null)
            t.close();
      }

      /**
       * @see org.eclipse.terminal.connector.provider.AbstractTerminalConnector#getTerminalToRemoteStream()
       */
      @Override
      public OutputStream getTerminalToRemoteStream()
      {
         return remoteStream;
      }

      /**
       * @see org.eclipse.terminal.connector.provider.AbstractTerminalConnector#setTerminalSize(int, int)
       */
      @Override
      public void setTerminalSize(int newWidth, int newHeight)
      {
         if ((newWidth < 1) || (newHeight < 1))
            return;
         columns = newWidth;
         rows = newHeight;
         TerminalTransport t = transport;
         if (connected && (t != null))
            t.resize(newWidth, newHeight);
      }

      /**
       * @see org.eclipse.terminal.connector.provider.AbstractTerminalConnector#getSettingsSummary()
       */
      @Override
      public String getSettingsSummary()
      {
         return getObjectName();
      }
   }
}
