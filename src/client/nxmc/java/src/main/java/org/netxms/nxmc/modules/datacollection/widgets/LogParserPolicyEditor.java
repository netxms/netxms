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

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.AgentPolicy;
import org.netxms.nxmc.modules.datacollection.views.PolicyEditorView;
import org.netxms.nxmc.modules.logwatch.widgets.LogParserEditor;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserModifyListener;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserType;

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
   public LogParserPolicyEditor(Composite parent, int style, AgentPolicy policy, PolicyEditorView view)
   {
      super(parent, style, policy, view);      

      setLayout(new FillLayout());

      editor = new LogParserEditor(this, SWT.NONE, LogParserType.POLICY);
      editor.addModifyListener(new LogParserModifyListener() {
         @Override
         public void modifyParser()
         {
            fireModifyListeners();
         }
      });

      updateControlFromPolicy();
   }
   
   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.AbstractPolicyEditor#updateControlFromPolicy()
    */
   @Override
   public void updateControlFromPolicy()
   {
      editor.setParserXml(policy.getContent());
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.AbstractPolicyEditor#updatePolicyFromControl()
    */
   @Override
   public void updatePolicyFromControl()
   {
      policy.setContent(editor.getParserXml());
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#setFocus()
    */
   @Override
   public boolean setFocus()
   {
      return editor.setFocus();      
   }
}
