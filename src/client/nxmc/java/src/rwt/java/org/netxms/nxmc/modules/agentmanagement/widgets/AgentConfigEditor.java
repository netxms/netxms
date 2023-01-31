/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;

/**
 * Agent configuration file editor
 */
public class AgentConfigEditor extends Composite
{
   private Text editor;

   /**
    * @param parent
    * @param style
    * @param editorStyle
    */
   public AgentConfigEditor(Composite parent, int style, int editorStyle)
   {
      super(parent, style);
      
      setLayout(new FillLayout());
      editor = new Text(this, editorStyle | SWT.MULTI);
      editor.setData(RWT.CUSTOM_VARIANT, "monospace");
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
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      Point p = editor.computeSize(wHint, hHint, changed);
      p.y += 4;
      return p;
   }
}
