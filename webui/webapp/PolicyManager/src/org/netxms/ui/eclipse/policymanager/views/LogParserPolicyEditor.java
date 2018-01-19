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
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.objects.AgentPolicyLogParser;
import org.netxms.ui.eclipse.serverconfig.widgets.LogParserEditor;
import org.netxms.ui.eclipse.serverconfig.widgets.helpers.LogParserModifyListener;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Editor for log parser policy
 */
public class LogParserPolicyEditor extends AbstractPolicyEditor
{
   public static final String ID = "org.netxms.ui.eclipse.policymanager.views.LogParserPolicyEditor";
   
   private LogParserEditor editor;
   private String content;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      super.createPartControl(parent);
      
      editor = new LogParserEditor(parent, SWT.NONE, false);
      editor.addModifyListener(new LogParserModifyListener() {
         @Override
         public void modifyParser()
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
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public void doSave(IProgressMonitor monitor)
   {
      editor.getDisplay().syncExec(new Runnable() {
         @Override
         public void run()
         {
            content = editor.getParserXml();
         }
      });
      try
      {
         NXCObjectModificationData md = new NXCObjectModificationData(policy.getObjectId());
         md.setConfigFileContent(content);
         session.modifyObject(md);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getSite().getShell(), "Error saving log parser", 
               String.format("Error saving log parser: %s", e.getLocalizedMessage()));
      }
   }

   /**
    * Refresh viewer
    */
   protected void doRefresh()
   {
      editor.setParserXml(((AgentPolicyLogParser)policy).getFileContent());    
   }
}
