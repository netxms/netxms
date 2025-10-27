/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Composite;

/**
 * Spacer composite
 */
public class Spacer extends Composite
{
   private int width;

   /**
    * Create spacer with given width.
    *
    * @param parent parent composite
    * @param width spacer width
    */
   public Spacer(Composite parent, int width)
   {
      this(parent, width, new GridData(SWT.CENTER, SWT.FILL, false, true));
   }

   /**
    * Create spacer with given width and layout data.
    *
    * @param parent parent composite
    * @param width spacer width
    * @param layoutData layout data
    */
   public Spacer(Composite parent, int width, Object layoutData)
   {
      super(parent, SWT.NONE);
      this.width = width;
      setBackground(parent.getBackground());
      setLayoutData(layoutData);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      return new Point(width, (hHint == SWT.DEFAULT) ? 20 : hHint);
   }
}