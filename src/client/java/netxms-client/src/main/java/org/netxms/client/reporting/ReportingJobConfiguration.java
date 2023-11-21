/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.client.reporting;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.netxms.base.NXCommon;
import org.netxms.client.xml.XMLTools;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.ElementMap;
import org.simpleframework.xml.Root;

/**
 * Parameters for reporting job
 */
@Root(name = "job", strict = false)
public class ReportingJobConfiguration
{
   @Element
   public UUID reportId;

   @ElementMap(required = false)
   public Map<String, String> executionParameters = new HashMap<String, String>();

   @Element(required = false)
   public boolean notifyOnCompletion = false;

   @Element(required = false)
   public ReportRenderFormat renderFormat = ReportRenderFormat.NONE;

   @ElementList(required = false)
   public List<String> emailRecipients = new ArrayList<String>(0);

   /**
    * Default constructor - for XML deserialization only
    */
   protected ReportingJobConfiguration()
   {
      reportId = NXCommon.EMPTY_GUID;
   }

   /**
    * Create new job configuration.
    * 
    * @param reportId report ID
    */
   public ReportingJobConfiguration(UUID reportId)
   {
      this.reportId = reportId;
   }

   /**
    * Create XML from object.
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      return XMLTools.serialize(this);
   }
}
