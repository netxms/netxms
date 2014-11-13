package org.netxms.client.agent.config;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

public class ConfigContent
{
   private long id;
   private String config;
   private String filter;
   private String name;
   private long sequenceNumber;
   
   /**
    * Default constructior
    */
   public ConfigContent()
   {
      id = 0;
      config = "";
      filter = "";
      name = "New config";
      sequenceNumber = -1;
   }
   
   /**
    * Constructs object from message
    */
   public ConfigContent(long id, NXCPMessage response)
   {
      this.id = id;
      name = response.getVariableAsString(NXCPCodes.VID_NAME) == null ? "" : response.getVariableAsString(NXCPCodes.VID_NAME);
      config = response.getVariableAsString(NXCPCodes.VID_CONFIG_FILE) == null ? "" : response.getVariableAsString(NXCPCodes.VID_CONFIG_FILE);
      filter = response.getVariableAsString(NXCPCodes.VID_FILTER) == null ? "" : response.getVariableAsString(NXCPCodes.VID_FILTER);
      sequenceNumber = response.getVariableAsInt64(NXCPCodes.VID_SEQUENCE_NUMBER);
   }
   
   public void fillMessage(NXCPMessage request)
   {
      request.setVariableInt32(NXCPCodes.VID_CONFIG_ID, (int)id);
      request.setVariable(NXCPCodes.VID_NAME, name);
      request.setVariable(NXCPCodes.VID_CONFIG_FILE, config);
      request.setVariable(NXCPCodes.VID_FILTER, filter);
      request.setVariableInt32(NXCPCodes.VID_SEQUENCE_NUMBER, (int)sequenceNumber);
   }
   
   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }
   
   /**
    * @param id the id to set
    */
   public void setId(long id)
   {
      this.id = id;
   }

   /**
    * @return the config
    */
   public String getConfig()
   {
      return config;
   }

   /**
    * @param config the config to set
    */
   public void setConfig(String config)
   {
      this.config = config;
   }

   /**
    * @return the filter
    */
   public String getFilter()
   {
      return filter;
   }

   /**
    * @param filter the filter to set
    */
   public void setFilter(String filter)
   {
      this.filter = filter;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @return the sequenceNumber
    */
   public long getSequenceNumber()
   {
      return sequenceNumber;
   }

   /**
    * @param sequenceNumber the sequenceNumber to set
    */
   public void setSequenceNumber(long sequenceNumber)
   {
      this.sequenceNumber = sequenceNumber;
   }  
   
}
