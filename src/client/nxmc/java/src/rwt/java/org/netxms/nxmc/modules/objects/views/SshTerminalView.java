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

import java.util.UUID;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ssh.SshConnectionSettings;
import org.netxms.nxmc.modules.objects.ssh.TerminalTransport;
import org.netxms.nxmc.modules.objects.views.helpers.SshTerminalSocketEndpoint;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * SSH terminal view (web): xterm.js in browser widget connected to terminal transport through websocket endpoint. Unknown host
 * keys in direct mode are accepted and recorded, because the SSH client runs on the web server and user cannot inspect
 * fingerprints out of band; changed keys are still rejected.
 */
public class SshTerminalView extends AdHocObjectView
{
   private SshConnectionSettings settings;
   private Browser browser;
   private UUID terminalId = null;

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
      browser = new Browser(parent, SWT.NONE);

      AbstractNode node = (AbstractNode)getObject();
      TerminalTransport transport = settings.createTransport(Registry.getSession(), node, (message) -> true);
      terminalId = SshTerminalSocketEndpoint.registerTransport(transport, RWT.getUISession().getHttpSession().getId());

      StringBuilder url = new StringBuilder();
      url.append(RWT.getResourceManager().getLocation("terminal/terminal.html"));
      url.append("?path=");
      String cp = RWT.getRequest().getContextPath();
      if (!cp.isEmpty())
      {
         url.append(cp.substring(1)); // without leading /
         url.append('/');
      }
      url.append("sshterminal/");
      url.append(terminalId);
      browser.setUrl(url.toString());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      browser.setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if (terminalId != null)
         SshTerminalSocketEndpoint.closeTransport(terminalId);
      super.dispose();
   }
}
