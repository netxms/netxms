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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewPart;
import org.netxms.client.AgentPolicy;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.AgentConfigEditor;

/**
 * Editor for agent configuration policy
 */
public class AgentConfigPolicyEditor extends AbstractPolicyEditor
{
   private AgentConfigEditor editor;
   private Button buttonExpandMacro;
   
   private int flags; 

   /**
    * Constructor
    * 
    * @param parent
    * @param style
    */
   public AgentConfigPolicyEditor(Composite parent, int style, AgentPolicy policy, IViewPart viewPart)
   {
      super(parent, style, policy, viewPart);  

      flags = policy.getFlags();
      setLayout(new FillLayout());
      
      Composite mainArea = new Composite(this, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      mainArea.setLayout(layout);
      
      buttonExpandMacro = new Button(mainArea, SWT.CHECK);
      buttonExpandMacro.setText("Expand macro");
      buttonExpandMacro.setLayoutData(new GridData());
      buttonExpandMacro.addSelectionListener(new SelectionListener() {
         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            fireModifyListeners();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
      editor = new AgentConfigEditor(mainArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL);
      editor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            fireModifyListeners();
         }
      }); 
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      editor.setLayoutData(gd);
      
      updateControlFromPolicy();
   }
   
   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#updateControlFromPolicy()
    */
   @Override
   public void updateControlFromPolicy()
   {
      editor.setText(getPolicy().getContent());
      flags = getPolicy().getFlags();
      buttonExpandMacro.setSelection((flags & AgentPolicy.EXPAND_MACRO) > 0);
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#updatePolicyFromControl()
    */
   @Override
   public AgentPolicy updatePolicyFromControl()
   {
      getPolicy().setContent(editor.getText());
      if(buttonExpandMacro.getSelection())
      {
         flags |= AgentPolicy.EXPAND_MACRO;
      }
      else
      {
         flags &= ~AgentPolicy.EXPAND_MACRO;         
      }
      getPolicy().setFlags(flags);
      return getPolicy();
   }   

   /**
    * @see org.eclipse.swt.widgets.Composite#setFocus()
    */
   @Override
   public boolean setFocus()
   {
      return editor.setFocus();      
   }
   
   /* toto add find and replace in the view
   protected void createActions()
   {
      actionFindReplace = NXFindAndReplaceAction.getFindReplaceAction(this.getParent());
   }

   protected void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionFindReplace);
   }
   */
}
