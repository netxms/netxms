/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Raden Soultions
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
package org.netxms.ui.eclipse.objecttools.widgets;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import org.eclipse.core.runtime.Platform;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.TcpProxy;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.TcpPortForwarder;

/**
 * Local command executer and result provider widget
 */
public class LocalCommandExecutor extends AbstractObjectToolExecutor
{
   private ObjectTool tool;
   private Process process;
   private TcpPortForwarder tcpPortForwarder = null;
   private boolean processRunning = false;
   private String command;
   private Object mutex = new Object();

   /**
    * Constructor 
    * 
    * @param result Area area to display result
    * @param view parent view
    * @param command command to execute
    */
   public LocalCommandExecutor(Composite resultArea, ViewPart view, ObjectContext objectContext, ActionSet actionSet, ObjectTool tool, String command)
   {
      super(resultArea, view, objectContext, actionSet);
      this.tool = tool;
      this.command = command;
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            synchronized(mutex)
            {
               if (processRunning)
               {
                  process.destroy();
                  if (tcpPortForwarder != null)
                  {
                     tcpPortForwarder.close();
                     tcpPortForwarder = null;
                  }
               }
            }
         }
      });
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#executeInternal(org.eclipse.swt.widgets.Display)
    */
   @Override
   protected void executeInternal(Display display) throws Exception
   {
      synchronized(mutex)
      {
         if (processRunning)
         {
            process.destroy();
            try
            {
               mutex.wait();
            }
            catch(InterruptedException e)
            {
            }
            if (tcpPortForwarder != null)
            {
               tcpPortForwarder.close();
               tcpPortForwarder = null;
            }
         }
         processRunning = true;
      }

      String commandLine;
      if ((tool.getFlags() & ObjectTool.SETUP_TCP_TUNNEL) != 0)
      {
         TcpProxy tcpProxy = session.setupTcpProxy(objectContext.object.getObjectId(), tool.getRemotePort());
         tcpPortForwarder = new TcpPortForwarder(tcpProxy);
         commandLine = command.replace("${local-port}", Integer.toString(tcpPortForwarder.getLocalPort()));
         tcpPortForwarder.run();
      }
      else
      {
         commandLine = command;
      }

      Process process;
      if (Platform.getOS().equals(Platform.OS_WIN32))
      {
         commandLine = "CMD.EXE /C START \"NetXMS\" " + commandLine; //$NON-NLS-1$
         process = Runtime.getRuntime().exec(command);
      }
      else
      {
         process = Runtime.getRuntime().exec(new String[] { "/bin/sh", "-c", commandLine }); //$NON-NLS-1$ //$NON-NLS-2$
      }

      InputStream in = process.getInputStream();
      try
      {
         byte[] data = new byte[16384];
         boolean isWindows = Platform.getOS().equals(Platform.OS_WIN32);
         while(true)
         {
            int bytes = in.read(data);
            if (bytes == -1)
               break;
            String s = new String(Arrays.copyOf(data, bytes));

            // The following is a workaround for issue NX-65
            // Problem is that on Windows XP many system commands
            // (like ping, tracert, etc.) generates output with lines
            // ending in 0x0D 0x0D 0x0A
            if (isWindows)
               out.write(s.replace("\r\r\n", " \r\n")); //$NON-NLS-1$ //$NON-NLS-2$
            else
               out.write(s);
         }

         out.write(Messages.get().LocalCommandResults_Terminated);
      }
      catch(IOException e)
      {
         Activator.logError("Exception while running local command", e); //$NON-NLS-1$
      }
      finally
      {
         in.close();
         synchronized(mutex)
         {
            processRunning = false;
            process = null;
            if (tcpPortForwarder != null)
            {
               tcpPortForwarder.close();
               tcpPortForwarder = null;
            }
            mutex.notifyAll();
         }
      }
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#terminate()
    */
   @Override
   public void terminate()
   {
      synchronized(mutex)
      {
         if (processRunning)
         {
            process.destroy();
         }
      }
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#isTerminateSupported()
    */
   @Override
   protected boolean isTerminateSupported()
   {
      return true;
   }
}
