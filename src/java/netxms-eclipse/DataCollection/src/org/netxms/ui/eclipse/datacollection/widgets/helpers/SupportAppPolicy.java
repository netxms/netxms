package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.io.StringWriter;
import java.io.Writer;
import java.util.Base64;
import java.util.UUID;
import java.util.Base64.Encoder;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

@Root(name="SupportAppPolicy")
public class SupportAppPolicy
{
   @Element(required=false)
   public int backgroundColor;
   
   @Element(required=false)
   public int normalForegroundColor;
   
   @Element(required=false)
   public int highForegroundColor;
   
   @Element(required=false)
   public int menuBackgroundColor;
   
   @Element(required=false)
   public int menuForegroundColor;
   
   @Element(required=false)
   public String logo;
   
   @Element(required=false)
   public String welcomeMessage;
   
   @ElementArray(required = true)
   public GenericMenuItem menuItems[] = new GenericMenuItem[0];
   
   /**
    * Create rack attribute from config entry
    * 
    * @param xml config
    * @return attribute
    * @throws Exception
    */
   public static SupportAppPolicy createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(SupportAppPolicy.class, xml);
   }
   
   /**
    * Create XML from configuration entry
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
   
   public void setLogo(byte bs[])
   {
      Encoder enc = Base64.getEncoder();  
      logo = enc.encodeToString(bs);
   }

}
