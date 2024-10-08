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

import org.commonmark.node.Node;
import org.commonmark.parser.Parser;
import org.commonmark.renderer.html.HtmlRenderer;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.ProgressEvent;
import org.eclipse.swt.browser.ProgressListener;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Widget to display markdown-formatted text
 */
public class MarkdownViewer extends Composite
{
   private Browser browser;
   private String text = "";
   private int htmlDocumentHeight = 0;
   private Runnable renderCompletionHandler = null;
   private String htmlHeader;

   /**
    * Create new markdown viewer.
    *
    * @param parent parent composite
    * @param style widget style
    */
   public MarkdownViewer(Composite parent, int style)
   {
      super(parent, style);
      setLayout(new FillLayout());
      browser = WidgetHelper.createBrowser(this, SWT.NONE, null);
      browser.addProgressListener(new ProgressListener() {
         @Override
         public void completed(ProgressEvent event)
         {
            Double height = (Double)browser.evaluate("return document.getElementsByClassName('content')[0].offsetHeight;");
            htmlDocumentHeight = (int)Math.ceil(height);
            if (renderCompletionHandler != null)
               renderCompletionHandler.run();
         }

         @Override
         public void changed(ProgressEvent event)
         {
         }
      });
      FontData f = JFaceResources.getDefaultFont().getFontData()[0];
      htmlHeader = "<html><head><style>body { font-family: " + f.getName() +
            "; font-size: " + f.getHeight() + (Registry.IS_WEB_CLIENT ? "px" : "pt") +
            "; font-weight: normal; margin: 0; padding: 0; }</style></head><body><div class=\"content\" style=\"padding-left: 5px; padding-right: 5px;\">";
   }

   /**
    * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      if (hHint != SWT.DEFAULT)
         return super.computeSize(wHint, hHint, changed);
      return new Point(wHint == SWT.DEFAULT ? 100 : wHint, htmlDocumentHeight);
   }

   /**
    * Set text in markdown format.
    *
    * @param text text to display
    */
   public void setText(String text)
   {
      htmlDocumentHeight = 0;
      this.text = (text != null) ? text : "";
      Parser parser = Parser.builder().build();
      Node document = parser.parse(this.text);
      HtmlRenderer renderer = HtmlRenderer.builder().build();
      String html = htmlHeader + renderer.render(document) + "</div></body></html>";
      browser.setText(html);
   }

   /**
    * Get text previously set by <code>setText</code>.
    *
    * @return text set for rendering
    */
   public String getText()
   {
      return text;
   }

   /**
    * Get current render completion handler.
    *
    * @return current render completion handler
    */
   public Runnable getRenderCompletionHandler()
   {
      return renderCompletionHandler;
   }

   /**
    * Set render completion handler. Called by control when document rendering is complete. Can be used to resize control and/or
    * it's parents to fully accomodate rendered content.
    *
    * @param renderCompletionHandler new render completion handler or null
    */
   public void setRenderCompletionHandler(Runnable renderCompletionHandler)
   {
      this.renderCompletionHandler = renderCompletionHandler;
   }
}
