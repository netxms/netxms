/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.datacollection;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.DciTier;

/**
 * Request for export of DCI history. Metrics are selected either by tag patterns under a root object, or by an explicit
 * list of DCI references (which takes precedence when not empty).
 */
public class DataExportRequest
{
   public static final int FORMAT_CSV = 0;

   public static final int FLAG_ANONYMIZE = 0x0001;
   public static final int FLAG_KEEP_MAPPING = 0x0002;

   /**
    * Reference to DCI by owner object ID and DCI ID
    */
   public static class DciReference
   {
      public final long ownerId;
      public final long dciId;

      public DciReference(long ownerId, long dciId)
      {
         this.ownerId = ownerId;
         this.dciId = dciId;
      }
   }

   private long rootObjectId = 0;
   private List<String> tagPatterns = new ArrayList<>();
   private List<DciReference> dciList = new ArrayList<>();
   private Date timeFrom;
   private Date timeTo;
   private DciTier tier = DciTier.RAW;
   private boolean anonymize = false;
   private boolean keepMapping = false;

   /**
    * Fill NXCP message with request data
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, rootObjectId);
      msg.setFieldsFromStringCollection(tagPatterns, NXCPCodes.VID_TAG_LIST_BASE, NXCPCodes.VID_NUM_TAGS);
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, dciList.size());
      long fieldId = NXCPCodes.VID_ITEM_LIST;
      for(DciReference ref : dciList)
      {
         msg.setFieldUInt32(fieldId++, ref.ownerId);
         msg.setFieldUInt32(fieldId++, ref.dciId);
      }
      msg.setFieldInt64(NXCPCodes.VID_TIME_FROM, (timeFrom != null) ? timeFrom.getTime() : 0);
      msg.setFieldInt64(NXCPCodes.VID_TIME_TO, (timeTo != null) ? timeTo.getTime() : 0);
      msg.setFieldInt16(NXCPCodes.VID_DCI_TIER, tier.getValue());
      msg.setFieldInt16(NXCPCodes.VID_DATA_FORMAT, FORMAT_CSV);
      int flags = 0;
      if (anonymize)
         flags |= FLAG_ANONYMIZE;
      if (keepMapping)
         flags |= FLAG_KEEP_MAPPING;
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
   }

   /**
    * @return root object ID
    */
   public long getRootObjectId()
   {
      return rootObjectId;
   }

   /**
    * @param rootObjectId root object ID (subtree searched for tagged DCIs)
    */
   public void setRootObjectId(long rootObjectId)
   {
      this.rootObjectId = rootObjectId;
   }

   /**
    * @return tag glob patterns
    */
   public List<String> getTagPatterns()
   {
      return tagPatterns;
   }

   /**
    * @param tagPatterns tag glob patterns (e.g. "eed.*")
    */
   public void setTagPatterns(List<String> tagPatterns)
   {
      this.tagPatterns = tagPatterns;
   }

   /**
    * @return explicit DCI list
    */
   public List<DciReference> getDciList()
   {
      return dciList;
   }

   /**
    * @param dciList explicit DCI list; when not empty, root object and tag patterns are ignored
    */
   public void setDciList(List<DciReference> dciList)
   {
      this.dciList = dciList;
   }

   /**
    * @return start of export period
    */
   public Date getTimeFrom()
   {
      return timeFrom;
   }

   /**
    * @param timeFrom start of export period
    */
   public void setTimeFrom(Date timeFrom)
   {
      this.timeFrom = timeFrom;
   }

   /**
    * @return end of export period (inclusive)
    */
   public Date getTimeTo()
   {
      return timeTo;
   }

   /**
    * @param timeTo end of export period (inclusive)
    */
   public void setTimeTo(Date timeTo)
   {
      this.timeTo = timeTo;
   }

   /**
    * @return history tier to export
    */
   public DciTier getTier()
   {
      return tier;
   }

   /**
    * @param tier history tier to export (RAW, HOURLY, or DAILY)
    */
   public void setTier(DciTier tier)
   {
      this.tier = tier;
   }

   /**
    * @return true if identities should be replaced with pseudonyms
    */
   public boolean isAnonymize()
   {
      return anonymize;
   }

   /**
    * @param anonymize true to replace identities with pseudonyms
    */
   public void setAnonymize(boolean anonymize)
   {
      this.anonymize = anonymize;
   }

   /**
    * @return true if pseudonym mapping file should be produced
    */
   public boolean isKeepMapping()
   {
      return keepMapping;
   }

   /**
    * @param keepMapping true to produce pseudonym mapping file (anonymized exports only)
    */
   public void setKeepMapping(boolean keepMapping)
   {
      this.keepMapping = keepMapping;
   }
}
