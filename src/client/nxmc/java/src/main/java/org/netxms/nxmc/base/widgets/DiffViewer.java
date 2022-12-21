/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

import java.util.LinkedList;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.DiffMatchPatch.Diff;
import org.netxms.nxmc.base.widgets.helpers.StyleRange;

/**
 * Generic diff viewer
 */
public class DiffViewer extends Composite
{
   private StyledText textControl;
   private Color colorDeleted;
   private Color colorInserted;

   /**
    * @param parent
    * @param style
    */
   public DiffViewer(Composite parent, int style)
   {
      super(parent, style);
      setLayout(new FillLayout());
      textControl = new StyledText(this, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY);
      textControl.setFont(JFaceResources.getTextFont());
      colorDeleted = new Color(getDisplay(), 0xFD, 0xB8, 0xC0);
      colorInserted = new Color(getDisplay(), 0xAC, 0xF2, 0xBD);
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            colorDeleted.dispose();
            colorInserted.dispose();
         }
      });
   }

   /**
    * Set content for diff viewer
    * 
    * @param diffs set of diffs to display
    */
   public void setContent(LinkedList<Diff> diffs)
   {
      textControl.setText("");
      for(Diff diff : diffs)
      {
         StyleRange style = new StyleRange();
         style.start = textControl.getCharCount();
         style.length = diff.text.length();
         switch(diff.operation)
         {
            case DELETE:
               style.background = colorDeleted;
               break;
            case EQUAL:
               break; // Use default style for unchanged lines
            case INSERT:
               style.background = colorInserted;
               break;
         }
         textControl.append(diff.text);
         textControl.setStyleRange(style);
      }
   }
}
