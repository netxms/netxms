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

import java.io.IOException;
import java.io.OutputStream;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;

/**
 * Generic text console widget
 */
public class TextConsole extends Composite
{
   private Text console;
   private boolean autoScroll = true;
   
   /**
    * @param parent
    * @param style
    */
   public TextConsole(Composite parent, int style)
   {
      super(parent, style);
      setLayout(new FillLayout());
      console = new Text(this, SWT.MULTI | SWT.V_SCROLL);
      console.setEditable(false);
      console.setData(RWT.CUSTOM_VARIANT, "monospace");
   }

   /**
    * Add listener for console selection change
    * 
    * @param listener
    */
   public void addSelectionChangedListener(ISelectionChangedListener listener) 
   {
   }
   
   /**
    * Check if copy operation is allowed
    * 
    * @return
    */
   public boolean canCopy()
   {
      return false;
   }
   
   /**
    * Copy selection to clipboard
    */
   public void copy()
   {
   }
   
   /**
    * Clear console
    */
   public void clear()
   {
      console.setText("");
   }
   
   /**
    * Select all
    */
   public void selectAll()
   {
      console.selectAll();
   }
   
   /**
    * @param autoScroll
    */
   public void setAutoScroll(boolean autoScroll)
   {
      this.autoScroll = autoScroll;
   }
   
   /**
    * Create new output stream attached to underlying console.
    * 
    * @return
    */
   public IOConsoleOutputStream newOutputStream()
   {
      return new IOConsoleOutputStream();
   }
   
   /**
    * Console output stream implementation
    */
   public class IOConsoleOutputStream extends OutputStream
   {
      /* (non-Javadoc)
       * @see java.io.OutputStream#write(int)
       */
      @Override
      public void write(int b) throws IOException
      {
         write(new byte[] {(byte)b}, 0, 1);
      }
      
      /* (non-Javadoc)
       * @see java.io.OutputStream#write(byte[])
       */
      @Override
      public void write(byte[] b) throws IOException 
      {
         write(b, 0, b.length);
      }
      
      /* (non-Javadoc)
       * @see java.io.OutputStream#write(byte[], int, int)
       */
      @Override
      public void write(byte[] b, int off, int len) throws IOException 
      {
         write(new String(b, off, len));
      }
      
      /**
       * Write string to console
       * 
       * @param s
       * @throws IOException
       */
      public synchronized void write(final String s) throws IOException 
      {
         getDisplay().asyncExec(new Runnable() {
            @Override
            public void run()
            {
               console.append(s);
            }
         });
      }
   }
}
