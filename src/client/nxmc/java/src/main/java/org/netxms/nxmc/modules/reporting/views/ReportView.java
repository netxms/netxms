/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.reporting.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.reporting.ReportDefinition;
import org.netxms.nxmc.base.views.ViewWithContext;
import org.netxms.nxmc.modules.reporting.widgets.ReportExecutionForm;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Report view
 */
public class ReportView extends ViewWithContext
{
   private Composite content;
   private ReportExecutionForm form;

   /**
    * Create report view
    */
   public ReportView()
   {
      super("Report", ResourceManager.getImageDescriptor("icons/report.png"), "Report", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FillLayout());
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
      if (form != null)
         form.dispose();

      if ((newContext != null) && (newContext instanceof ReportDefinition))
         form = new ReportExecutionForm(content, SWT.NONE, (ReportDefinition)newContext, this);
      else
         form = null;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      return getContextName();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getContextName()
    */
   @Override
   protected String getContextName()
   {
      Object context = getContext();
      return ((context != null) && (context instanceof ReportDefinition)) ? ((ReportDefinition)context).getName() : "";
   }
}
