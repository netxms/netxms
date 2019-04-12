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
package org.netxms.websvc.json.adapters;

import java.io.IOException;
import org.netxms.base.MacAddress;
import org.netxms.base.MacAddressFormatException;
import com.google.gson.TypeAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonToken;
import com.google.gson.stream.JsonWriter;

/**
 * Type adapter for MacAddress class
 */
public class MacAddressAdapter extends TypeAdapter<MacAddress>
{
   /* (non-Javadoc)
    * @see com.google.gson.TypeAdapter#write(com.google.gson.stream.JsonWriter, java.lang.Object)
    */
   @Override
   public void write(JsonWriter writer, MacAddress value) throws IOException
   {
      if (value == null)
      {
         writer.nullValue();
         return;
      }
      writer.value(value.toString());
   }

   /* (non-Javadoc)
    * @see com.google.gson.TypeAdapter#read(com.google.gson.stream.JsonReader)
    */
   @Override
   public MacAddress read(JsonReader reader) throws IOException
   {
      if (reader.peek() == JsonToken.NULL)
      {
         reader.nextNull();
         return null;
      }

      try
      {
         return MacAddress.parseMacAddress(reader.nextString());
      }
      catch(MacAddressFormatException e)
      {
         return null;
      }
   }
}
