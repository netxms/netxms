/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console;

import org.eclipse.jface.action.ContributionItem;
import org.eclipse.jface.action.StatusLineLayoutData;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.FontMetrics;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.LicenseProblem;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Status line contribution item to display server name
 */
public class LicenseProblemStatusLineItem extends ContributionItem
{
   private final static int DEFAULT_CHAR_WIDTH = 40;

   /**
    * A constant indicating that the contribution should compute its actual size depending on the text. It will grab all space
    * necessary to display the whole text.
    * 
    * @since 3.6
    */
   public final static int CALC_TRUE_WIDTH = -1;

   private int charWidth;
   private int widthHint = -1;
   private int heightHint = -1;

   private CLabel label;
   private Font font;

   /**
    * The composite into which this contribution item has been placed. This will be <code>null</code> if this instance has not yet
    * been initialized.
    */
   private Composite statusLine = null;

   /**
    * Creates a status line contribution item with the given id.
    * 
    * @param id the contribution item's id, or <code>null</code> if it is to have no id
    */
   public LicenseProblemStatusLineItem(String id)
   {
      this(id, DEFAULT_CHAR_WIDTH);
   }

   /**
    * Creates a status line contribution item with the given id that displays the given number of characters.
    * 
    * @param id the contribution item's id, or <code>null</code> if it is to have no id
    * @param charWidth the number of characters to display. If the value is CALC_TRUE_WIDTH then the contribution will compute the
    *           preferred size exactly. Otherwise the size will be based on the average character size * 'charWidth'
    */
   public LicenseProblemStatusLineItem(String id, int charWidth)
   {
      super(id);
      this.charWidth = charWidth;
   }

   /**
    * Fill control
    */
   public void fill(Composite parent)
   {
      statusLine = parent;
      
      FontData fd = JFaceResources.getDialogFont().getFontData()[0];
      fd.setStyle(SWT.BOLD);
      font = new Font(Display.getCurrent(), fd);

      Label sep = new Label(parent, SWT.SEPARATOR);
      label = new CLabel(statusLine, SWT.BORDER | SWT.SHADOW_ETCHED_IN | SWT.CENTER);
      label.setFont(font);
      label.setText("LICENSE PROBLEM");
      label.setBackground(parent.getDisplay().getSystemColor(SWT.COLOR_YELLOW));
      
      label.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            font.dispose();
         }
      });

      if (charWidth == CALC_TRUE_WIDTH)
      {
         // compute the size of the label to get the width hint for the contribution
         Point preferredSize = label.computeSize(SWT.DEFAULT, SWT.DEFAULT);
         widthHint = preferredSize.x;
         heightHint = preferredSize.y;
      }
      else if (widthHint < 0)
      {
         // Compute the size base on 'charWidth' average char widths
         GC gc = new GC(statusLine);
         gc.setFont(statusLine.getFont());
         FontMetrics fm = gc.getFontMetrics();
         widthHint = (int)(fm.getAverageCharacterWidth() * charWidth);
         heightHint = fm.getHeight();
         gc.dispose();
      }

      StatusLineLayoutData data = new StatusLineLayoutData();
      data.widthHint = widthHint;
      label.setLayoutData(data);

      data = new StatusLineLayoutData();
      data.heightHint = heightHint;
      sep.setLayoutData(data);

      label.addMouseListener(new MouseListener() {
         @Override
         public void mouseUp(MouseEvent e)
         {
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            displayProblems();
         }
      });
   }

   /**
    * An accessor for the current location of this status line contribution item -- relative to the display.
    * 
    * @return The current location of this status line; <code>null</code> if not yet initialized.
    */
   public Point getDisplayLocation()
   {
      if ((label != null) && (statusLine != null))
      {
         return statusLine.toDisplay(label.getLocation());
      }
      return null;
   }

   /**
    * Display pending problems
    */
   private void displayProblems()
   {
      LicenseProblem[] licenseProblems = ConsoleSharedData.getSession().getLicenseProblems();
      if ((licenseProblems != null) && (licenseProblems.length > 0))
      {
         StringBuilder sb = new StringBuilder();
         for(LicenseProblem p : licenseProblems)
         {
            if (sb.length() == 0)
               sb.append("\r\n");
            sb.append(p.getDescription());
         }
         MessageDialog.openWarning(Display.getCurrent().getActiveShell(), "License Problem", sb.toString());
      }
   }
}
