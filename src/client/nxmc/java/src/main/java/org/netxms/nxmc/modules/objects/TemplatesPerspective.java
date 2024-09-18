/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.util.HashSet;
import java.util.Set;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.interfaces.NodeComponent;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * "Templates" perspective
 */
public class TemplatesPerspective extends ObjectsPerspective
{
   public static final I18n i18n = LocalizationHelper.getI18n(TemplatesPerspective.class);

   private static final Set<Integer> classFilterTemplate = new HashSet<>(1);
   static
   {
      classFilterTemplate.add(AbstractObject.OBJECT_TEMPLATE);
   }

   /**
    * Create templates perspective
    */
   public TemplatesPerspective()
   {
      super("objects.templates", i18n.tr("Templates"), ResourceManager.getImage("icons/perspective-templates.png"), SubtreeType.TEMPLATES, 
            (o) -> {
               if ((o.getObjectClass() == AbstractObject.OBJECT_INTERFACE) || (o.getObjectClass() == AbstractObject.OBJECT_NETWORKSERVICE) ||
                     (o.getObjectClass() == AbstractObject.OBJECT_VPNCONNECTOR))
               {
                  AbstractNode node = ((NodeComponent)o).getParentNode();
                  return (node != null) && node.hasAccessibleParents(classFilterTemplate);
               }
               return (o.getObjectClass() == AbstractObject.OBJECT_TEMPLATE) || (o.getObjectClass() == AbstractObject.OBJECT_TEMPLATEGROUP) ||
                     (o.getObjectClass() == AbstractObject.OBJECT_TEMPLATEROOT) || o.hasAccessibleParents(classFilterTemplate);
            });
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ObjectsPerspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.priority = 16;
   }
}
