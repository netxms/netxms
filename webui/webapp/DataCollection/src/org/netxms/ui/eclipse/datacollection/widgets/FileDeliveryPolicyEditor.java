/**
 * NetXMS - open source network management system
 * Copyright (C) 2019 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.widgets;

import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AgentPolicy;

/**
 * Editor for file delivery policy
 */
public class FileDeliveryPolicyEditor extends AbstractPolicyEditor
{
   private TreeViewer fileTree;

   /**
    * @param parent
    * @param style
    */
   public FileDeliveryPolicyEditor(Composite parent, int style, AgentPolicy policy)
   {
      super(parent, style, policy);
      
      setLayout(new FillLayout());
      
      fileTree = new TreeViewer(this, SWT.NONE);
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#updateControlFromPolicy()
    */
   @Override
   protected void updateControlFromPolicy()
   {
      // TODO Auto-generated method stub

   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#updatePolicyFromControl()
    */
   @Override
   public AgentPolicy updatePolicyFromControl()
   {
      // TODO Auto-generated method stub
      return null;
   }
}
