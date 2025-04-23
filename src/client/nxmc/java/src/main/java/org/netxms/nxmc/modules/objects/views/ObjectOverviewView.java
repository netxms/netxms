/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Layout;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.elements.Capabilities;
import org.netxms.nxmc.modules.objects.views.elements.Commands;
import org.netxms.nxmc.modules.objects.views.elements.Comments;
import org.netxms.nxmc.modules.objects.views.elements.Communications;
import org.netxms.nxmc.modules.objects.views.elements.Connection;
import org.netxms.nxmc.modules.objects.views.elements.DeviceBackup;
import org.netxms.nxmc.modules.objects.views.elements.ExternalResources;
import org.netxms.nxmc.modules.objects.views.elements.Inventory;
import org.netxms.nxmc.modules.objects.views.elements.LastValues;
import org.netxms.nxmc.modules.objects.views.elements.Location;
import org.netxms.nxmc.modules.objects.views.elements.ObjectInfo;
import org.netxms.nxmc.modules.objects.views.elements.ObjectState;
import org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement;
import org.netxms.nxmc.modules.objects.views.elements.PollStates;
import org.netxms.nxmc.modules.objects.views.elements.Topology;
import org.netxms.nxmc.modules.objects.views.elements.InterfaceTrafficChart;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Overview" view for objects
 */
public class ObjectOverviewView extends ObjectView
{
   private I18n i18n = LocalizationHelper.getI18n(ObjectOverviewView.class);

   private Set<OverviewPageElement> elements = new HashSet<OverviewPageElement>();
   private ScrolledComposite scroller;
   private Composite viewArea;
   private Composite leftColumn;
   private Composite rightColumn;
   private boolean updateLayoutOnSelect = false;

   /**
    * Constructor.
    */
   public ObjectOverviewView()
   {
      super(LocalizationHelper.getI18n(ObjectOverviewView.class).tr("Overview"), ResourceManager.getImageDescriptor("icons/object-views/overview.png"), "objects.overview", false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) &&
            ((context instanceof DataCollectionTarget) || (context instanceof Chassis) || (context instanceof Interface) || (context instanceof Subnet) || (context instanceof Zone));
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 10;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
      scroller.setExpandVertical(true);
      scroller.setExpandHorizontal(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            Rectangle r = scroller.getClientArea();
            scroller.setMinSize(viewArea.computeSize(r.width, SWT.DEFAULT));
            onObjectChange(getObject());
         }
      });

      viewArea = new Composite(scroller, SWT.NONE);
      viewArea.setBackground(ThemeEngine.getBackgroundColor("Dashboard"));
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      viewArea.setLayout(layout);
      scroller.setContent(viewArea);

      leftColumn = new Composite(viewArea, SWT.NONE);
      leftColumn.setLayout(createColumnLayout());
      leftColumn.setBackground(viewArea.getBackground());
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      leftColumn.setLayoutData(gd);

      rightColumn = new Composite(viewArea, SWT.NONE);
      rightColumn.setLayout(createColumnLayout());
      rightColumn.setBackground(viewArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.minimumWidth = SWT.DEFAULT;
      rightColumn.setLayoutData(gd);

      // Left column
      OverviewPageElement e = new ObjectInfo(leftColumn, null, this);
      elements.add(e);
      e = new ObjectState(leftColumn, e, this);
      elements.add(e);
      e = new Communications(leftColumn, e, this);
      elements.add(e);
      e = new Topology(leftColumn, e, this);
      elements.add(e);
      e = new Inventory(leftColumn, e, this);
      elements.add(e);
      e = new Location(leftColumn, e, this);
      elements.add(e);
      e = new LastValues(leftColumn, e, this);
      elements.add(e);   
      e = new ExternalResources(leftColumn, e, this);
      elements.add(e);
      e = new Comments(leftColumn, e, this);
      elements.add(e);

      // Right column
      e = new Commands(rightColumn, null, this);
      elements.add(e);
      e = new Capabilities(rightColumn, e, this);
      elements.add(e);
      e = new DeviceBackup(rightColumn, e, this);
      elements.add(e);
      e = new PollStates(rightColumn, e, this);
      elements.add(e);
      e = new Connection(rightColumn, e, this);
      elements.add(e);
      e = new InterfaceTrafficChart(rightColumn, e, this, false);
      elements.add(e);   
      e = new InterfaceTrafficChart(rightColumn, e, this, true);
      elements.add(e); 
   }

   /**
    * Create layout for column
    * 
    * @return
    */
   private Layout createColumnLayout()
   {
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 5;
      return layout;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      super.onObjectUpdate(object);
      onObjectChange(object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if (isActive())
         updateLayout();
      else
         updateLayoutOnSelect = true;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      if (updateLayoutOnSelect)
      {
         updateLayout();
         updateLayoutOnSelect = false;
      }
   }

   /**
    * Update view layout
    */
   private void updateLayout()
   {
      AbstractObject object = getObject();

      WidgetHelper.setRedraw(viewArea, false);
      for(OverviewPageElement element : elements)
      {
         if ((object != null) && element.isApplicableForObject(object))
         {
            element.setObject(object);
         }
         else
         {
            element.dispose();
         }
      }
      for(OverviewPageElement element : elements)
         element.fixPlacement();
      viewArea.layout(true, true);
      WidgetHelper.setRedraw(viewArea, true);

      Rectangle r = scroller.getClientArea();
      scroller.setMinSize(viewArea.computeSize(r.width, SWT.DEFAULT));

      Point s = viewArea.getSize();
      viewArea.redraw(0, 0, s.x, s.y, true);
   }

   /**
    * Request element layout. Can be used by elements when they need to relayout other elements after size change.
    */
   public void requestElementLayout()
   {
      viewArea.layout(true, true);
      Rectangle r = scroller.getClientArea();
      scroller.setMinSize(viewArea.computeSize(r.width, SWT.DEFAULT));
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Resynchronize object"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.syncObjectSet(new long[] { getObjectId() }, NXCSession.OBJECT_SYNC_NOTIFY);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot initiate object synchronization");
         }
      }.start();
   }
}
