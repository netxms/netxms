/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Predefined map view for maps displayed in object context
 */
public class ContextPredefinedMapView extends PredefinedMapView
{
   private final I18n i18n = LocalizationHelper.getI18n(ContextPredefinedMapView.class);

   private NetworkMap map;
   private SessionListener clientListener;

   /**
    * Create ad-hoc map view.
    * 
    * @param contextObjectId ID of object that is context for this view
    * @param chassis chassis object to be shown
    */
   public ContextPredefinedMapView(NetworkMap map)
   {
      super("objects.context.network-map." + Long.toString(map.getObjectId()));
      this.map = map;
   }

   /**
    * Constructor for cloning
    */
   protected ContextPredefinedMapView()
   {
      super();
      objectMoveLocked = false;
      readOnly = true;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      ContextPredefinedMapView view = (ContextPredefinedMapView)super.cloneView();
      view.map = map;
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      clientListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_CHANGED && map.getObjectId() == n.getSubCode())
            {
               getViewArea().getDisplay().asyncExec(() -> {
                  map = (NetworkMap)n.getObject();
                  setName(map.getObjectName());
                  ContextPredefinedMapView.super.onObjectUpdate(map);
               });
            }
         }
      };

      session.addListener(clientListener);
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
      return (context != null) && (context instanceof AbstractObject) && ((AbstractObject)context).hasNetworkMap(map.getObjectId());
   }

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.PredefinedMapView#getPriority()
    */
   @Override
   public int getPriority()
   {
      return (map.getDisplayPriority() > 0) ? map.getDisplayPriority() : DEFAULT_PRIORITY;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getName()
    */
   @Override
   public String getName()
   {
      return map.getObjectName();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
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
      memento.set("map", map.getObjectId());
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      map = session.findObjectById(memento.getAsLong("map", 0), NetworkMap.class);  
      if (map == null)
         throw (new ViewNotRestoredException(i18n.tr("Invalid map ID")));
   }
}
