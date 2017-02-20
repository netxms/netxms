/**
 * 
 */
package org.netxms.websvc.json.adapters;

import java.io.IOException;
import java.util.Date;
import com.google.gson.TypeAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonToken;
import com.google.gson.stream.JsonWriter;

/**
 * Type adapter for Date class
 */
public class DateAdapter extends TypeAdapter<Date>
{
   /* (non-Javadoc)
    * @see com.google.gson.TypeAdapter#write(com.google.gson.stream.JsonWriter, java.lang.Object)
    */
   @Override
   public void write(JsonWriter writer, Date value) throws IOException
   {
      if (value == null)
      {
         writer.nullValue();
         return;
      }
      writer.value(value.getTime() / 1000);
   }

   /* (non-Javadoc)
    * @see com.google.gson.TypeAdapter#read(com.google.gson.stream.JsonReader)
    */
   @Override
   public Date read(JsonReader reader) throws IOException
   {
      if (reader.peek() == JsonToken.NULL)
      {
         reader.nextNull();
         return null;
      }

      try
      {
         return new Date(reader.nextLong() * 1000);
      }
      catch(NumberFormatException e)
      {
         return null;
      }
      catch(IllegalStateException e)
      {
         return null;
      }
   }
}
