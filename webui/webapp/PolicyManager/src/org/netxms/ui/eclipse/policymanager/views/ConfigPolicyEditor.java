/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2018 Raden Solutions
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
package org.netxms.ui.eclipse.policymanager.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.objects.AgentPolicyConfig;
import org.netxms.ui.eclipse.agentmanager.widgets.AgentConfigEditor;

public class ConfigPolicyEditor extends AbstractPolicyEditor
{
   public static final String ID = "org.netxms.ui.eclipse.policymanager.views.ConfigPolicyEditor";
   
   private AgentConfigEditor editor;
   private String content;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      super.createPartControl(parent);
      
      parent.setLayout(new FillLayout());
      
      editor = new AgentConfigEditor(parent, SWT.NONE, SWT.H_SCROLL | SWT.V_SCROLL);
      editor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            setModified();
         }
      });
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      editor.setFocus();      
   }
  
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.policymanager.views.AbstractPolicyEditor#createActions()
    */
   @Override
   protected void createActions()
   {
      super.createActions();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.policymanager.views.AbstractPolicyEditor#fillLocalPullDown(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalPullDown(IMenuManager manager)
   {
      super.fillLocalPullDown(manager);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.policymanager.views.AbstractPolicyEditor#doSaveInternal(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public void doSaveInternal(IProgressMonitor monitor) throws Exception
   {
      editor.getDisplay().syncExec(new Runnable() {
         @Override
         public void run()
         {
            content = editor.getText();
         }
      });
      NXCObjectModificationData md = new NXCObjectModificationData(policy.getObjectId());
      md.setConfigFileContent(content);
      session.modifyObject(md);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.policymanager.views.AbstractPolicyEditor#doRefresh()
    */
   @Override
   protected void doRefresh()
   {
      editor.setText(((AgentPolicyConfig)policy).getFileContent());
   }
}
