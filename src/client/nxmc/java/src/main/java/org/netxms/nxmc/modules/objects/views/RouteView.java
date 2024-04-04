/**
 * NetXMS - open source network management system
 * Copyright (C) 2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.topology.NetworkPath;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.RouteLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * IP route view
 */
public class RouteView extends AdHocObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(RouteView.class);

   public static final int COLUMN_NODE = 0;
   public static final int COLUMN_NEXT_HOP = 1;
   public static final int COLUMN_TYPE = 2;
   public static final int COLUMN_NAME = 3;

   /**
    * Generate name for the view.
    *
    * @param source source object
    * @param destination destination object
    * @param contextId context object
    * @return generated view name
    */
   private static String generateName(AbstractNode source, AbstractNode destination, long contextId)
   {
      if (source.getObjectId() == contextId)
         return i18n.tr("Route to {0}", destination.getObjectName());
      if (destination.getObjectId() == contextId)
         return i18n.tr("Route from {0}", source.getObjectName());
      return i18n.tr("Route {0} - {1}", source.getObjectName(), destination.getObjectName());
   }

   private SortableTableViewer viewer;
   private AbstractNode source;
   private AbstractNode destination;

   /**
    * @param name
    * @param image
    * @param baseId
    * @param objectId
    * @param contextId
    * @param hasFilter
    */
   public RouteView(AbstractNode source, AbstractNode destination, long contextId)
   {
      super(generateName(source, destination, contextId), ResourceManager.getImageDescriptor("icons/object-views/route.png"),
            "objects.routes." + source.getObjectId() + "." + destination.getObjectId(), contextId, contextId, false);
      this.source = source;
      this.destination = destination;
   }

   /**
    * Constructor for cloning
    */
   protected RouteView()
   {
      super();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] names = { i18n.tr("Node"), i18n.tr("Next hop"), i18n.tr("Type"), i18n.tr("Name") };
      final int[] widths = { 150, 150, 150, 150 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.disableSorting();
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new RouteLabelProvider());
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.AdHocObjectView#cloneView()
    */
   @Override
   public View cloneView()
   {
      RouteView view = (RouteView)super.cloneView();
      view.source = source;
      view.destination = destination;
      return view;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      return generateName(source, destination, 0);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Reading network path"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final NetworkPath path = session.getNetworkPath(source.getObjectId(), destination.getObjectId());
            runInUIThread(() -> {
               viewer.setInput(path.getPath());
               viewer.packColumns();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot retrieve network path between {0} and {1}", source.getObjectName(), destination.getObjectName());
         }
      }.start();
   }
}
