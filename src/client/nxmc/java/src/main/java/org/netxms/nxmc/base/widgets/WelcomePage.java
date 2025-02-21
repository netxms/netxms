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

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.LocationEvent;
import org.eclipse.swt.browser.LocationListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.base.VersionInfo;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ExternalWebBrowser;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Welcome page
 */
public class WelcomePage extends Composite
{
   private static final String ERROR_PAGE = 
         "<html>"+
         "   <body>" +
         "      <div style=\"padding: 10px; display: flex; flex-wrap: nowrap; align-items: center;\">" +
         "         <img src=\"data:image/png;base64,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\">" +
         "         <div style=\"font-size: 1.5rem;\">Cannot load page content. Please use external browser to navigate to <a href=\"ext{url}\">{url}</a>.</div>" +
         "      </div>" +
         "   </body>" +
         "</html>";

   private boolean initialLoad = true;

   /**
    * @param parent
    * @param style
    */
   public WelcomePage(Composite parent, int style)
   {
      super(parent, style);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      setLayout(layout);

      Composite header = new Composite(this, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      header.setLayout(layout);
      header.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label title = new Label(header, SWT.NONE);
      title.setText("Welcome to NetXMS " + VersionInfo.baseVersion());
      title.setFont(JFaceResources.getBannerFont());
      title.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      ToolBar toolbar = new ToolBar(header, SWT.FLAT | SWT.HORIZONTAL | SWT.RIGHT);
      toolbar.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      Label separator = new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      ToolItem closeButton = new ToolItem(toolbar, SWT.PUSH);
      closeButton.setImage(SharedIcons.IMG_CLOSE);
      closeButton.setText("Close");
      closeButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            WelcomePage.this.dispose();
            PreferenceStore.getInstance().set("WelcomePage.LastDisplayedVersion", VersionInfo.baseVersion());
         }
      });

      Browser browser = WidgetHelper.createBrowser(this, SWT.NONE, null);
      browser.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      final String url = "https://netxms.com/release-notes";
      browser.addLocationListener(new LocationListener() {
         @Override
         public void changing(LocationEvent event)
         {
            if (event.location.startsWith("exthttp"))
            {
               event.doit = false;
               ExternalWebBrowser.open(event.location.substring(3));
            }
         }

         @Override
         public void changed(LocationEvent event)
         {
            if (initialLoad && (browser.getText().indexOf("<title>Release notes</title>") == -1))
            {
               initialLoad = false;
               browser.setText(ERROR_PAGE.replace("{url}", url));
            }
         }
      });
      browser.setUrl(url);
   }
}
