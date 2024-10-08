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
package org.netxms.websvc.handlers;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.regex.Pattern;
import org.json.JSONObject;
import org.netxms.base.Glob;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.maps.MapType;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.NetworkService;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.VPNConnector;
import org.netxms.websvc.WebSvcException;
import org.netxms.websvc.json.JsonTools;
import org.netxms.websvc.json.ResponseContainer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Objects request handler
 */
public class Objects extends AbstractObjectHandler
{
   private Logger log = LoggerFactory.getLogger(AbstractHandler.class);

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      if (!session.areObjectsSynchronized())
         session.syncObjects();

      boolean topLevelOnly = (query.get("topLevelOnly") != null) ? Boolean.parseBoolean(query.get("topLevelOnly")) : false;
      List<AbstractObject> objects = topLevelOnly ? Arrays.asList(session.getTopLevelObjects()) : session.getAllObjects();

      boolean useRegex = (query.get("regex") != null) ? Boolean.parseBoolean(query.get("regex")) : false;

      String areaFilter = query.get("area");
      String classFilter = query.get("class");
      String nameFilter = query.get("name");
      String parentFilter = query.get("parent");
      String primaryNameFilter = query.get("primaryName");
      String stateFilter = query.get("state");
      String zoneFilter = query.get("zone");

      Pattern nameFilterRegex = null;
      if ((nameFilter != null) && !nameFilter.isEmpty())
         nameFilterRegex = Pattern.compile(nameFilter, Pattern.CASE_INSENSITIVE);

      Pattern primaryNameFilterRegex = null;
      if ((primaryNameFilter != null) && !primaryNameFilter.isEmpty())
         primaryNameFilterRegex = Pattern.compile(primaryNameFilter, Pattern.CASE_INSENSITIVE);

      Integer stateFilterValue = null;
      if ((stateFilter != null) && !stateFilter.isEmpty())
      {
         try
         {
            stateFilterValue = stateFilter.startsWith("0x") ? Integer.parseInt(stateFilter.substring(2), 16) : Integer.parseInt(stateFilter);
         }
         catch(NumberFormatException e)
         {
            log.debug("Invalid state filter " + stateFilter);
         }
      }
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

      if ((areaFilter != null) || (classFilter != null) || (customAttributes != null) || (nameFilter != null) ||
          (parentFilter != null) || (primaryNameFilter != null) || (stateFilterValue != null) || (zoneFilter != null))
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
            // Filter by state
            if (stateFilterValue != null)
            {
               if (!(o instanceof AbstractNode))
                  continue; // FIXME: check state for all objects
               if ((stateFilterValue & ((AbstractNode)o).getStateFlags()) == 0)
                  continue;
            }

            // Filter by zone
            if (zones != null)
            {
               long zoneUin = getZoneUin(o);
               if (zoneUin == -1)
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
                     String value = o.getCustomAttributeValue(e.getKey());
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
                     String value = o.getCustomAttributeValue(e.getKey());
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

            // Filter by primary name
            if (primaryNameFilter != null)
            {
               // If we want to filter by primary name this implies that we are only looking
               // for nodes.
               if (o instanceof AbstractNode)
               {
                  AbstractNode node = (AbstractNode)o;
                  if (useRegex)
                  {
                     if ((primaryNameFilterRegex != null) && !primaryNameFilterRegex.matcher(node.getPrimaryName()).matches())
                        continue;
                  }
                  else
                  {
                     if ((primaryNameFilter != null) && !primaryNameFilter.isEmpty() && !Glob.matchIgnoreCase(primaryNameFilter, node.getPrimaryName()))
                        continue;
                  }
               }
               else
               {
                  continue;
               }
            }

            filteredObjects.add(o);
         }
         objects = filteredObjects;
      }

      return new ResponseContainer("objects", objects);
   }

   /**
    * Get zoneUIN depending on object class
    * 
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

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String, java.util.Map)
    */
   @Override
   protected Object get(String id, Map<String, String> query) throws Exception
   {
      return getObject();
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getEntityIdFieldName()
    */
   @Override
   protected String getEntityIdFieldName()
   {
      return "object-id";
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#create(org.json.JSONObject)
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {
      int type = JsonTools.getIntFromJson(data, "objectType", -1);
      String name = JsonTools.getStringFromJson(data, "name", null);
      long parentId = JsonTools.getLongFromJson(data, "parentId", -1);
      if ((type == -1) || (name == null) || (parentId == -1))
         return createErrorResponseRepresentation(RCC.INVALID_ARGUMENT);

      NXCObjectCreationData createData = new NXCObjectCreationData(type, name, parentId);
      createData.setComments(JsonTools.getStringFromJson(data, "comments", createData.getComments()));
      createData.setCreationFlags(JsonTools.getIntFromJson(data, "creationFlags", createData.getFlags()));
      createData.setMapType(MapType.getByValue(JsonTools.getIntFromJson(data, "mapType", createData.getMapType().getValue())));
      createData.setZoneUIN(JsonTools.getIntFromJson(data, "zoneUIN", createData.getZoneUIN()));
      createData.setLinkedNodeId(JsonTools.getLongFromJson(data, "linkedNodeId", createData.getLinkedNodeId()));
      createData.setTemplate(JsonTools.getBooleanFromJson(data, "template", createData.isTemplate()));
      createData.setIfIndex(JsonTools.getIntFromJson(data, "ifIndex", createData.getIfIndex()));
      createData.setIfType(JsonTools.getIntFromJson(data, "ifType", createData.getIfType()));
      createData.setChassis(JsonTools.getIntFromJson(data, "chassis", createData.getChassis()));
      createData.setModule(JsonTools.getIntFromJson(data, "module", createData.getModule()));
      createData.setPIC(JsonTools.getIntFromJson(data, "pic", createData.getPIC()));
      createData.setPort(JsonTools.getIntFromJson(data, "port", createData.getPort()));
      createData.setPhysicalPort(JsonTools.getBooleanFromJson(data, "physicalPort", createData.isPhysicalPort()));
      createData.setCreateStatusDci(JsonTools.getBooleanFromJson(data, "createStatusDci", createData.isCreateStatusDci()));
      createData.setDeviceId(JsonTools.getStringFromJson(data, "deviceId", createData.getDeviceId()));
      createData.setFlags(JsonTools.getIntFromJson(data, "flags", createData.getFlags()));
      createData.setInstanceDiscoveryMethod(JsonTools.getIntFromJson(data, "instanceDiscoveryMethod", createData.getInstanceDiscoveryMethod()));
      createData.setDeviceAddress(JsonTools.getStringFromJson(data, "deviceAddress", createData.getDeviceAddress()));
      createData.setVendor(JsonTools.getStringFromJson(data, "vendor", createData.getVendor()));
      createData.setModel(JsonTools.getStringFromJson(data, "model", createData.getModel()));
      createData.setSerialNumber(JsonTools.getStringFromJson(data, "serialNumber", createData.getSerialNumber()));
      createData.setModbusUnitId((short)JsonTools.getIntFromJson(data, "modbusUnitId", createData.getModbusUnitId()));

      NXCObjectModificationData mdObject = JsonTools.createGsonInstance().fromJson(data.toString(), NXCObjectModificationData.class);
      createData.updateFromModificationData(mdObject);

      NXCSession session = getSession();
      long nodeId = session.createObject(createData);
      mdObject.setObjectId(nodeId);
      session.modifyObject(mdObject);

      JSONObject result = new JSONObject();
      result.put("id", nodeId);
      return result;
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#update(java.lang.String, org.json.JSONObject)
    */
   @Override
   protected Object update(String id, JSONObject data) throws Exception
   {
      NXCSession session = getSession();
      NXCObjectModificationData mdObject = JsonTools.createGsonInstance().fromJson(data.toString(), NXCObjectModificationData.class);
      mdObject.setObjectId(getObjectId());
      session.modifyObject(mdObject);
      return null;
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#delete(java.lang.String)
    */
   @Override
   protected Object delete(String id) throws Exception
   {
      NXCSession session = getSession();
      session.deleteObject(getObjectId());
      return null;
   }
}
