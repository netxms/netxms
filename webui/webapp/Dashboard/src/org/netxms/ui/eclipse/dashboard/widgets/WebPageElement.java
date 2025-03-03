/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.internal.WebPageConfig;
import com.google.gson.Gson;

/**
 * Embedded web page element for dashboard
 */
public class WebPageElement extends ElementWidget
{
	private Browser browser;
	private WebPageConfig config;
	
	/**
	 * @param parent
	 * @param data
	 */
	public WebPageElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
         config = new Gson().fromJson(element.getData(), WebPageConfig.class);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new WebPageConfig();
		}

      processCommonSettings(config);

      browser = new Browser(getContentArea(), SWT.NONE);
		browser.setUrl(config.getUrl());
	}
}
