package org.netxms.client.maps.configs;

import org.netxms.client.datacollection.DciValue;

public class MapImageDataSource extends MapDataSource
{
   protected String name;


   /**
    * Default constructor
    */
   public MapImageDataSource()
   { 
      super();
      name = "";
   }

   /**
    * Copy constructor
    * 
    * @param src source object
    */
   public MapImageDataSource(MapImageDataSource src)
   {
      super(src);
      this.name = src.name;
   }  
   


   /**
    * Create DCI info from DciValue object
    * 
    * @param dci source DciValue object
    */
   public MapImageDataSource(DciValue dci)
   {
      super(dci);
      name = dci.getDescription();
   }

   /**
    * Create DCI info for node/DCI ID pair
    *
    * @param nodeId node ID
    * @param dciId DCI ID
    */
   public MapImageDataSource(long nodeId, long dciId)
   {
      super(nodeId, dciId);
      name = "";
   }

   /**
    * Get DCI name. Always returns non-empty string.
    * 
    * @return DCI name
    */
   public String getName()
   {
      return ((name != null) && !name.isEmpty()) ? name : ("[" + Long.toString(dciId) + "]");
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }
}
