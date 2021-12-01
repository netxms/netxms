/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.reporting.model;

import java.util.ResourceBundle;
import org.netxms.base.NXCPMessage;
import net.sf.jasperreports.engine.JRExpression;
import net.sf.jasperreports.engine.JRParameter;
import net.sf.jasperreports.engine.JRPropertiesMap;

/**
 * Report parameter
 */
public class ReportParameter
{
   private int index;
   private String name;
   private String dependsOn;
   private String description;
   private String type;
   private String defaultValue;
   private String multiselectValues;
   private int span;

   /**
    * Create parameter definition from JRParameter object and localization bundle.
    *
    * @param jrParameter JRParameter object
    * @param labels localization bundle
    * @param index default index
    */
   public ReportParameter(JRParameter jrParameter, ResourceBundle labels, int index)
   {
      name = jrParameter.getName();
      description = getTranslation(labels, name);

      final JRPropertiesMap propertiesMap = jrParameter.getPropertiesMap();
      type = ReportDefinition.getPropertyFromMap(propertiesMap, "logicalType", jrParameter.getValueClass().getName());
      this.index = ReportDefinition.getPropertyFromMap(propertiesMap, "index", index);
      dependsOn = ReportDefinition.getPropertyFromMap(propertiesMap, "dependsOn", "");
      multiselectValues = ReportDefinition.getPropertyFromMap(propertiesMap, "multiselectValues", "");
      span = ReportDefinition.getPropertyFromMap(propertiesMap, "span", 1);
      if (span < 1)
         span = 1;

      final JRExpression defaultValue = jrParameter.getDefaultValueExpression();
      this.defaultValue = (defaultValue != null) ? defaultValue.getText() : null;
   }

   /**
    * Fill NXCP message with parameter's data.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setFieldInt32(baseId, index);
      msg.setField(baseId + 1, name);
      msg.setField(baseId + 2, description);
      msg.setField(baseId + 3, type);
      msg.setField(baseId + 4, defaultValue);
      msg.setField(baseId + 5, dependsOn);
      msg.setFieldInt32(baseId + 6, span);
      msg.setField(baseId + 7, multiselectValues);
   }

   /**
    * Get parameter's name.
    *
    * @return parameter's name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ReportParameter [index=" + index + ", name=" + name + ", dependsOn=" + dependsOn + ", description=" + description + ", type=" + type + ", defaultValue=" + defaultValue +
            ", multiselectValues=" + multiselectValues + ", span=" + span + "]";
   }

   /**
    * Get translation for given string
    *
    * @param bundle bundle to use
    * @param name string name
    * @return translated string or same string if translation not found
    */
   private static String getTranslation(ResourceBundle bundle, String name)
   {
      if (bundle.containsKey(name))
      {
         return bundle.getString(name);
      }
      return name;
   }
}
