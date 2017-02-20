/**
 * 
 */
package org.netxms.websvc.json.adapters;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import com.google.gson.TypeAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonToken;
import com.google.gson.stream.JsonWriter;

/**
 * Type adapter for InetAddress class
 */
public class InetAddressAdapter extends TypeAdapter<InetAddress>
{
   /* (non-Javadoc)
    * @see com.google.gson.TypeAdapter#write(com.google.gson.stream.JsonWriter, java.lang.Object)
    */
   @Override
   public void write(JsonWriter writer, InetAddress value) throws IOException
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
   public InetAddress read(JsonReader reader) throws IOException
   {
      if (reader.peek() == JsonToken.NULL)
      {
         reader.nextNull();
         return null;
      }

      try
      {
         return InetAddress.getByName(reader.nextString());
      }
      catch(UnknownHostException e)
      {
         return null;
      }
   }
}
