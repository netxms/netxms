/**
 * NetXMS - open source network management system
 * Copyright (C) 2021-2023 Raden Solutions
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
package org.netxms.ui.eclipse.objectmanager.actions;

import java.util.List;
import org.eclipse.core.commands.AbstractHandler;
import org.eclipse.core.commands.ExecutionEvent;
import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.handlers.HandlerUtil;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.dialogs.CreateSubnetDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Create subnet manually
 */
public class CreateSubnet extends AbstractHandler
{
   @Override
   public Object execute(ExecutionEvent event) throws ExecutionException
   {
      IWorkbenchWindow window = HandlerUtil.getActiveWorkbenchWindow(event);
      ISelection selection = window.getActivePage().getSelection();
      if ((selection == null) || !(selection instanceof IStructuredSelection) || selection.isEmpty())
         return null;

      long parentId = -1;
      int zoneUin = 0;
      Object obj = ((IStructuredSelection)selection).getFirstElement();
      final NXCSession session = ConsoleSharedData.getSession();
      if (session.isZoningEnabled() && (obj instanceof Zone))
      {
         parentId = ((AbstractObject)obj).getObjectId();
         zoneUin = ((Zone)obj).getUIN();
      }      
      if (!session.isZoningEnabled() && (obj instanceof EntireNetwork))
      {
         parentId = ((AbstractObject)obj).getObjectId();
      }

      CreateSubnetDialog dlg = new CreateSubnetDialog(window.getShell());
      if (dlg.open() != Window.OK)
         return null;

      final NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_SUBNET, dlg.getObjectName(), parentId);
      cd.setObjectAlias(dlg.getObjectAlias());
      cd.setIpAddress(dlg.getIpAddress());
      cd.setZoneUIN(zoneUin);
      
      new ConsoleJob("Create subnet", null, Activator.PLUGIN_ID, null) {
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            try
            {
               session.createObject(cd);
            }
            catch (NXCException e)
            {
               if (e.getErrorCode() == RCC.SUBNET_OVERLAP)
               {
                  List<AbstractObject> subnets = session.findMultipleObjects(e.getRelatedObjects(), false);
                  StringBuilder sb = new StringBuilder();
                  for (int i = 0; i < subnets.size(); i++)
                  {
                     sb.append(subnets.get(i).getObjectName());
                     if ((i + 1) != subnets.size())
                     sb.append(", ");
                  }
                  runInUIThread(new Runnable() {
                     
                     @Override
                     public void run()
                     {
                        MessageDialogHelper.openError(window.getShell(), getErrorMessage(), "Subnet was not created because it overlaps with other subnets: " + sb.toString());
                     }
                  });                  
               }
               else
                  throw e;
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            
            return "Error creating subnet";
         }
      }.start();
      
      return null;
   }

}
