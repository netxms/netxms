/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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

import org.json.JSONArray;
import org.json.JSONObject;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.websvc.ObjectToolExecutor;
import org.netxms.websvc.ObjectToolOutputListener;
import org.netxms.websvc.json.ResponseContainer;
import org.restlet.data.MediaType;
import org.restlet.ext.json.JsonRepresentation;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Post;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;

public class ObjectTools extends AbstractObjectHandler
{
   private Logger log = LoggerFactory.getLogger(ObjectTools.class);

   @Override protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      AbstractObject object = getObject();
      if (object.getObjectClass() != AbstractObject.OBJECT_NODE)
         throw new NXCException(RCC.INVALID_OBJECT_ID);

      List<ObjectTool> objectTools = session.getObjectTools();
      List<ObjectTool> result = new ArrayList<ObjectTool>();
      for(ObjectTool t : objectTools)
         if (t.isApplicableForNode((AbstractNode)object))
            result.add(t);

      return new ResponseContainer("objectTools", result);
   }

   @Post
   public Representation onPost(Representation entity) throws Exception
   {
      if (entity == null || !attachToSession())
      {
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }

      NXCSession session = getSession();
      AbstractObject object = getObject();

      if (object == null)
         return new StringRepresentation(createErrorResponse(RCC.INVALID_OBJECT_ID).toString(), MediaType.APPLICATION_JSON);
      JSONObject json = new JsonRepresentation(entity).getJsonObject();

      if (json.has("toolList"))
      {
         JSONArray toolList = json.getJSONArray("toolList");

         for(int i = 0; i < toolList.length(); i++)
         {
            Map<String, String> fields = new HashMap<String, String>();
            JSONObject tool = toolList.getJSONObject(i);

            int id = tool.getInt("id");
            if (tool.optJSONArray("inputFields") != null)
            {
               JSONArray inputFields = tool.getJSONArray("inputFields");

               for(int n = 0; n < inputFields.length(); n++)
               {
                  String field = inputFields.getString(n);
                  String[] pair = field.split(";");
                  if (pair != null && pair.length == 2)
                  {
                     fields.put(pair[1], pair[0]);
                  }
               }
            }

            ObjectToolDetails details = session.getObjectToolDetails(id);
            if (((details.getFlags() & ObjectTool.GENERATES_OUTPUT) != 0))
            {
               ObjectToolOutputListener listener = new ObjectToolOutputListener();
               UUID uuid = UUID.randomUUID();
               ObjectToolOutputHandler.addListener(uuid, listener);
               new ObjectToolExecutor(details, object.getObjectId(), fields, listener, session);

               JSONObject response = new JSONObject();
               response.put("UUID", uuid);
               return new StringRepresentation(response.toString(), MediaType.APPLICATION_JSON);
            }
            else
               return new StringRepresentation(createErrorResponse(RCC.SUCCESS).toString(), MediaType.APPLICATION_JSON);
         }
      }

      return new StringRepresentation(createErrorResponse(RCC.INTERNAL_ERROR).toString(), MediaType.APPLICATION_JSON);
   }
}
