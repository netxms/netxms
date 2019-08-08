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

import org.netxms.base.VersionInfo;
import org.netxms.websvc.json.JsonTools;
import org.restlet.data.MediaType;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Get;
import org.restlet.resource.ServerResource;
import com.google.gson.JsonObject;

/**
 * Handler for API information
 */
public class InfoHandler extends ServerResource
{
   private static final String API_VERSION = "2";
   
   /**
    * Process GET requests
    * 
    * @return
    */
   @Get
   public Representation onGet() throws Exception
   {
      JsonObject response = new JsonObject();
      response.addProperty("apiVersion", API_VERSION);
      response.addProperty("build", VersionInfo.buildTag());
      response.addProperty("clientVersion", VersionInfo.version());
      return new StringRepresentation(JsonTools.jsonFromObject(response, null), MediaType.APPLICATION_JSON);
   }
}
