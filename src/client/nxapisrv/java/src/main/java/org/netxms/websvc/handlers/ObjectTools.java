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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.json.JSONArray;
import org.json.JSONObject;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.client.objecttools.ObjectToolFolder;
import org.netxms.websvc.ObjectToolExecutor;
import org.netxms.websvc.ServerOutputListener;
import org.netxms.websvc.json.ResponseContainer;
import org.restlet.data.MediaType;
import org.restlet.ext.json.JsonRepresentation;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Post;

/**
 * Handler for object tool management
 */
public class ObjectTools extends AbstractObjectHandler
{
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      AbstractObject object = getObject();
      if (object.getObjectClass() != AbstractObject.OBJECT_NODE)
         throw new NXCException(RCC.INVALID_OBJECT_ID);

      List<ObjectTool> objectTools = session.getObjectTools();
      objectTools.removeIf(tool -> !tool.isApplicableForObject(object));

      return new ResponseContainer("root", createToolTree(objectTools));
   }

   /**
    * Create object tool tree
    *
    * @param tools list of object tools
    * @return the root folder of the tree
    */
   public ObjectToolFolder createToolTree(List<ObjectTool> tools)
   {
      ObjectToolFolder root = new ObjectToolFolder("[root]");
      Map<String, ObjectToolFolder> folders = new HashMap<String, ObjectToolFolder>();
      for(ObjectTool t : tools)
      {
         ObjectToolFolder folder = root;
         String[] path = t.getName().split("\\-\\>");
         for(int i = 0; i < path.length - 1; i++)
         {
            String key = folder.hashCode() + "@" + path[i].replace("&", "");
            ObjectToolFolder curr = folders.get(key);
            if (curr == null)
            {
               curr = new ObjectToolFolder(path[i]);
               folders.put(key, curr);
               folder.addFolder(curr);
            }
            folder = curr;
         }
         folder.addTool(t);
      }
      return root;
   }

   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#onPost(org.restlet.representation.Representation)
    */
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
      if (json.has("toolData"))
      {
         JSONObject toolData = json.getJSONObject("toolData");
         Map<String, String> fields = new HashMap<String, String>();
         List<String> maskedFields = new ArrayList<String>();

         int id = toolData.getInt("id");
         if (toolData.optJSONArray("inputFields") != null)
         {
            JSONArray inputFields = toolData.getJSONArray("inputFields");

            for(int n = 0; n < inputFields.length(); n++)
            {
               String field = inputFields.getString(n);
               String[] pair = field.split(";");
               if (pair.length == 2)
                  fields.put(pair[1], pair[0]);
            }
         }
         if (toolData.optJSONArray("maskedInputFields") != null)
         {
            JSONArray inputFields = toolData.getJSONArray("maskedInputFields");

            for(int n = 0; n < inputFields.length(); n++)
            {
               maskedFields.add(inputFields.getString(n));
            }
         }

         ObjectToolDetails details = session.getObjectToolDetails(id);
         if (((details.getFlags() & ObjectTool.GENERATES_OUTPUT) != 0))
         {
            ServerOutputListener listener = new ServerOutputListener();
            UUID uuid = UUID.randomUUID();
            ObjectToolOutputHandler.addListener(uuid, listener);
            new ObjectToolExecutor(details, object.getObjectId(), fields, maskedFields, listener, session);

            JSONObject response = new JSONObject();
            response.put("UUID", uuid);
            response.put("toolId", id);
            return new StringRepresentation(response.toString(), MediaType.APPLICATION_JSON);
         }
         else
            return new StringRepresentation(createErrorResponse(RCC.SUCCESS).toString(), MediaType.APPLICATION_JSON);
      }

      return new StringRepresentation(createErrorResponse(RCC.INTERNAL_ERROR).toString(), MediaType.APPLICATION_JSON);
   }
}
