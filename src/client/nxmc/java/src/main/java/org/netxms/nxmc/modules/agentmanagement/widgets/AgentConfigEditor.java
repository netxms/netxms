/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.agentmanagement.widgets;

import java.util.Collection;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.base.widgets.CompositeWithMessageArea;

/**
 * NXSL script editor
 */
public class AgentConfigEditor extends CompositeWithMessageArea
{
   private Composite content;
   private Text editor;
   
   /**
    * @param parent
    * @param style
    * @param editorStyle
    * @param showLineNumbers
    * @param hints
    * @param showCompileButton
    */
   public AgentConfigEditor(Composite parent, int style, int editorStyle)
   {
      super(parent, style);

      content = new Composite(this, SWT.NONE);
      setContent(content);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      content.setLayout(layout);
      
      editor = new Text(content, editorStyle | SWT.MULTI);
      //editor.setData(RWT.CUSTOM_VARIANT, "monospace");
      editor.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
   }

   /**
    * Get underlying text widget
    * @return text widget
    */
   public Text getTextWidget()
   {
      return editor;
   }
   
   /**
    * Set text for editing
    * @param text
    */
   public void setText(String text)
   {
      editor.setText(text);
   }

   /**
    * Get editor's content
    * @return
    */
   public String getText()
   {
      return editor.getText();
   }

   /**
    * @param functions the functions to set
    */
   public void setFunctions(Set<String> functions)
   {
   }
   
   /**
    * Add functions
    * 
    * @param fc
    */
   public void addFunctions(Collection<String> fc)
   {
   }

   /**
    * @param variables the variables to set
    */
   public void setVariables(Set<String> variables)
   {
   }

   /**
    * Add variables
    * 
    * @param vc
    */
   public void addVariables(Collection<String> vc)
   {
   }

   /**
    * @param constants new constant set
    */
   public void setConstants(Set<String> constants)
   {
   }

   /**
    * Add constants
    * 
    * @param cc constants to add
    */
   public void addConstants(Collection<String> cc)
   {
   }

   /**
    * @return the functionsCache
    */
   public String[] getFunctions()
   {
      return new String[0];
   }

   /**
    * @return the variablesCache
    */
   public String[] getVariables()
   {
      return new String[0];
   }

   /**
    * @return constants cache
    */
   public String[] getConstants()
   {
      return new String[0];
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      Point p = editor.computeSize(wHint, hHint, changed);
      p.y += 4;
      return p;
   }

   /**
    * Show/hide line numbers in editor
    * 
    * @param show
    */
   public void showLineNumbers(boolean show)
   {
   }
}
