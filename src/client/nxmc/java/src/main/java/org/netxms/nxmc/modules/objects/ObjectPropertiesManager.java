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
package org.netxms.nxmc.modules.objects;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.propertypages.AccessControl;
import org.netxms.nxmc.modules.objects.propertypages.Communication;
import org.netxms.nxmc.modules.objects.propertypages.General;
import org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Manager for object properties
 */
public class ObjectPropertiesManager
{
   private static final Logger logger = LoggerFactory.getLogger(ObjectPropertiesManager.class);
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectPropertiesManager.class);

   private static Set<Class<? extends ObjectPropertyPage>> pageClasses = new HashSet<Class<? extends ObjectPropertyPage>>();
   static
   {
      pageClasses.add(AccessControl.class);
      pageClasses.add(Communication.class);
      pageClasses.add(General.class);
   }

   public static boolean openObjectPropertiesDialog(AbstractObject object, Shell shell)
   {
      List<ObjectPropertyPage> pages = new ArrayList<ObjectPropertyPage>(pageClasses.size());
      for(Class<? extends ObjectPropertyPage> c : pageClasses)
      {
         try
         {
            ObjectPropertyPage p = c.getConstructor(AbstractObject.class).newInstance(object);
            if (p.isVisible())
               pages.add(p);
         }
         catch(Exception e)
         {
            logger.error("Error instantiating object property page", e);
         }
      }
      pages.sort(new Comparator<ObjectPropertyPage>() {
         @Override
         public int compare(ObjectPropertyPage p1, ObjectPropertyPage p2)
         {
            int rc = p1.getPriority() - p2.getPriority();
            return (rc != 0) ? rc : p1.getTitle().compareToIgnoreCase(p2.getTitle());
         }
      });

      PreferenceManager pm = new PreferenceManager();
      for(ObjectPropertyPage p : pages)
         pm.addToRoot(new PreferenceNode(p.getId(), p));

      PreferenceDialog dlg = new PreferenceDialog(shell, pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText(String.format(i18n.tr("Object Properties - %s"), object.getObjectName()));
         }
      };
      dlg.setBlockOnOpen(true);
      return dlg.open() == Window.OK;
   }
}
