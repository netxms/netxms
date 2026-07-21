/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.datacollection;

import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * NETCONF query definition
 */
public class NetconfQueryDefinition
{
   public static final int DATASTORE_OPERATIONAL = 0;
   public static final int DATASTORE_RUNNING     = 1;
   public static final int DATASTORE_CANDIDATE   = 2;
   public static final int DATASTORE_STARTUP     = 3;

   public static final int FILTER_NONE    = 0;
   public static final int FILTER_SUBTREE = 1;
   public static final int FILTER_XPATH   = 2;

   private int id;
   private UUID guid;
   private String name;
   private String description;
   private int datastore;
   private int filterType;
   private String filter;
   private int cacheRetentionTime;
   private int requestTimeout;
   private int flags;

   /**
    * Create new definition. Definition object will be created with random GUID and ID 0.
    *
    * @param name name for new definition
    */
   public NetconfQueryDefinition(String name)
   {
      id = 0;
      guid = UUID.randomUUID();
      this.name = name;
      description = "";
      datastore = DATASTORE_OPERATIONAL;
      filterType = FILTER_NONE;
      filter = "";
      cacheRetentionTime = 0;
      requestTimeout = 0;
      flags = 0;
   }

   /**
    * Create definition from NXCP message
    *
    * @param msg NXCP message
    */
   public NetconfQueryDefinition(NXCPMessage msg)
   {
      id = msg.getFieldAsInt32(NXCPCodes.VID_QUERY_ID);
      guid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
      name = msg.getFieldAsString(NXCPCodes.VID_NAME);
      description = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
      datastore = msg.getFieldAsInt32(NXCPCodes.VID_DATASTORE);
      filterType = msg.getFieldAsInt32(NXCPCodes.VID_FILTER_TYPE);
      filter = msg.getFieldAsString(NXCPCodes.VID_FILTER);
      cacheRetentionTime = msg.getFieldAsInt32(NXCPCodes.VID_RETENTION_TIME);
      requestTimeout = msg.getFieldAsInt32(NXCPCodes.VID_TIMEOUT);
      flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
   }

   /**
    * Fill NXCP message with object data
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldInt32(NXCPCodes.VID_QUERY_ID, id);
      msg.setField(NXCPCodes.VID_GUID, guid);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setFieldInt16(NXCPCodes.VID_DATASTORE, datastore);
      msg.setFieldInt16(NXCPCodes.VID_FILTER_TYPE, filterType);
      msg.setField(NXCPCodes.VID_FILTER, filter);
      msg.setFieldInt32(NXCPCodes.VID_RETENTION_TIME, cacheRetentionTime);
      msg.setFieldInt32(NXCPCodes.VID_TIMEOUT, requestTimeout);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
   }

   /**
    * Get NETCONF query definition ID.
    *
    * @return NETCONF query definition ID
    */
   public int getId()
   {
      return id;
   }

   /**
    * Set NETCONF query definition ID.
    *
    * @param id new NETCONF query definition ID
    */
   public void setId(int id)
   {
      this.id = id;
   }

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * @param guid the guid to set
    */
   public void setGuid(UUID guid)
   {
      this.guid = guid;
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
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @param description the description to set
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * Get datastore to be queried (one of <code>DATASTORE_</code> constants).
    *
    * @return datastore to be queried
    */
   public int getDatastore()
   {
      return datastore;
   }

   /**
    * Set datastore to be queried (one of <code>DATASTORE_</code> constants).
    *
    * @param datastore datastore to be queried
    */
   public void setDatastore(int datastore)
   {
      this.datastore = datastore;
   }

   /**
    * Get filter type (one of <code>FILTER_</code> constants).
    *
    * @return filter type
    */
   public int getFilterType()
   {
      return filterType;
   }

   /**
    * Set filter type (one of <code>FILTER_</code> constants).
    *
    * @param filterType new filter type
    */
   public void setFilterType(int filterType)
   {
      this.filterType = filterType;
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
    * Get cache retention time in seconds.
    *
    * @return the cacheRetentionTime
    */
   public int getCacheRetentionTime()
   {
      return cacheRetentionTime;
   }

   /**
    * Set cache retention time in seconds.
    *
    * @param cacheRetentionTime the cacheRetentionTime to set
    */
   public void setCacheRetentionTime(int cacheRetentionTime)
   {
      this.cacheRetentionTime = cacheRetentionTime;
   }

   /**
    * Get request timeout in milliseconds.
    *
    * @return the requestTimeout
    */
   public int getRequestTimeout()
   {
      return requestTimeout;
   }

   /**
    * Set request timeout in milliseconds.
    *
    * @param requestTimeout the requestTimeout to set
    */
   public void setRequestTimeout(int requestTimeout)
   {
      this.requestTimeout = requestTimeout;
   }

   /**
    * Get flags.
    *
    * @return flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * Set flags.
    *
    * @param flags new flags
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
   }
}
