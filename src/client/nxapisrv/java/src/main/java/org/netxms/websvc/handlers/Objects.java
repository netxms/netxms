/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.websvc.handlers;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.regex.Pattern;
import org.netxms.base.Glob;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.NetworkService;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.VPNConnector;
import org.netxms.websvc.WebSvcException;
import org.netxms.websvc.json.ResponseContainer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Objects request handler
 */
public class Objects extends AbstractObjectHandler
{
   private Logger log = LoggerFactory.getLogger(AbstractHandler.class);
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(org.json.JSONObject)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      if (!session.isObjectsSynchronized())
         session.syncObjects();
      
      boolean topLevelOnly = (query.get("topLevelOnly") != null) ? Boolean.parseBoolean(query.get("topLevelOnly")) : false;
      List<AbstractObject> objects = topLevelOnly ? Arrays.asList(session.getTopLevelObjects()) : session.getAllObjects();

      boolean useRegex = (query.get("regex") != null) ? Boolean.parseBoolean(query.get("regex")) : false;
      
      String areaFilter = query.get("area");
      String classFilter = query.get("class");
      String nameFilter = query.get("name");
      String parentFilter = query.get("parent");
      String zoneFilter = query.get("zone");
      
      Pattern nameFilterRegex = null;
      if ((nameFilter != null) && !nameFilter.isEmpty())
         nameFilterRegex = Pattern.compile(nameFilter, Pattern.CASE_INSENSITIVE);

      Map<String, Object> customAttributes = null;
      for(String k : query.keySet())
      {
         if (!k.startsWith("@"))
            continue;
         
         if (customAttributes == null)
            customAttributes = new HashMap<String, Object>();
         if (useRegex)
            customAttributes.put(k.substring(1), Pattern.compile(query.get(k), Pattern.CASE_INSENSITIVE));
         else
            customAttributes.put(k.substring(1), query.get(k));
      }
      
      if ((areaFilter != null) || (classFilter != null) || (customAttributes != null) || (nameFilter != null) 
            || (parentFilter != null) || (zoneFilter != null))
      {
         double[] area = null;
         if (areaFilter != null)
         {
            String[] parts = areaFilter.split(",");
            if (parts.length == 4)
            {
               try
               {
                  area = new double[4];
                  for(int i = 0; i < 4; i++)
                     area[i] = Double.parseDouble(parts[i]);
               }
               catch(NumberFormatException e)
               {
                  log.warn("Invalid area filter " + areaFilter);
               }
            }
            else
            {
               log.warn("Invalid area filter " + areaFilter);
            }
         }

         String[] classes = null;
         if ((classFilter != null) && !classFilter.isEmpty())
         {
            classes = classFilter.split(",");
         }
         
         long parentId;
         if (parentFilter != null)
         {
            try
            {
               parentId = Long.parseLong(parentFilter);
            }
            catch(NumberFormatException e)
            {
               throw new WebSvcException(RCC.INVALID_ARGUMENT);
            }
         }
         else
         {
            parentId = 0;
         }

         long[] zones = null;
         if (zoneFilter != null)
         {
            String[] parts = zoneFilter.split(",");
            try
            {
               zones = new long[parts.length];
               for(int i = 0; i < parts.length; i++)
                  zones[i] = Long.parseLong(parts[i]);
            }
            catch(NumberFormatException e)
            {
               log.warn("Invalid zone filter " + zoneFilter);
            }
         }

         List<AbstractObject> filteredObjects = new ArrayList<AbstractObject>();
         for(AbstractObject o : objects)
         {
            // Filter by zone
            if (zones != null)
            {
               long zoneUin = getZoneUin(o);
               if(zoneUin == -1)
                  continue;
                  
               boolean match = false;
               for(long z : zones)
               {
                  if (zoneUin == z)
                  {
                     match = true;
                     break;
                  }
               }
               if (!match)
                  continue;
            }
            
            // Filter by name
            if (useRegex)
            {
               if ((nameFilterRegex != null) && !nameFilterRegex.matcher(o.getObjectName()).matches())
                  continue;
            }
            else
            {
               if ((nameFilter != null) && !nameFilter.isEmpty() && !Glob.matchIgnoreCase(nameFilter, o.getObjectName()))
                  continue;
            }
            
            // Filter by class
            if (classes != null)
            {
               boolean match = false;
               for(String c : classes)
               {
                  if (o.getObjectClassName().equalsIgnoreCase(c))
                  {
                     match = true;
                     break;
                  }
               }
               if (!match)
                  continue;
            }
            
            // Filter by parent
            if ((parentId != 0) && !o.isChildOf(parentId))
               continue;

            // Filter by geographical area
            if (area != null)
            {
               if (!o.getGeolocation().isWithinArea(area[0], area[1], area[2], area[3]))
                  continue;
            }
            
            // Filter by custom attribute
            if (customAttributes != null)
            {
               if (o.getCustomAttributes().isEmpty())
                  continue;
               boolean match = true;
               if (useRegex)
               {
                  for(Entry<String, Object> e : customAttributes.entrySet())
                  {
                     String value = o.getCustomAttributes().get(e.getKey());
                     if ((value == null) || !((Pattern)e.getValue()).matcher(value).matches())
                     {
                        match = false;
                        break;
                     }
                  }
               }
               else
               {
                  for(Entry<String, Object> e : customAttributes.entrySet())
                  {
                     String value = o.getCustomAttributes().get(e.getKey());
                     if ((value == null) || !Glob.matchIgnoreCase((String)e.getValue(), value))
                     {
                        match = false;
                        break;
                     }
                  }
               }
               if (!match)
                  continue;
            }
            filteredObjects.add(o);
         }
         objects = filteredObjects;
      }
      
      return new ResponseContainer("objects", objects);
   }
   
   /**
    * Get zoneUIN depending on object class
    * @param obj
    * @return
    */
   private static long getZoneUin(AbstractObject obj)
   {
      if (obj instanceof AbstractNode)
      {
         return ((AbstractNode)obj).getZoneId();
      }
      if (obj instanceof Subnet)
      {
         return ((Subnet)obj).getZoneId();
      }
      if (obj instanceof Interface)
      {
         return ((Interface)obj).getZoneId();
      }
      if (obj instanceof NetworkService)
      {
         return ((NetworkService)obj).getParentNode().getZoneId();
      }
      if (obj instanceof VPNConnector)
      {
         return ((VPNConnector)obj).getParentNode().getZoneId();
      }
      if (obj instanceof AccessPoint)
      {
         return ((AccessPoint)obj).getParentNode().getZoneId();
      }
      return -1;
   }

   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String)
    */
   @Override
   protected Object get(String id, Map<String, String> query) throws Exception
   {
      return getObject();
   }

   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getEntityIdFieldName()
    */
   @Override
   protected String getEntityIdFieldName()
   {
      return "object-id";
   }
}
