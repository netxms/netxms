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
package org.netxms.nxmc.base.views;

import java.lang.reflect.Constructor;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Pinboard perspective
 */
public class PinboardPerspective extends Perspective
{
   
   private static final Logger logger = LoggerFactory.getLogger(PinboardPerspective.class);
   
   /**
    * @param name
    * @param image
    */
   public PinboardPerspective()
   {
      super("pinboard", LocalizationHelper.getI18n(PinboardPerspective.class).tr("Pinboard"), ResourceManager.getImage("icons/perspective-pinboard.png"));
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = false;
      configuration.hasSupplementalArea = false;
      configuration.multiViewMainArea = true;
      configuration.enableViewPinning = false;
      configuration.allViewsAreCloseable = true;
      configuration.useGlobalViewId = true;
      configuration.ignoreViewContext = true;
      configuration.enableViewHide = false;
      configuration.keyboardShortcut = new KeyStroke(SWT.F12);
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      List<String> views = new ArrayList<String>();
      for(View v : getAllMainViews())
      {
         String id = v.getGlobalId();        
         views.add(id);         
         Memento viewConfig = new Memento();
         v.saveState(viewConfig);
         memento.set(id + ".state", viewConfig);
      }  
      memento.set("PinboardPerspective.Views", views);
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento)
   {
      List<String> views = memento.getAsStringList("PinboardPerspective.Views");
      for (String id : views)
      {
         Memento viewConfig = memento.getAsMemento(id + ".state");
         View v = null;
         try
         {
            Class<?> viewClass = Class.forName(viewConfig.getAsString("class"));
            Constructor<?> c = viewClass.getDeclaredConstructor();
            c.setAccessible(true);         
            v = (View)c.newInstance();
            if (v != null)
            {
               v.restoreState(viewConfig);
               Registry.getMainWindow().pinView(v, PinLocation.PINBOARD);
            }
         }
         catch (Exception e)
         {
            Registry.getMainWindow().pinView(new NonRestorableView(e, v != null ? v.getFullName() : id), PinLocation.PINBOARD);
            logger.error("Cannot instantiate saved view", e);
         }
      }     
   }
}
