/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.io.IOException;
import org.apache.commons.io.IOUtils;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * PDF viewer
 */
public class PDFViewer extends Composite
{
   private static final Logger logger = LoggerFactory.getLogger(PDFViewer.class);

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
      if (browser == null)
         return;

      try
      {
         String html = IOUtils.resourceToString("pdfview.html", null, getClass().getClassLoader());
         browser.setText(html.replace("{{url}}", url));
      }
      catch(IOException e)
      {
         logger.error("Cannot load pdfview.html from resources", e);
      }
   }
}
