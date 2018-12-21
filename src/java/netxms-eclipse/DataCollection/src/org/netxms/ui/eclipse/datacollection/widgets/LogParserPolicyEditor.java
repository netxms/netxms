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

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.LogParserModifyListener;

/**
 * Generic policy editor widget
 */
public class LogParserPolicyEditor extends AbstractPolicyEditor
{
   private LogParserEditor editor; 

   /**
    * Constructor
    * 
    * @param parent
    * @param style
    */
   public LogParserPolicyEditor(Composite parent, int style, AgentPolicy policy)
   {
      super(parent, style);      
      this.policy = policy;  
      
      setLayout(new FillLayout());
      
      editor = new LogParserEditor(this, SWT.NONE, false);
      editor.addModifyListener(new LogParserModifyListener() {
         @Override
         public void modifyParser()
         {
            fireModifyListeners();
         }
      });
      
      refresh();
   }
   
   protected void refresh()
   {
      editor.setParserXml(policy.getContent());
   }

   public AgentPolicy getUpdatedPolicy()
   {
      policy.setContent(editor.getParserXml());
      return policy;
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public boolean setFocus()
   {
      return editor.setFocus();      
   }
}
