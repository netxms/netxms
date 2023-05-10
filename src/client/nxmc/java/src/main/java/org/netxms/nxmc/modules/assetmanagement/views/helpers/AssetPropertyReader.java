/**
 * 
 */
package org.netxms.nxmc.modules.assetmanagement.views.helpers;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.constants.AMDataType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for reading data from asset property
 */
public class AssetPropertyReader
{
   private static final Logger logger = LoggerFactory.getLogger(AssetPropertyReader.class);

   private I18n i18n = LocalizationHelper.getI18n(AssetPropertyReader.class);
   private NXCSession session = Registry.getSession();
   private BaseObjectLabelProvider objectLabelProvider = new BaseObjectLabelProvider();

   /**
    * Convert property raw value to image. If no image should be displayed for this property, returns null.
    *
    * @param name property name
    * @param value property raw value
    * @return image to display or null
    */
   public Image valueToImage(String name, String value)
   {
      if (session.getAssetManagementSchema().get(name).getDataType() != AMDataType.OBJECT_REFERENCE)
         return null;

      long objectId = 0;
      try
      {
         objectId = Integer.parseInt(value);
      }
      catch(NumberFormatException e)
      {
         logger.warn("Cannot parse object ID", e);
      }
      AbstractObject object = session.findObjectById(objectId);
      return (object != null) ? objectLabelProvider.getImage(object) : null;
   }

   /**
    * Convert property raw value to text for display.
    *
    * @param name property name
    * @param value raw property value
    * @return string representation of property value, decoded as needed
    */
   public String valueToText(String name, String value)
   {
      if (value == null)
         return "";

      AssetAttribute attribute = session.getAssetManagementSchema().get(name);
      if (attribute.getDataType() == AMDataType.OBJECT_REFERENCE)
      {
         long objectId = 0;
         try
         {
            objectId = Integer.parseInt(value);
         }
         catch(NumberFormatException e)
         {
            logger.warn("Filed to parse object ID", e);
         }
         AbstractObject object = session.findObjectById(objectId);
         if (object != null)
            return objectLabelProvider.getText(object);
      }
      else if (attribute.getDataType() == AMDataType.ENUM)
      {
         String displayName = attribute.getEnumValues().get(value);
         return displayName == null || displayName.isBlank() ? value : displayName;
      }
      else if (attribute.getDataType() == AMDataType.DATE)
      {
         final Calendar c = Calendar.getInstance();
         SimpleDateFormat dateFormat = new SimpleDateFormat("yyyyMMdd");
         try
         {
            c.setTime(dateFormat.parse(value));
         }
         catch(ParseException e)
         {
            logger.error("Cannot parse date", e);
            return "<Error>";
         }
         return DateFormatFactory.getDateFormat().format(c.getTime());
      }

      return value;
   }

   /**
    * Get name of the system type
    * 
    * @param name property name
    * @return system type name
    */
   public String getSystemType(String name)
   {
      return AssetAttributeListLabelProvider.SYSTEM_TYPE[session.getAssetManagementSchema().get(name).getSystemType().getValue()];
   }

   /**
    * Text representation if value is unique
    * 
    * @param name property name
    * @return Text representation if value is unique
    */
   public String isUnique(String name)
   {
      return session.getAssetManagementSchema().get(name).isUnique() ? i18n.tr("Yes") : i18n.tr("No");
   }

   /**
    * Text representation if value is mandatory
    * 
    * @param name property name
    * @return Text representation if value is mandatory
    */
   public String isMandatory(String name)
   {
      return session.getAssetManagementSchema().get(name).isMandatory() ? i18n.tr("Yes") : i18n.tr("No");
   }

   /**
    * Get display name for given property.
    *
    * @param name property name
    * @return display name
    */
   public String getDisplayName(String name)
   {
      return session.getAssetManagementSchema().get(name).getEffectiveDisplayName();
   }

   /**
    * Dispose reader
    */
   public void dispose()
   {
      objectLabelProvider.dispose();
   }
}
