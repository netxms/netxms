package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.io.StringWriter;
import java.io.UnsupportedEncodingException;
import java.io.Writer;
import org.apache.commons.codec.binary.Base64;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

@Root(name="SupportAppPolicy", strict=false)
public class SupportAppPolicy
{
   @Element(required=false)
   public boolean closeOnDeactivate = false;

   @Element(required=false)
   public Integer mainWindowPosition = null;
   
   @Element(required=false)
   public boolean customColorSchema = false;

   @Element(required=false)
   public Integer backgroundColor = 0x303030;
   
   @Element(required=false)
   public Integer textColor = 0xC0C0C0;
   
   @Element(required=false)
   public Integer highlightColor = 0x2020C0;
   
   @Element(required=false)
   public Integer borderColor = 0xC0C0C0;
   
   @Element(required=false)
   public Integer menuBackgroundColor = 0x303030;
   
   @Element(required=false)
   public Integer menuHighligtColor = 0x505050;
   
   @Element(required=false)
   public Integer menuSelectionColor = 0x0069D2;
   
   @Element(required=false)
   public Integer menuTextColor = 0xF0F0F0;
   
   @Element(required=false)
   public String icon;
   
   @Element(required=false)
   public String welcomeMessage;

   @Element(required=false)
   public AppMenuItem menu = new AppMenuItem("[root]", "", null);
   
   /**
    * Create support application policy from XML
    * 
    * @param xml configuration
    * @return attribute support application policy
    * @throws Exception if deserialization fails
    */
   public static SupportAppPolicy createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      SupportAppPolicy obj = serializer.read(SupportAppPolicy.class, xml);
      obj.updateMenuParents();
      return obj;
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
   
   /**
    * Set application icon
    * 
    * @param content application icon file content (in ICO format)
    */
   public void setIcon(byte[] content)
   {
      if (content != null)
      {
         try
         {
            icon = new String(Base64.encodeBase64(content), "ISO-8859-1");
         }
         catch(UnsupportedEncodingException e)
         {
            icon = null;
         }
      }
      else
      {
         icon = null;
      }
   }
	
   /**
    * Get application icon file content (in ICO format)
    * 
    * @return application icon file content (in ICO format)
    */
   public byte[] getIcon() 
   {
      if (icon == null)
         return null;
      
      try
      {
         return Base64.decodeBase64(icon.getBytes("ISO-8859-1"));
      }
      catch(UnsupportedEncodingException e)
      {
         return null;
      }
   }

   /**
    * Update parent references in menu items
    */
   public void updateMenuParents()
   {
      menu.updateParents(null);
   }
}
