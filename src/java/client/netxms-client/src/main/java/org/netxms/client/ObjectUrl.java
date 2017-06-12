/**
 * 
 */
package org.netxms.client;

import java.net.MalformedURLException;
import java.net.URL;
import org.netxms.base.NXCPMessage;

/**
 * URL associated with object
 */
public class ObjectUrl
{
   private int id;
   private URL url;
   private String description;
   
   /**
    * Create from NXCP message
    * 
    * @param msg The NXCPMessage
    * @param baseId The base ID
    */
   public ObjectUrl(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      try
      {
         url = new URL(msg.getFieldAsString(baseId + 1));
      }
      catch(MalformedURLException e)
      {
         url = null;
      }
      description = msg.getFieldAsString(baseId + 2);
   }

   /**
    * Create new object URL
    * 
    * @param id The ID of url
    * @param url The url
    * @param description The description
    */
   public ObjectUrl(int id, URL url, String description)
   {
      this.id = id;
      this.url = url;
      this.description = description;
   }

   /**
    * Fill NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setFieldInt32(baseId, id);
      msg.setField(baseId + 1, (url != null) ? url.toExternalForm() : "");
      msg.setField(baseId + 2, description);
   }
   
   /**
    * @return the id
    */
   public int getId()
   {
      return id;
   }

   /**
    * @return the url
    */
   public URL getUrl()
   {
      return url;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }
}
