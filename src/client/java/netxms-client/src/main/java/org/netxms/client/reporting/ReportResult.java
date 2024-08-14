/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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

import java.io.File;
import java.io.FileWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.util.Date;
import java.util.UUID;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;
import org.netxms.client.xml.XMLTools;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;

/**
 * Report execution result
 */
@Root(name = "result", strict = false)
public class ReportResult
{
   @Element(required = true)
   private UUID jobId;

   @Element(required = false)
   private UUID reportId;

   @Element(required = false)
   private boolean carboneReport;

   @Element(required = false)
	private Date executionTime;

   @Element(required = false)
	private int userId;

   @Element(required = false)
   private boolean success;

   /**
    * Create report result object from XML file
    *
    * @param xmlFile file containing XML document
    * @return reporting job parameters object
    * @throws Exception if de-serialization is not possible
    */
   public static ReportResult loadFromFile(final File xmlFile) throws Exception
   {
      Serializer serializer = XMLTools.createSerializer();
      return serializer.read(ReportResult.class, xmlFile, false);
   }

   /**
    * Default constructor - only intended for use during XML deserialization
    */
   protected ReportResult()
   {
      jobId = NXCommon.EMPTY_GUID;
      reportId = NXCommon.EMPTY_GUID;
      carboneReport = false;
      executionTime = new Date();
      userId = 0;
      success = false;
   }

   /**
    * Create new report result object.
    *
    * @param jobId reporting job ID
    * @param reportId report ID
    * @param carboneReport true if report should be rendered with Carbone
    * @param executionTime execution time
    * @param userId user (initiator) ID
    * @param success success indicator
    */
   public ReportResult(UUID jobId, UUID reportId, boolean carboneReport, Date executionTime, int userId, boolean success)
   {
      this.jobId = jobId;
      this.reportId = reportId;
      this.carboneReport = carboneReport;
      this.executionTime = executionTime;
      this.userId = userId;
      this.success = success;
   }

   /**
    * Create report result object from NXCP message.
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public ReportResult(NXCPMessage msg, long baseId)
	{
      jobId = msg.getFieldAsUUID(baseId);
      reportId = msg.getFieldAsUUID(baseId + 1);
      executionTime = msg.getFieldAsDate(baseId + 2);
      userId = msg.getFieldAsInt32(baseId + 3);
      success = msg.getFieldAsBoolean(baseId + 4);
      carboneReport = msg.getFieldAsBoolean(baseId + 5);
	}

   /**
    * Fill NXCP message with object's data.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setField(baseId, jobId);
      msg.setField(baseId + 1, reportId);
      msg.setField(baseId + 2, executionTime);
      msg.setFieldInt32(baseId + 3, userId);
      msg.setField(baseId + 4, success);
      msg.setField(baseId + 5, carboneReport);
   }

   /**
    * Create XML from object.
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      Serializer serializer = XMLTools.createSerializer();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }

   /**
    * Save object as XML file.
    * 
    * @param file destination file
    * @throws Exception if the schema for the object is not valid or write operation failed
    */
   public void saveAsXml(File file) throws Exception
   {
      Serializer serializer = XMLTools.createSerializer();
      Writer writer = new FileWriter(file);
      serializer.write(this, writer);
      writer.close();
   }

   /**
    * Get job ID.
    *
    * @return job ID
    */
   public UUID getJobId()
   {
      return jobId;
   }

   /**
    * Get report ID.
    *
    * @return report ID
    */
   public UUID getReportId()
   {
      return reportId;
   }

   /**
    * Get execution time.
    *
    * @return execution time
    */
	public Date getExecutionTime()
	{
		return executionTime;
	}

   /**
    * Get user ID.
    *
    * @return user ID
    */
	public int getUserId()
	{
		return userId;
	}

   /**
    * Get success indicator.
    *
    * @return true if execution was successful
    */
   public boolean isSuccess()
   {
      return success;
   }

   /**
    * Returns true if this report definition is intended for Carbone renderer instead of standard Jasper renderer.
    * 
    * @return true if this report definition is intended for Carbone renderer
    */
   public boolean isCarboneReport()
   {
      return carboneReport;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ReportResult [jobId=" + jobId + ", reportId=" + reportId + ", carboneReport=" + carboneReport + ", executionTime=" + executionTime + ", userId=" + userId + ", success=" + success + "]";
   }
}
