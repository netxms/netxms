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

import java.util.List;
import org.commonmark.Extension;
import org.commonmark.ext.autolink.AutolinkExtension;
import org.commonmark.ext.footnotes.FootnotesExtension;
import org.commonmark.ext.gfm.strikethrough.StrikethroughExtension;
import org.commonmark.ext.gfm.tables.TablesExtension;
import org.commonmark.ext.image.attributes.ImageAttributesExtension;
import org.commonmark.ext.ins.InsExtension;
import org.commonmark.ext.task.list.items.TaskListItemsExtension;
import org.commonmark.node.Node;
import org.commonmark.parser.Parser;
import org.commonmark.renderer.html.HtmlRenderer;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.LocationEvent;
import org.eclipse.swt.browser.LocationListener;
import org.eclipse.swt.browser.ProgressEvent;
import org.eclipse.swt.browser.ProgressListener;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.tools.ExternalWebBrowser;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import fr.brouillard.oss.commonmark.ext.notifications.NotificationsExtension;

/**
 * Widget to display markdown-formatted text
 */
public class MarkdownViewer extends Composite
{
   private static final Logger logger = LoggerFactory.getLogger(MarkdownViewer.class);
   private static final List<Extension> extensions =
         List.of(AutolinkExtension.create(), FootnotesExtension.builder().inlineFootnotes(true).build(), ImageAttributesExtension.create(), InsExtension.create(),
               NotificationsExtension.create().withClassMapper((n) -> "notification notification-" + n.name().toLowerCase()),
               TaskListItemsExtension.create(), StrikethroughExtension.create(), TablesExtension.create());

   private Browser browser;
   private Text textWidget; // Used if browser cannot be embedded
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
      if (browser != null)
      {
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
         browser.addLocationListener(new LocationListener() {
            @Override
            public void changing(LocationEvent event)
            {
               String url = event.location;
               if (!url.startsWith("about:") && !url.startsWith("chrome:") && !url.startsWith("edge:"))
               {
                  ExternalWebBrowser.open(event.location);
                  event.doit = false;
               }
            }
   
            @Override
            public void changed(LocationEvent event)
            {
            }
         });
         FontData f = JFaceResources.getDefaultFont().getFontData()[0];
         htmlHeader = "<html><head><style>body { font-family: " + f.getName() +
               "; font-size: " + f.getHeight() + (Registry.IS_WEB_CLIENT ? "px" : "pt") +
               "; font-weight: normal; margin: 0; padding: 0; }" +
               ".notification { padding: 9.5px; margin-top: 0.5rem; margin-bottom: 1rem; " +
               "border: 1px solid transparent; border-radius: 4px; font-size: 120%; font-weight: 500; } " +
               ".notification p { padding: 0; margin: 0; } " +
               ".notification-info { color: #07525e; background-color: #d1ecf1; border-color: #bee5eb; } " +
               ".notification-success { color: #125522; background-color: #d4edda; border-color: #c3e6cb; } " +
               ".notification-warning { color: #6a4d00; background-color: #fff3cd; border-color: #ffeeba; } " +
               ".notification-error { color: #900000; background: #ffd2d2; border-color: #f1a899; } " +
               "img { max-width: 100%; }" +
               "p code { padding: 2px 4px; font-size: 90%; border-radius: 4px; color: #333; background-color: #f5f5f5; }" +
               "pre code { display: block; padding: 9.5px; color: #333; word-break: break-all; word-wrap: break-word; background-color: #f5f5f5; border: 1px solid #ccc; border-radius: 4px; white-space: pre-wrap; }" +
               "</style></head><body><div class=\"content\" style=\"padding-left: 5px; padding-right: 5px;\">";
      }
      else
      {
         textWidget = new Text(this, SWT.MULTI | SWT.READ_ONLY | SWT.H_SCROLL | SWT.V_SCROLL);
      }
   }

   /**
    * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      if ((hHint != SWT.DEFAULT) || (browser == null))
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
      if (this.text.equals((text != null) ? text : ""))
         return; // no changes

      this.text = (text != null) ? text : "";
      if (browser != null)
      {
         htmlDocumentHeight = 0;
         String html;
         try
         {
            Parser parser = Parser.builder().extensions(extensions).build();
            Node document = parser.parse(this.text);
            HtmlRenderer renderer = HtmlRenderer.builder().extensions(extensions).build();
            html = htmlHeader + renderer.render(document) + "</div></body></html>";
         }
         catch(Exception e)
         {
            logger.error("Exception in Markdown renderer", e);
            html = htmlHeader + "<div class=\"notification notification-error\">Markdown rendering error</div><div>Original document source:</div><code>" + this.text.replace("\n", "<br/>") +
                  "</code></div></body></html>";
         }
         browser.setText(html);
      }
      else
      {
         textWidget.setText(this.text);
      }
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
