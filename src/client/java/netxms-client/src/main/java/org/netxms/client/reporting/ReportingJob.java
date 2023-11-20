/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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

import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCException;
import org.netxms.client.ScheduledTask;
import org.netxms.client.constants.RCC;
import org.netxms.client.xml.XMLTools;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Reporting job
 */
public class ReportingJob
{
   private static final Logger logger = LoggerFactory.getLogger(ReportingJob.class);

   private ReportingJobConfiguration configuration;
   private ScheduledTask task;

	/**
    * Create reporting job
    * 
    * @param report report definition object
    */
	public ReportingJob(ReportDefinition report)
	{
      configuration = new ReportingJobConfiguration(report.getId());
      task = new ScheduledTask();
      task.setTaskHandlerId("Report.Execute");
      task.setComments(report.getName());
	}

	/**
    * Create reporting job object from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field id
    */
	public ReportingJob(NXCPMessage msg, long baseId)
	{
      task = new ScheduledTask(msg, baseId);
      try
      {
         configuration = XMLTools.createFromXml(ReportingJobConfiguration.class, task.getParameters());
      }
      catch(Exception e)
      {
         logger.error("Cannot deserialize reporting job parameters", e);
         configuration = new ReportingJobConfiguration(NXCommon.EMPTY_GUID);
      }
	}

   /**
    * Fill NXCP message with job data. Will serialize execution parameters and update underlying scheduled task.
    * 
    * @param msg NXCP message
    * @throws NXCException if execution parameters serialization fails
    */
   public void fillMessage(NXCPMessage msg) throws NXCException
   {
      try
      {
         task.setParameters(configuration.createXml());
         task.fillMessage(msg);
      }
      catch(Exception e)
      {
         throw new NXCException(RCC.INTERNAL_ERROR, e);
      }
   }

	/**
    * @return the renderFormat
    */
   public ReportRenderFormat getRenderFormat()
   {
      return configuration.renderFormat;
   }

   /**
    * @param renderFormat the renderFormat to set
    */
   public void setRenderFormat(ReportRenderFormat renderFormat)
   {
      configuration.renderFormat = renderFormat;
   }

   /**
    * @return the emailRecipients
    */
   public List<String> getEmailRecipients()
   {
      return configuration.emailRecipients;
   }

   /**
    * @param emailRecipients the emailRecipients to set
    */
   public void setEmailRecipients(List<String> emailRecipients)
   {
      configuration.emailRecipients = emailRecipients;
   }

   /**
    * Get unique report ID
    * 
	 * @return unique report ID
	 */
	public UUID getReportId()
	{
      return configuration.reportId;
	}

	/**
	 * Check if notification on completion is on
	 * 
    * @return true if notification on completion is on
    */
   public boolean isNotifyOnCompletion()
   {
      return configuration.notifyOnCompletion;
   }

   /**
    * Set notification on completion flag
    * 
    * @param notifyOnCompletion new notification on completion flag
    */
   public void setNotifyOnCompletion(boolean notifyOnCompletion)
   {
      configuration.notifyOnCompletion = notifyOnCompletion;
   }

   /**
    * Get all reporting job execution parameters. Any changes to returned map will be reflected in job object.
    * 
    * @return all reporting job execution parameters
    */
   public Map<String, String> getExecutionParameters()
   {
      return configuration.executionParameters;
   }

   /**
    * Get value of single reporting job execution parameter.
    *
    * @param name parameter name
    * @return parameter value or null
    */
   public String getExecutionParameter(String name)
   {
      return configuration.executionParameters.get(name);
   }

   /**
    * Set reporting job execution parameter.
    *
    * @param name parameter name
    * @param value parameter value
    */
   public void setExecutionParameter(String name, String value)
   {
      configuration.executionParameters.put(name, value);
   }

   /**
    * Prepare underlying scheduled task for create or update API call.
    *
    * @return underlying scheduled task
    * @throws Exception if task parameter serialization fails
    */
   public ScheduledTask prepareTask() throws Exception
   {
      task.setParameters(configuration.createXml());
      return task;
   }

   /**
    * Get underlying scheduled task
    *
    * @return underlying scheduled task
    */
   public ScheduledTask getTask()
   {
      return task;
   }
}
