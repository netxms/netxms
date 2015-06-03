/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TableValueConfig;
import org.netxms.ui.eclipse.perfview.widgets.TableValue;
import org.netxms.ui.eclipse.tools.ViewRefreshController;

/**
 * "Table value" element for dashboard
 */
public class TableValueElement extends ElementWidget
{
	private TableValueConfig config;
	private TableValue viewer;
	
	/**
	 * @param parent
	 * @param element
	 * @param viewPart
	 */
	public TableValueElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
			config = TableValueConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new TableValueConfig();
		}
		
		if (config.getTitle().trim().isEmpty())
		{
			setLayout(new FillLayout());
			viewer = new TableValue(this, SWT.NONE, viewPart);
		}
		else
		{
			GridLayout layout = new GridLayout();
			layout.marginWidth = 0;
			layout.marginHeight = 0;
			setLayout(layout);

			Label title = new Label(this, SWT.CENTER);
			title.setText(config.getTitle().trim());
			title.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
			title.setFont(JFaceResources.getBannerFont());
			
			viewer = new TableValue(this, SWT.NONE, viewPart);
			viewer.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		}

		viewer.setObject(config.getObjectId(), config.getDciId());	
		viewer.refresh(null);

		final ViewRefreshController refreshController = new ViewRefreshController(viewPart, config.getRefreshRate(), new Runnable() {
			@Override
			public void run()
			{
				if (TableValueElement.this.isDisposed())
					return;
				
				viewer.refresh(null);
			}
		});
		
		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            refreshController.dispose();
         }
      });
	}
}
