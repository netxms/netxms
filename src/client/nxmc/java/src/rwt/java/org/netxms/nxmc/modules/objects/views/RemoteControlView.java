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

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.VncSocketEndpoint;
import org.netxms.nxmc.modules.objecttools.TcpPortForwarder;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Remote control view (embedded VNC viewer)
 */
public class RemoteControlView extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(RemoteControlView.class);

   private Browser browser;

   /**
    * Create VNC viewer
    *
    * @param node target node
    * @param contextId context ID
    */
   public RemoteControlView(AbstractNode node, long contextId)
   {
      super(LocalizationHelper.getI18n(RemoteControlView.class).tr("Remote Control"),
            ResourceManager.getImageDescriptor("icons/object-views/remote-desktop.png"), "objects.vncviewer", node.getObjectId(), contextId, false);
   }

   /**
    * Create VNC viewer
    */
   public RemoteControlView()
   {
      super(LocalizationHelper.getI18n(RemoteControlView.class).tr("Remote Control"),
      ResourceManager.getImageDescriptor("icons/object-views/remote-desktop.png"), null, 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      browser = new Browser(parent, SWT.NONE);
      connect();
   }

   /**
    * Connect to remote system
    */
   private void connect()
   {
      final Node node = (Node)getObject();
      final NXCSession session = Registry.getSession();

      Job job = new Job(i18n.tr("Connecting to VNC server"), this) {
         private TcpPortForwarder tcpPortForwarder;

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            tcpPortForwarder = new TcpPortForwarder(session, node.getObjectId(), (node.getCapabilities() & AbstractNode.NC_IS_LOCAL_VNC) != 0, node.getVncPort(), 0);
            tcpPortForwarder.setDisplay(getDisplay());
            tcpPortForwarder.setMessageArea(RemoteControlView.this);
            tcpPortForwarder.run();

            VncSocketEndpoint.registerForwarder(tcpPortForwarder);
            runInUIThread(() -> {
               StringBuilder url = new StringBuilder();
               url.append(RWT.getResourceManager().getLocation("vncviewer/vnc.html"));
               url.append("?autoconnect=true&path=");
               String cp = RWT.getRequest().getContextPath();
               if (!cp.isEmpty())
               {
                  url.append(cp.substring(1)); // without leading /
                  url.append('/');
               }
               url.append("vncviewer/");
               url.append(tcpPortForwarder.getId());
               if ((node.getVncPassword() != null) && !node.getVncPassword().isEmpty())
               {
                  try
                  {
                     url.append("&password=");
                     url.append(URLEncoder.encode(node.getVncPassword(), "UTF-8"));
                  }
                  catch(UnsupportedEncodingException e)
                  {
                  }
               }
               browser.setUrl(url.toString());
            });
         }

         /**
          * @see org.netxms.nxmc.base.jobs.Job#jobFailureHandler(java.lang.Exception)
          */
         @Override
         protected void jobFailureHandler(Exception e)
         {
            tcpPortForwarder.close();
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("VNC connection error");
         }
      };
      job.setUser(false);
      job.start();
   }
}
