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

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.LocationAdapter;
import org.eclipse.swt.browser.LocationEvent;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Welcome page
 */
public class WelcomePage extends Composite
{
   private static final String ERROR_PAGE = 
         "<html>"+
         "<head>" +
         "   <style>" +
         ".button {" +
         "   text-decoration: none;" +
         "   font-weight: 600;" +
         "   background-color: #3498db;" +
         "   color: #f0f0f0;" +
         "   padding-left: 24px;" +
         "   padding-right: 24px;" +
         "   padding-top: 10px;" +
         "   padding-bottom: 10px;" +
         "   border: 1px solid #ffffff;" +
         "   border-radius: 8px;" +
         "}" +
         "   </style>" +
         "</head>" +
         "   <body>" +
         "      <div style=\"padding: 10px; display: flex; flex-wrap: nowrap; align-items: center;\">" +
         "         <img src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAYAAACqaXHeAAAACXBIWXMAAAsTAAALEwEAmpwYAAAClUlEQVR4nO2bzXGDMBCFXQIdhEMKcAkMFaQEl0AJmjTgEiiBErjk7hJcAUMJjtYWDsESSNrVH9bOvFskvfcZCSLE4UBU9fdPwzVy3RzrwnXmKqm8o4ubaT0El+kcOnvI8JP6dw4/qY0tfM/FHAjGVK0zVSzhQczh2AVXJxmzczXm0oDOZc8cewAI1+W4LsecBtad88yDl8brNDBc8JgzI39+Km8ADMPvC4BF+P0AsAy/DwCI8OkDQIZPGwBB+HQBEIVPEwBh+PQAEIcHNbRxpZ5pANTyR0qsSvLEr77JAFBvY3nZpaEEQBne28aESwB9bb5RAdPoSB9z1bczAIzarIvKADKADCADyAAygAwgA9gFgOHj84ur4+qFTlttTADw/ipl/6EBcDMt100iMFqo2kkAwLmBl79f6f8ydRQMgPjlZeaeJlUQFgBMw09ioQF0GwaVEGYAbMODrqEB9Bom1yAwRPi7UgGwOh0WfWqH5xpDAzgZmN2EYBge1MZwFzC5CpQQLMI/+okAQCHMWEOwDg8VGgAWAip8LAAQEEb09IkFAJQlBNwCGhMAKEcQ1HeP2ABAEUNYf36IEQAUEYTth6dYAUAhIWg9OUYNAEpAMF3tQaXWAAkAaN/2CkCET38NIAif7l2AMHx6zwEOwq9DiAmAw/BqCLEAsAgPYcoB+a90FAAswxeiLW4/QQLAz6cnBOFnfVhBmAD0EghezvsMjzc2JKu5JQSmOic4+oAw6L0X2AyPgHCdPkBSnRXsatrP3sqF4Z4qvC2EeyNu7KQAQK3KAoB2eAsI47OR+IVcA2gWRrfeCxiHN4Tw/2CnuBJcfgF+kRhVmbQOrwlhlPYv1oRGcXeg0FFiksGCNDPWYsMvxlD2/wtnrMBIlZAfegAAAABJRU5ErkJggg==\">" +
         "         <div style=\"font-size: 1.5rem; padding: 0.5rem;\">Cannot load page content.<br>Please use external browser to navigate to <a href=\"ext{url}\">{url}</a>.</div>" +
         "      </div>" +
         "      <div style=\"padding: 10px; display: flex; flex-wrap: nowrap; align-items: center;\">" +
         "         <p style=\"text-align: center; padding-top: 16px; padding-bottom: 16px;\"><a href=\"app:close\" class=\"button\">CLOSE</a></p>" +
         "      </div>" +
         "   </body>" +
         "</html>";

   private final I18n i18n = LocalizationHelper.getI18n(WelcomePage.class);

   /**
    * Create welcome page control.
    *
    * @param parent parent composite
    * @param style style for control
    * @param serverVersion server version to display information about
    */
   public WelcomePage(Composite parent, int style, final String serverVersion)
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
      layout.marginWidth = 10;
      header.setLayout(layout);
      header.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label title = new Label(header, SWT.NONE);
      title.setText("Welcome to NetXMS " + serverVersion);
      title.setData(RWT.CUSTOM_VARIANT, "MainWindowHeaderBold");
      title.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Button closeButton = new Button(header, SWT.PUSH);
      closeButton.setData(RWT.CUSTOM_VARIANT, "WelcomePageButton");
      GridData gd = new GridData(SWT.RIGHT, SWT.CENTER, false, false);
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      closeButton.setLayoutData(gd);
      closeButton.setText(i18n.tr("Close"));
      closeButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            WelcomePage.this.dispose();
            PreferenceStore.getInstance().set("WelcomePage.LastDisplayedVersion", serverVersion);
         }
      });

      Label separator = new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Browser browser = WidgetHelper.createBrowser(this, SWT.NONE, null);
      if (browser != null)
      {
         browser.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

         browser.addLocationListener(new LocationAdapter() {
            @Override
            public void changing(LocationEvent event)
            {
               if (event.location.equals("app:close"))
               {
                  WelcomePage.this.dispose();
                  PreferenceStore.getInstance().set("WelcomePage.LastDisplayedVersion", serverVersion);
               }
            }
         });

         final String url = "https://netxms.github.io/changelog/" + serverVersion.replace('.', '_') + ".html";
         new Job("Loading welcome page", null) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               try
               {
                  final String content = loadPage(url);
                  runInUIThread(() -> browser.setText(content));
               }
               catch(Exception e)
               {
                  runInUIThread(() -> browser.setText(ERROR_PAGE.replace("{url}", url)));
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return null;
            }
         }.start();
      }
   }

   /**
    * Load page at given URL.
    *
    * @param urlString URL
    * @return page content
    * @throws Exception on error
    */
   private static String loadPage(String urlString) throws Exception
   {
      StringBuilder result = new StringBuilder();
      URL url = new URL(urlString);
      HttpURLConnection conn = (HttpURLConnection)url.openConnection();
      conn.setRequestMethod("GET");
      try (BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream())))
      {
         String line;
         while((line = reader.readLine()) != null)
         {
            result.append(line).append("\n");
         }
      }
      return result.toString();
   }
}
