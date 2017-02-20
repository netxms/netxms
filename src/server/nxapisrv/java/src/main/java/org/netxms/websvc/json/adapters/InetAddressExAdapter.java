/**
 * 
 */
package org.netxms.websvc.json.adapters;

import java.io.IOException;
import java.net.InetAddress;
import org.netxms.base.InetAddressEx;
import com.google.gson.TypeAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonToken;
import com.google.gson.stream.JsonWriter;

/**
 * Type adapter for InetAddressEx class
 */
public class InetAddressExAdapter extends TypeAdapter<InetAddressEx>
{
   /* (non-Javadoc)
    * @see com.google.gson.TypeAdapter#write(com.google.gson.stream.JsonWriter, java.lang.Object)
    */
   @Override
   public void write(JsonWriter writer, InetAddressEx value) throws IOException
   {
      if (value == null)
      {
         writer.nullValue();
         return;
      }
      writer.value(value.getHostAddress());
   }

   /* (non-Javadoc)
    * @see com.google.gson.TypeAdapter#read(com.google.gson.stream.JsonReader)
    */
   @Override
   public InetAddressEx read(JsonReader reader) throws IOException
   {
      if (reader.peek() == JsonToken.NULL)
      {
         reader.nextNull();
         return null;
      }

      try
      {
         String value = reader.nextString();
         String[] parts = value.split("/");
         InetAddress addr = InetAddress.getByName(parts[0]);
         return (parts.length == 1) ? new InetAddressEx(addr) : new InetAddressEx(addr, Integer.parseInt(parts[1]));
      }
      catch(Exception e)
      {
         return null;
      }
   }
}
