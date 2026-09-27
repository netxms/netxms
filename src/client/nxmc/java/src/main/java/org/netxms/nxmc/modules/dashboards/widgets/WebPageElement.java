/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets;

import java.util.Collections;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectContextBase;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.WebPageConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.google.gson.Gson;

/**
 * Embedded web page element for dashboard
 */
public class WebPageElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(WebPageElement.class);

   private final I18n i18n = LocalizationHelper.getI18n(WebPageElement.class);

	private WebPageConfig config;
	
	/**
    * @param parent
    * @param element
    * @param view
    */
   public WebPageElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view);

		try
		{
         config = new Gson().fromJson(element.getData(), WebPageConfig.class);
         if (config == null)
            config = new WebPageConfig();
		}
		catch(Exception e)
		{
         logger.error("Cannot parse dashboard element configuration", e);
			config = new WebPageConfig();
		}

      processCommonSettings(config);

      final AbstractObject contextObject = getContext();
      if (config.isExpandMacros() && (contextObject != null))
         expandMacrosAndCreateBrowser(contextObject);
      else
         WidgetHelper.createBrowser(getContentArea(), SWT.NONE, config.getUrl());
	}

   /**
    * Expand macros in configured URL using given context object and create browser with expanded URL. Original URL is used if
    * expansion fails.
    *
    * @param contextObject context object
    */
   private void expandMacrosAndCreateBrowser(final AbstractObject contextObject)
   {
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Expanding macros in web page URL"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            String url;
            try
            {
               url = session.substituteMacros(new ObjectContextBase(contextObject, null), Collections.singletonList(config.getUrl()), null).get(0);
            }
            catch(Exception e)
            {
               logger.error("Cannot expand macros in web page URL \"" + config.getUrl() + "\"", e);
               url = config.getUrl();
            }
            final String expandedUrl = url;
            runInUIThread(() -> {
               if (isDisposed())
                  return;
               WidgetHelper.createBrowser(getContentArea(), SWT.NONE, expandedUrl);
               getContentArea().layout(true, true);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot expand macros in web page URL");
         }
      }.start();
   }
}
