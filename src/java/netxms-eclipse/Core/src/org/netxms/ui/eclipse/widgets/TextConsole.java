/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.widgets;

import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.console.IOConsole;
import org.eclipse.ui.console.IOConsoleOutputStream;
import org.eclipse.ui.console.TextConsoleViewer;
import org.eclipse.ui.internal.console.IOConsoleViewer;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.widgets.ansi.AnsiIOConsoleViewer;

/**
 * Generic text console widget
 */
@SuppressWarnings("restriction")
public class TextConsole extends Composite
{
   protected IOConsoleViewer viewer;
   protected IOConsole console;

   /**
    * @param parent
    * @param style
    */
   public TextConsole(Composite parent, int style)
   {
      super(parent, style);
      setLayout(new FillLayout());
      console = new IOConsole("Console", Activator.getImageDescriptor("icons/console.png")); //$NON-NLS-1$ //$NON-NLS-2$
      viewer = new AnsiIOConsoleViewer(this, console);
      viewer.setEditable(false);
   }

   /**
    * Add listener for console selection change
    * 
    * @param listener
    */
   public void addSelectionChangedListener(ISelectionChangedListener listener) 
   {
      viewer.addSelectionChangedListener(listener);
   }
   
   /**
    * Check if copy operation is allowed
    * 
    * @return
    */
   public boolean canCopy()
   {
      return viewer.canDoOperation(TextConsoleViewer.COPY);
   }
   
   /**
    * Copy selection to clipboard
    */
   public void copy()
   {
      if (viewer.canDoOperation(TextConsoleViewer.COPY))
         viewer.doOperation(TextConsoleViewer.COPY);
   }
   
   /**
    * Clear console
    */
   public void clear()
   {
      console.clearConsole();
   }
   
   /**
    * Select all
    */
   public void selectAll()
   {
      if (viewer.canDoOperation(TextConsoleViewer.SELECT_ALL))
         viewer.doOperation(TextConsoleViewer.SELECT_ALL);
   }
   
   /**
    * @param autoScroll
    */
   public void setAutoScroll(boolean autoScroll)
   {
      viewer.setAutoScroll(autoScroll);
   }
   
   /**
    * Create new output stream attached to underlying console.
    * 
    * @return
    */
   public IOConsoleOutputStream newOutputStream()
   {
      return console.newOutputStream();
   }
}
