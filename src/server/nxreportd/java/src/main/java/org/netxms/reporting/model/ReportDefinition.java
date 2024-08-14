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
package org.netxms.reporting.model;

import java.util.HashMap;
import java.util.Map;
import java.util.ResourceBundle;
import java.util.Set;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import net.sf.jasperreports.engine.JRParameter;
import net.sf.jasperreports.engine.JRPropertiesMap;
import net.sf.jasperreports.engine.JasperReport;

/**
 * Report definition
 */
public class ReportDefinition
{
   private String name;
   private boolean requiresDataView = false;
   private boolean requiresResponsibleUsersView = false;
   private String responsibleUsersTag = null;
   private boolean carboneReport = false;
   private int numberOfColumns = 1;
   private Map<String, ReportParameter> parameters = new HashMap<String, ReportParameter>(0);

   /**
    * Create report definition object from JasperReport object and localization bundle.
    *
    * @param jasperReport JasperReport object
    * @param labels localization bundle
    */
   public ReportDefinition(JasperReport jasperReport, ResourceBundle labels)
   {
      name = jasperReport.getName();
      numberOfColumns = getPropertyFromMap(jasperReport.getPropertiesMap(), "org.netxms.reporting.numberOfColumns", 1);
      requiresDataView = getPropertyFromMap(jasperReport.getPropertiesMap(), "org.netxms.reporting.requiresDataView", false);
      requiresResponsibleUsersView = getPropertyFromMap(jasperReport.getPropertiesMap(), "org.netxms.reporting.requiresResponsibleUsersView", false);
      responsibleUsersTag = getPropertyFromMap(jasperReport.getPropertiesMap(), "org.netxms.reporting.responsibleUsersTag", null);
      carboneReport = getPropertyFromMap(jasperReport.getPropertiesMap(), "org.netxms.reporting.useCarbone", false);

      int index = 0;
      for(JRParameter jrParameter : jasperReport.getParameters())
      {
         if (jrParameter.isSystemDefined() || !jrParameter.isForPrompting())
            continue;

         final JRPropertiesMap propertiesMap = jrParameter.getPropertiesMap();
         if (!ReportDefinition.getPropertyFromMap(propertiesMap, "internal", false))
         {
            final ReportParameter parameter = new ReportParameter(jrParameter, labels, index++);
            parameters.put(parameter.getName(), parameter);
         }
      }
   }

   /**
    * Get report name
    *
    * @return report name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get report parameters.
    *
    * @return report parameters
    */
   public Map<String, ReportParameter> getParameters()
   {
      return parameters;
   }

   /**
    * Get number of columns in parameter query form.
    *
    * @return number of columns in parameter query form
    */
   public int getNumberOfColumns()
   {
      return numberOfColumns;
   }

   /**
    * Check if DCI data view is required for this report.
    *
    * @return true if DCI data view is required for this report
    */
   public boolean isDataViewRequired()
   {
      return requiresDataView;
   }

   /**
    * Check if responsible users view is required for this report.
    *
    * @return true if responsible users view is required for this report
    */
   public boolean isResponsibleUsersViewRequired()
   {
      return requiresResponsibleUsersView;
   }

   /**
    * Get optional tag for responsible users.
    *
    * @return optional tag for responsible users (can be null)
    */
   public String getResponsibleUsersTag()
   {
      return responsibleUsersTag;
   }

   /**
    * Returns true if this report definition is intended for Carbone renderer instead of standard Jasper renderer.
    * 
    * @return true if this report definition is intended for Carbone renderer
    */
   public boolean isCarboneReport()
   {
      return carboneReport;
   }

   /**
    * Fill NXCP message.
    *
    * @param message NXCP message
    */
   public void fillMessage(NXCPMessage message)
   {
      message.setField(NXCPCodes.VID_NAME, name);
      final int size = parameters.size();
      message.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, size);
      message.setFieldInt32(NXCPCodes.VID_NUM_COLUMNS, numberOfColumns);
      message.setField(NXCPCodes.VID_REQUIRES_DATA_VIEW, requiresDataView);
      message.setField(NXCPCodes.VID_USE_CARBONE_RENDERER, carboneReport);

      final Set<Map.Entry<String, ReportParameter>> entries = parameters.entrySet();
      long fieldId = NXCPCodes.VID_ROW_DATA_BASE;
      for(Map.Entry<String, ReportParameter> entry : entries)
      {
         entry.getValue().fillMessage(message, fieldId);
         fieldId += 10;
      }
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ReportDefinition [name=" + name + ", requiresDataView=" + requiresDataView + ", requiresResponsibleUsersView=" + requiresResponsibleUsersView + ", responsibleUsersTag=" +
            responsibleUsersTag + ", usesCarboneRenderer=" + carboneReport + ", numberOfColumns=" + numberOfColumns + ", parameters=" + parameters + "]";
   }

   /**
    * Get integer property from map.
    *
    * @param map map to use
    * @param key key name
    * @param defaultValue default value
    * @return property value or default value
    */
   protected static int getPropertyFromMap(JRPropertiesMap map, String key, int defaultValue)
   {
      if (map.containsProperty(key))
      {
         try
         {
            return Integer.parseInt(map.getProperty(key));
         }
         catch(NumberFormatException e)
         {
            return defaultValue;
         }
      }
      return defaultValue;
   }

   /**
    * Get integer property from map.
    *
    * @param map map to use
    * @param key key name
    * @param defaultValue default value
    * @return property value or default value
    */
   protected static boolean getPropertyFromMap(JRPropertiesMap map, String key, boolean defaultValue)
   {
      if (map.containsProperty(key))
         return Boolean.parseBoolean(map.getProperty(key));
      return defaultValue;
   }

   /**
    * Get string property from map.
    *
    * @param map map to use
    * @param key key name
    * @param defaultValue default value
    * @return property value or default value
    */
   protected static String getPropertyFromMap(JRPropertiesMap map, String key, String defaultValue)
   {
      if (map.containsProperty(key))
      {
         return map.getProperty(key);
      }
      return defaultValue;
   }
}
