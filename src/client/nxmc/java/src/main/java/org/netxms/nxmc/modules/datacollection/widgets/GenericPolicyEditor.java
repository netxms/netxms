/**
 * NetXMS - open source network management system
 * Copyright (C) 2019-2023 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.widgets;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.AgentPolicy;
import org.netxms.nxmc.base.widgets.StyledText;
import org.netxms.nxmc.modules.datacollection.views.PolicyEditorView;

/**
 * Generic policy editor widget
 */
public class GenericPolicyEditor extends AbstractPolicyEditor
{
   private StyledText editor;

   /**
    * Constructor
    * 
    * @param parent
    * @param style
    */
   public GenericPolicyEditor(Composite parent, int style, AgentPolicy policy, PolicyEditorView view)
   {
      super(parent, style, policy, view);      

      setLayout(new FillLayout());

      editor = new StyledText(this, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      editor.setFont(JFaceResources.getTextFont());
      
      editor.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            fireModifyListeners();
         }
      });   
      editor.setText(policy.getContent());
      
      updateControlFromPolicy();
   }
   
   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.AbstractPolicyEditor#updateControlFromPolicy()
    */
   @Override
   public void updateControlFromPolicy()
   {
      editor.setText(policy.getContent());
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.AbstractPolicyEditor#updatePolicyFromControl()
    */
   @Override
   public void updatePolicyFromControl()
   {
      policy.setContent(editor.getText());
   }
}
