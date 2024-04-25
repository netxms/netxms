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
package org.netxms.websvc.json.adapters;

import java.io.IOException;
import org.netxms.client.NXCSession;
import com.google.gson.TypeAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonWriter;

/**
 * Type adapter for NXCSession class
 */
public class NXCSessionAdapter extends TypeAdapter<NXCSession>
{
   /**
    * @see com.google.gson.TypeAdapter#write(com.google.gson.stream.JsonWriter, java.lang.Object)
    */
   @Override
   public void write(JsonWriter writer, NXCSession value) throws IOException
   {
      if (value == null)
      {
         writer.nullValue();
         return;
      }

      writer.beginObject();

      writer.name("server");
      writer.beginObject();

      writer.name("address");
      writer.value(value.getServerAddress());

      writer.name("serverName");
      writer.value(value.getServerName());
      
      writer.name("version");
      writer.value(value.getServerVersion());
      
      writer.name("color");
      writer.value(value.getServerColor());
      
      writer.name("id");
      writer.value(value.getServerId());

      writer.name("timeZone");
      writer.value(value.getServerTimeZone());

      writer.endObject();

      writer.name("user");
      writer.beginObject();
      
      writer.name("name");
      writer.value(value.getUserName());
      
      writer.name("id");
      writer.value(value.getUserId());
      
      writer.name("globalAccessRights");
      writer.value(value.getUserSystemRights());
      
      writer.endObject();
      
      writer.name("objectsSynchronized");
      writer.value(value.areObjectsSynchronized());
      
      writer.name("passwordExpired");
      writer.value(value.isPasswordExpired());
      
      writer.name("zoningEnabled");
      writer.value(value.isZoningEnabled());
      
      writer.endObject();
   }

   /**
    * @see com.google.gson.TypeAdapter#read(com.google.gson.stream.JsonReader)
    */
   @Override
   public NXCSession read(JsonReader reader) throws IOException
   {
      // NXCSession cannot be constructed from JSON
      return null;
   }
}
