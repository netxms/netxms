/**
 * 
 */
package org.netxms.websvc.json.adapters;

import java.io.IOException;
import org.netxms.client.MacAddress;
import org.netxms.client.MacAddressFormatException;
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
