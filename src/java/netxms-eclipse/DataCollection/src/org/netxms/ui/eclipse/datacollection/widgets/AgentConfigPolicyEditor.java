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
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.widgets.AgentConfigEditor;

/**
 * Generic policy editor widget
 */
public class AgentConfigPolicyEditor extends AbstractPolicyEditor
{
   private AgentConfigEditor editor;

   /**
    * Constructor
    * 
    * @param parent
    * @param style
    */
   public AgentConfigPolicyEditor(Composite parent, int style, AgentPolicy policy)
   {
      super(parent, style);      
      this.policyObject = policy;  

      setLayout(new FillLayout());
      
      editor = new AgentConfigEditor(this, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL);
      editor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            fireModifyListeners();
            actionFindReplace.update();
         }
      }); 
      
      updateControlsFromPolicy();
   }
   
   protected void updateControlsFromPolicy()
   {
      editor.setText(policyObject.getContent());
   }

   public AgentPolicy getUpdatedPolicy()
   {
      policyObject.setContent(editor.getText());
      return policyObject;
   }   


   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public boolean setFocus()
   {
      return editor.setFocus();      
   }

   @Override
   public boolean canPerformFind()
   {
      return editor.getFindReplaceTarget().canPerformFind();
   }

   @Override
   public int findAndSelect(int widgetOffset, String findString, boolean searchForward, boolean caseSensitive, boolean wholeWord)
   {
      return editor.getFindReplaceTarget().findAndSelect(widgetOffset, findString, searchForward, caseSensitive, wholeWord);
   }

   @Override
   public Point getSelection()
   {
      return editor.getFindReplaceTarget().getSelection();
   }

   @Override
   public String getSelectionText()
   {
      return editor.getFindReplaceTarget().getSelectionText();
   }

   @Override
   public boolean isEditable()
   {
      return true;
   }

   @Override
   public void replaceSelection(String text)
   {
      editor.getFindReplaceTarget().replaceSelection(text);
   }

   @Override
   public boolean isFindReplaceRequired()
   {
      return true;
   }
}
