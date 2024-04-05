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
package org.netxms.nxmc.modules.objects.propertypages;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.GeoArea;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.GeoLocationControlMode;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Sensor;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.worldmap.dialogs.GeoAreaSelectionDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Location Control" property page
 */
public class LocationControl extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(LocationControl.class);

   private DataCollectionTarget dcTarget;
   private Set<Integer> geoAreas = new HashSet<Integer>();
   private Map<Integer, GeoArea> geoAreaCache = new HashMap<Integer, GeoArea>();
   private Button checkGenerateEvent;
   private Combo locationControlMode;
   private TableViewer areaListViewer;
   private Button addButton;
   private Button deleteButton;
   private boolean initialGenerateEventSelection;
   private GeoLocationControlMode initialLocationControlMode;
   private boolean isAreaListModified = false;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public LocationControl(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(LocationControl.class).tr("Location Control"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "locationControl";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getParentId()
    */
   @Override
   public String getParentId()
   {
      return "location";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof AbstractNode) || (object instanceof MobileDevice) || (object instanceof Sensor);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      dcTarget = (DataCollectionTarget)object;
      initialGenerateEventSelection = dcTarget.isLocationChageEventGenerated();
      initialLocationControlMode = dcTarget.getGeoLocationControlMode();
      for(long id : dcTarget.getGeoAreas())
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
      checkGenerateEvent.setSelection(dcTarget.isLocationChageEventGenerated());
      checkGenerateEvent.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      locationControlMode = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY | SWT.DROP_DOWN, "Location control mode",
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      locationControlMode.add("No control");
      locationControlMode.add("Alert when within restricted area");
      locationControlMode.add("Alert when outside allowed area");
      locationControlMode.select(dcTarget.getGeoLocationControlMode().getValue());

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
            GeoArea area1 = geoAreaCache.get((Integer)e1);
            GeoArea area2 = geoAreaCache.get((Integer)e2);
            String n1 = (area1 != null) ? area1.getName() : ("[" + e1 + "]");
            String n2 = (area2 != null) ? area2.getName() : ("[" + e2 + "]");
            return n1.compareToIgnoreCase(n2);
         }
      });
      areaListViewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            GeoArea area = geoAreaCache.get((Integer)element);
            return (area != null) ? area.getName() : ("[" + element + "]");
         }
      });
      areaListViewer.setInput(geoAreas);

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttons.setLayout(buttonLayout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("Add..."));
      addButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            GeoAreaSelectionDialog dlg = new GeoAreaSelectionDialog(getShell(), new ArrayList<GeoArea>(geoAreaCache.values()));
            if (dlg.open() != Window.OK)
               return;
            int areaId = dlg.getAreaId();
            if (geoAreas.contains(areaId))
               return;
            geoAreas.add(areaId);
            areaListViewer.refresh();
            isAreaListModified = true;
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);

      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("Delete"));
      deleteButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @SuppressWarnings("unchecked")
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            IStructuredSelection selection = areaListViewer.getStructuredSelection();
            Iterator<Integer> it = selection.iterator();
            if (it.hasNext())
            {
               while(it.hasNext())
               {
                  geoAreas.remove(it.next());
               }
               areaListViewer.refresh();
               isAreaListModified = true;
            }
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);

      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Reading configured geographical areas"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot get configured geographical areas");
         }
      };
      job.setUser(false);
      job.start();

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if ((checkGenerateEvent.getSelection() == initialGenerateEventSelection) &&
          (GeoLocationControlMode.getByValue(locationControlMode.getSelectionIndex()) == initialLocationControlMode) &&
          !isAreaListModified)
         return true;   // No changes
      
      final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setGeoLocationControlMode(GeoLocationControlMode.getByValue(locationControlMode.getSelectionIndex()));
      md.setObjectFlags(checkGenerateEvent.getSelection() ? DataCollectionTarget.DCF_LOCATION_CHANGE_EVENT : 0, DataCollectionTarget.DCF_LOCATION_CHANGE_EVENT);
      long[] areas = new long[geoAreas.size()];
      int index = 0;
      for(Integer id : geoAreas)
         areas[index++] = id;
      md.setGeoAreas(areas);

      if (isApply)
         setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating geolocation control settings for object {0}", object.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot modify object's geolocation control settings");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     isAreaListModified = false;
                     initialGenerateEventSelection = checkGenerateEvent.getSelection();
                     initialLocationControlMode = md.getGeoLocationControlMode();
                     LocationControl.this.setValid(true);
                  }
               });
            }
         }
      }.start();

      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      checkGenerateEvent.setSelection(false);
      locationControlMode.select(GeoLocationControlMode.NO_CONTROL.getValue());
      if (!geoAreas.isEmpty())
      {
         geoAreas.clear();
         areaListViewer.refresh();
         isAreaListModified = true;
      }
   }
}
