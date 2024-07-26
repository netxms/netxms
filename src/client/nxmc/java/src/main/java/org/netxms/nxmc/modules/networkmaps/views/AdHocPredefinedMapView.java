/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.views;

import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.views.View;

/**
 * Ad-hoc map view
 */
public class AdHocPredefinedMapView extends PredefinedMapView
{
   private long contextObjectId;
   private NetworkMap map;
   
   /**
    * Create ad-hoc map view.
    * 
    * @param contextObjectId ID of object that is context for this view
    * @param chassis chassis object to be shown
    */
   public AdHocPredefinedMapView(long contextObjectId, NetworkMap map)
   {
      super(Long.toString(map.getObjectId()).toString());
      this.contextObjectId = contextObjectId;
      this.map = map;
   }

   /**
    * Constructor for cloning
    */
   protected AdHocPredefinedMapView()
   {
      super();
      editModeEnabled = true;
      readOnly = true;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      AdHocPredefinedMapView view = (AdHocPredefinedMapView)super.cloneView();
      view.contextObjectId = contextObjectId;
      view.map = map;
      return view;
   }
   
   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.PredefinedMapView#saveZoom(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void saveZoom(AbstractObject object) 
   {
      //Do not save zoom
   }
  
   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#loadZoom(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void loadZoom(AbstractObject object)
   {
      final PreferenceStore settings = PreferenceStore.getInstance();
      viewer.zoomTo(settings.getAsDouble(getBaseId() + ".zoom", 1.0));
   }
   
   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.PredefinedMapView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject) && (((AbstractObject)context).getObjectId() == contextObjectId);
   }

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.PredefinedMapView#getPriority()
    */
   @Override
   public int getPriority()
   {
      return DEFAULT_PRIORITY;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getName()
    */
   @Override
   public String getName()
   {
      return super.getName() + " - " + map.getObjectName();
   }

   /**
    * Get current context
    *
    * @return current context
    */
   @Override
   protected Object getContext()
   {
      return map;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      if (object.getObjectId() == map.getObjectId())
      {
         map = (NetworkMap)object;
         super.onObjectUpdate(object);         
      }
   }

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.PredefinedMapView#setupMapControl()
    */
   @Override
   public void setupMapControl()
   {
      super.onObjectChange(map); //set correct edit mode and zoom level
      super.setupMapControl();
   }

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.PredefinedMapView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      // Ignore context object change 
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isRelatedObject(long)
    */
   @Override
   protected boolean isRelatedObject(long objectId)
   {
      return map.getObjectId() == objectId;
   }
   
   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {        
      // Ignore context object change 
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("contextObjectId", contextObjectId);
      memento.set("map", map.getObjectId());
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento)
   {      
      super.restoreState(memento);
      contextObjectId = memento.getAsLong("contextObjectId", 0);    
      map = session.findObjectById(memento.getAsLong("map", 0), NetworkMap.class);      
   }
}
