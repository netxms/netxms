/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Solutions
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
package org.netxms.nxmc.modules.worldmap.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.GeoArea;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;

/**
 * Geo area selection dialog
 */
public class GeoAreaSelectionDialog extends Dialog
{
   private static final String TABLE_CONFIG_PREFIX = "GeoAreaSelectionDialog"; //$NON-NLS-1$

   private List<GeoArea> cachedAreaList;
   private TableViewer viewer;
   private int areaId = 0;

   /**
    * Create new dialog
    *
    * @param parentShell parent shell
    */
   public GeoAreaSelectionDialog(Shell parentShell, List<GeoArea> cachedAreaList)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.cachedAreaList = cachedAreaList;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Select Geo Area");
      PreferenceStore settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsInteger(TABLE_CONFIG_PREFIX + ".cx", 300), settings.getAsInteger(TABLE_CONFIG_PREFIX + ".cy", 400));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      dialogArea.setLayout(new FillLayout());

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((GeoArea)e1).getName().compareToIgnoreCase(((GeoArea)e2).getName());
         }
      });
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((GeoArea)element).getName();
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            okPressed();
         }
      });

      if (cachedAreaList != null)
      {
         viewer.setInput(cachedAreaList);
      }
      else
      {
         final NXCSession session = Registry.getSession();
         Job job = new Job("Read configured geographical areas", null) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final List<GeoArea> areas = session.getGeoAreas();
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.setInput(areas);
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot get configured geographical areas";
            }
         };
         job.setUser(false);
         job.start();
      }
      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      areaId = selection.isEmpty() ? 0 : ((GeoArea)selection.getFirstElement()).getId();
      super.okPressed();
   }

   /**
    * Get selected geographical area ID.
    *
    * @return selected geographical area ID
    */
   public int getAreaId()
   {
      return areaId;
   }
}
