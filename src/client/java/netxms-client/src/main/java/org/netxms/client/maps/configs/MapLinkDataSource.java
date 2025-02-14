package org.netxms.client.maps.configs;

import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.LinkDataLocation;

public class MapLinkDataSource extends MapDataSource
{
   protected boolean system = false; 
   protected LinkDataLocation location = LinkDataLocation.CENTER; 


   /**
    * Default constructor
    */
   public MapLinkDataSource()
   { 
      super();
   }

   /**
    * Copy constructor
    * 
    * @param src source object
    */
   public MapLinkDataSource(MapLinkDataSource src)
   {
      super(src);
      this.system = src.system;
      this.location = src.location;
   }  
   


   /**
    * Create DCI info from DciValue object
    * 
    * @param dci source DciValue object
    */
   public MapLinkDataSource(DciValue dci)
   {
      super(dci);
   }

   /**
    * Create DCI info for node/DCI ID pair
    *
    * @param nodeId node ID
    * @param dciId DCI ID
    */
   public MapLinkDataSource(long nodeId, long dciId)
   {
      super(nodeId, dciId);
   }

   /**
    * Get current location on link
    * 
    * @return location on link
    */
   public LinkDataLocation getLocation()
   {
      return location == null ? LinkDataLocation.CENTER : location;
   }

   /**
    * Set current location on link
    * 
    * @param location to set
    */
   public void setLocation(LinkDataLocation location)
   {
      this.location = location;
   }

   public boolean isSystem()
   {
      return system;
   }

   public void setSystem(boolean system)
   {
      this.system = system;
   }
}
