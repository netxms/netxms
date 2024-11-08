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
package org.netxms.nxmc.modules.reporting.views;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.reporting.ReportDefinition;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.views.NavigationView;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Report navigation view
 */
public class ReportNavigationView extends NavigationView
{
   private final I18n i18n = LocalizationHelper.getI18n(ReportNavigationView.class);

   private TableViewer viewer;
   private Image validReportIcon;
   private Image invalidReportIcon;

   /**
    * @param name
    * @param image
    * @param id
    * @param hasFilter
    * @param showFilterTooltip
    * @param showFilterLabel
    */
   public ReportNavigationView()
   {
      super(LocalizationHelper.getI18n(ReportNavigationView.class).tr("Reports"), ResourceManager.getImageDescriptor("icons/report.png"), "ReportNavigator", true, false, true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      validReportIcon = ResourceManager.getImage("icons/report.png");
      invalidReportIcon = ResourceManager.getImage("icons/invalid-report.png");

      viewer = new TableViewer(parent, SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public Image getImage(Object element)
         {
            return ((ReportDefinition)element).isValid() ? validReportIcon : invalidReportIcon;
         }

         @Override
         public String getText(Object element)
         {
            return ((ReportDefinition)element).getName();
         }
      });

      ReportListFilter filter = new ReportListFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.NavigationView#getSelectionProvider()
    */
   @Override
   public ISelectionProvider getSelectionProvider()
   {
      return viewer;
   }

   /**
    * @see org.netxms.nxmc.base.views.NavigationView#setSelection(java.lang.Object)
    */
   @Override
   public void setSelection(Object selection)
   {
      viewer.setSelection(new StructuredSelection(selection));
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if (validReportIcon != null)
         validReportIcon.dispose();
      if (invalidReportIcon != null)
         invalidReportIcon.dispose();
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Loading report definitions"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<ReportDefinition> definitions = new ArrayList<ReportDefinition>();
            for(UUID reportId : session.listReports())
            {
               try
               {
                  final ReportDefinition definition = session.getReportDefinition(reportId);
                  definitions.add(definition);
               }
               catch(NXCException e)
               {
                  if (e.getErrorCode() == RCC.INTERNAL_ERROR)
                     definitions.add(new ReportDefinition(reportId));
               }
            }
            Collections.sort(definitions, (d1, d2) -> d1.getName().compareTo(d2.getName()));
            runInUIThread(() -> viewer.setInput(definitions));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error loading report definitions");
         }
      }.start();
   }

   /**
    * Filter for report list
    */
   private static class ReportListFilter extends ViewerFilter implements AbstractViewerFilter
   {
      private String filterString = null;

      /**
       * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
       */
      @Override
      public void setFilterString(String string)
      {
         this.filterString = (filterString != null) ? filterString.toLowerCase() : null;
      }

      /**
       * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
       */
      @Override
      public boolean select(Viewer viewer, Object parentElement, Object element)
      {
         if ((filterString == null) || filterString.isEmpty())
            return true;
         return ((ReportDefinition)element).getName().toLowerCase().contains(filterString);
      }
   }
}
