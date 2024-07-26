/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2024 Raden Soultions
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

import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.PDFViewer;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Report result view
 */
public class ReportResultView extends View
{
   private static Map<UUID, Integer> instances = new ConcurrentHashMap<>();

   private UUID renderId;
   private String resultFileUrl;
   private Runnable disposeCallback;

   /**
    * @param name
    * @param renderId
    * @param resultFile
    * @param disposeCallback
    */
   public ReportResultView(String name, UUID renderId, String resultFileUrl, Runnable disposeCallback)
   {
      super(name, ResourceManager.getImageDescriptor("icons/object-views/report.png"), "ReportResult." + renderId, false);
      this.renderId = renderId;
      this.resultFileUrl = resultFileUrl;
      this.disposeCallback = disposeCallback;
      Integer n = instances.get(renderId);
      instances.put(renderId, (n != null) ? n + 1 : 1);
   }

   /**
    * 
    */
   protected ReportResultView()
   {
      super("", ResourceManager.getImageDescriptor("icons/object-views/report.png"), "ReportResult.null", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#cloneView()
    */
   @Override
   public View cloneView()
   {
      ReportResultView view = (ReportResultView)super.cloneView();
      view.renderId = renderId;
      view.resultFileUrl = resultFileUrl;
      view.disposeCallback = disposeCallback;
      Integer n = instances.get(renderId);
      instances.put(renderId, (n != null) ? n + 1 : 1);
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      new PDFViewer(parent, SWT.NONE).openUrl(resultFileUrl);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      Integer n = instances.get(renderId);
      if ((n == null) || (n <= 1))
      {
         instances.remove(renderId);
         if (disposeCallback != null)
            disposeCallback.run();
      }
      else
      {
         instances.put(renderId, n - 1);
      }
      super.dispose();
   }
}
