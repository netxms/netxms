/**
 * 
 */
package org.netxms.client.objecttools;

import java.io.StringWriter;
import java.io.Writer;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Additional options for input field
 */
@Root(name="options", strict=false)
public class InputFieldOptions
{
   @Element(required=false)
   public boolean validatePassword = false;

   /**
    * Default constructor
    */
   public InputFieldOptions()
   {
   }

   /**
    * Copy constructor
    * 
    * @param src
    */
   public InputFieldOptions(InputFieldOptions src)
   {
      validatePassword = src.validatePassword;
   }

   /**
    * Create options object from XML
    * 
    * @param xml XML document
    * @return input field options object
    */
   public static InputFieldOptions createFromXml(String xml)
   {
      try
      {
         Serializer serializer = new Persister();
         return serializer.read(InputFieldOptions.class, xml);
      }
      catch(Exception e)
      {
         return new InputFieldOptions();
      }
   }

   /**
    * Create XML from configuration.
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml()
   {
      try
      {
         Serializer serializer = new Persister();
         Writer writer = new StringWriter();
         serializer.write(this, writer);
         return writer.toString();
      }
      catch(Exception e)
      {
         return null;
      }
   }
}
