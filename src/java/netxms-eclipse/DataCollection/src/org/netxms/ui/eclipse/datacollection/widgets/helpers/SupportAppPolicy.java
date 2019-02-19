package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.io.StringWriter;
import java.io.UnsupportedEncodingException;
import java.io.Writer;
import org.apache.commons.codec.binary.Base64;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

@Root(name="SupportAppPolicy")
public class SupportAppPolicy
{
   @Element(required=false)
   public Integer backgroundColor;
   
   @Element(required=false)
   public Integer borderColor;
   
   @Element(required=false)
   public Integer headerColor;
   
   @Element(required=false)
   public Integer textColor;
   
   @Element(required=false)
   public Integer menuBackgroundColor;
   
   @Element(required=false)
   public Integer menuHighligtColor;
   
   @Element(required=false)
   public Integer menuSelectionColor;
   
   @Element(required=false)
   public Integer menuTextColor;
   
   @Element(required=false)
   public String logo;
   
   @Element(required=false)
   public String welcomeMessage;

   @Element(required=false)
   public FolderMenuItem root = new FolderMenuItem();
   
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
      SupportAppPolicy obj = serializer.read(SupportAppPolicy.class, xml);
      obj.updateParents();
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
   
   public void setLogo(byte bs[])
   {
      if(bs != null)
      {
         try
         {
            logo = new String(Base64.encodeBase64(bs, false), "ISO-8859-1");
         }
         catch(UnsupportedEncodingException e)
         {
            logo = null;
         }
      }
      else
         logo = null;
   }
	
   public byte[] getLogo() 
   {
      if(logo == null)
         return null;
      try
      {
         return Base64.decodeBase64(logo.getBytes("ISO-8859-1"));
      }
      catch(UnsupportedEncodingException e)
      {
         return null;
      }
   }

   public void updateParents()
   {
      root.updateParents(null);
   }

   public void deleteMenuItem(GenericMenuItem genericMenuItem)
   {
      genericMenuItem.delete();
   }
}
