/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.views.helpers.WebServiceDefinitionLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Web service manager
 */
public class WebServiceManager extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.datacollection.views.WebServiceManager"; //$NON-NLS-1$

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_URL = 1;
   public static final int COLUMN_AUTHENTICATION = 2;
   public static final int COLUMN_LOGIN = 3;
   public static final int COLUMN_RETENTION_TIME = 4;
   public static final int COLUMN_TIMEOUT = 5;
   public static final int COLUMN_DESCRIPTION = 6;

   private Map<Integer, WebServiceDefinition> webServiceDefinitions = new HashMap<Integer, WebServiceDefinition>();
   private SortableTableViewer viewer;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      final String[] names = { "Name", "URL", "Authentication", "Login", "Retention Time", "Timeout", "Description" };
      final int[] widths = { 300, 600, 100, 160, 90, 90, 600 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new WebServiceDefinitionLabelProvider());

      refresh();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * Refresh list
    */
   private void refresh()
   {
      final NXCSession session = ConsoleSharedData.getSession();
      ConsoleJob job = new ConsoleJob("Get web service definitions", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<WebServiceDefinition> definitions = session.getWebServiceDefinitions();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  webServiceDefinitions.clear();
                  for(WebServiceDefinition d : definitions)
                     webServiceDefinitions.put(d.getId(), d);
                  viewer.setInput(webServiceDefinitions.values());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get web service definitions";
         }
      };
      job.setUser(false);
      job.start();
   }
}
