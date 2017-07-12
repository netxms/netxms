/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.netxms.base.Glob;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
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
      
      List<AbstractObject> objects = session.getAllObjects();
      
      String areaFilter = query.get("area");
      String classFilter = query.get("class");
      String nameFilter = query.get("name");
      
      Map<String, String> customAttributes = null;
      for(String k : query.keySet())
      {
         if (!k.startsWith("@"))
            continue;
         
         if (customAttributes == null)
            customAttributes = new HashMap<String, String>();
         customAttributes.put(k.substring(1), query.get(k));
      }
      
      if ((areaFilter != null) || (classFilter != null) || (customAttributes != null) || (nameFilter != null))
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
         
         Iterator<AbstractObject> it = objects.iterator();
         while(it.hasNext())
         {
            AbstractObject o = it.next();
            
            // Filter by name
            if ((nameFilter != null) && !nameFilter.isEmpty() && !Glob.matchIgnoreCase(nameFilter, o.getObjectName()))
            {
               it.remove();
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
               {
                  it.remove();
                  continue;
               }
            }
            
            // Filter by geographical area
            if (area != null)
            {
               if (!o.getGeolocation().isWithinArea(area[0], area[1], area[2], area[3]))
               {
                  it.remove();
                  continue;
               }
            }
            
            // Filter by custom attribute
            if (customAttributes != null)
            {
               boolean match = true;
               for(Entry<String, String> e : customAttributes.entrySet())
               {
                  String value = o.getCustomAttributes().get(e.getKey());
                  if ((value == null) || !Glob.matchIgnoreCase(e.getValue(), value))
                  {
                     match = false;
                     break;
                  }
               }
               if (!match)
               {
                  it.remove();
                  continue;
               }
            }
         }
      }

      return new ResponseContainer("objects", objects);
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
