/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * PDF viewer
 */
public class PDFViewer extends Composite
{
   private Browser browser;

   /**
    * @param parent
    * @param style
    */
   public PDFViewer(Composite parent, int style)
   {
      super(parent, style);
      setLayout(new FillLayout());
      browser = WidgetHelper.createBrowser(this, SWT.NONE, null);
   }

   /**
    * Open document file.
    *
    * @param fileName document file name
    */
   public void openFile(String fileName)
   {
      openUrl("file://" + fileName);
   }

   /**
    * Open document by URL.
    *
    * @param url document URL
    */
   public void openUrl(String url)
   {
      browser.setUrl(url);
   }
}
