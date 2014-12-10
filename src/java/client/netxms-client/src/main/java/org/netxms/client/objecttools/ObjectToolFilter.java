package org.netxms.client.objecttools;

import java.io.StringWriter;
import java.io.Writer;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Class created to store 
 */
@Root(name="objectToolFilter")
public class ObjectToolFilter
{
   @Element(required=false)
   public String toolOS;
   
   @Element(required=false)
   public String toolTemplate;
   
   @Element(required=false)
   public String snmpOid;
   
   /**
    * Create ObjectToolFilter object from XML document
    * 
    * @param xml XML document
    * @return deserialized object
    * @throws Exception if the object cannot be fully deserialized
    */
   public static ObjectToolFilter createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(ObjectToolFilter.class, xml);
   }
   
   /**
    * Create XML from configuration.
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      Serializer serializer = new Persister();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }
   
   public ObjectToolFilter()
   {
      toolOS = "";
      toolTemplate = "";
      snmpOid = "";
   }
}
