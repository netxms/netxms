/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.GeoArea;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Location Control" property page
 */
public class LocationControl extends PropertyPage
{
   private DataCollectionTarget object;
   private Set<Integer> geoAreas = new HashSet<Integer>();
   private Map<Integer, GeoArea> geoAreaCache = new HashMap<Integer, GeoArea>();
   private Button checkGenerateEvent;
   private Combo locationControlMode;
   private TableViewer areaListViewer;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      object = (DataCollectionTarget)getElement().getAdapter(DataCollectionTarget.class);
      for(long id : object.getGeoAreas())
         geoAreas.add((int)id);

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 1;
      dialogArea.setLayout(layout);

      checkGenerateEvent = new Button(dialogArea, SWT.CHECK);
      checkGenerateEvent.setText("&Generate event when location changes");
      checkGenerateEvent.setSelection(object.isLocationChageEventGenerated());
      checkGenerateEvent.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      locationControlMode = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY | SWT.DROP_DOWN, "Location control mode",
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      locationControlMode.add("No control");
      locationControlMode.add("Alert when within restricted area");
      locationControlMode.add("Alert when outside allowed area");
      locationControlMode.select(object.getGeoLocationControlMode().getValue());

      Composite viewerComposite = new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 1;
      viewerComposite.setLayout(layout);
      viewerComposite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      new Label(viewerComposite, SWT.LEFT).setText("Areas");

      areaListViewer = new TableViewer(viewerComposite, SWT.BORDER);
      areaListViewer.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      areaListViewer.setContentProvider(new ArrayContentProvider());
      areaListViewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((GeoArea)e1).getName().compareToIgnoreCase(((GeoArea)e2).getName());
         }
      });
      areaListViewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((GeoArea)element).getName();
         }
      });
      areaListViewer.setInput(geoAreas);

      final NXCSession session = ConsoleSharedData.getSession();
      ConsoleJob job = new ConsoleJob("Read configured geographical areas", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<GeoArea> areas = session.getGeoAreas();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  for(GeoArea a : areas)
                     geoAreaCache.put(a.getId(), a);
                  areaListViewer.refresh(true);
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

      return dialogArea;
   }
}
