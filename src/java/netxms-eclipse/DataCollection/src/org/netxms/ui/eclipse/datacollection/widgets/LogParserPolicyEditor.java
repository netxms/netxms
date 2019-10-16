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
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.texteditor.FindReplaceAction;
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
      this.policyObject = policy;  
      
      setLayout(new FillLayout());
      
      editor = new LogParserEditor(this, SWT.NONE, false);
      editor.addModifyListener(new LogParserModifyListener() {
         @Override
         public void modifyParser()
         {
            fireModifyListeners();
         }
      });
      
      updateControlsFromPolicy();
   }
   
   protected void updateControlsFromPolicy()
   {
      editor.setParserXml(policyObject.getContent());
   }

   public AgentPolicy getUpdatedPolicy()
   {
      policyObject.setContent(editor.getParserXml());
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
   public boolean isFindReplaceRequired()
   {
      return editor.isEditorTabSelected(); 
   }

   public void setFindAndReplaceAction(FindReplaceAction actionFindReplace)
   {
      editor.setFindAndReplaceAction(actionFindReplace);
   }

   @Override
   public boolean canPerformFind()
   {
      return editor.isEditorTabSelected();
   }

   @Override
   public int findAndSelect(int widgetOffset, String findString, boolean searchForward, boolean caseSensitive, boolean wholeWord)
   {
      return editor.findAndSelect(widgetOffset, findString, searchForward, caseSensitive, wholeWord);
   }

   @Override
   public Point getSelection()
   {
      return editor.getSelection();
   }

   @Override
   public String getSelectionText()
   {
      return editor.getSelectionText();
   }

   @Override
   public boolean isEditable()
   {
      return editor.isEditable();
   }

   @Override
   public void replaceSelection(String text)
   {
      editor.replaceSelection(text);
   }
}
