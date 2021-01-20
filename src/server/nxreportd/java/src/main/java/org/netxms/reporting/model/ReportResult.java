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
package org.netxms.reporting.model;

import org.hibernate.annotations.Type;

import javax.persistence.*;
import java.io.Serializable;
import java.util.Date;
import java.util.UUID;

/**
 * Report result
 */
@Entity
@Table(name = "report_results")
public class ReportResult implements Serializable
{
   private static final long serialVersionUID = 1L;

   @Id
   @Column(name = "id")
   @GeneratedValue
   private Integer id;

   @Column(name = "executionTime")
   private Date executionTime;

   @Column(name = "reportId")
   @Type(type = "uuid-char")
   private UUID reportId;

   @Column(name = "jobId")
   @Type(type = "uuid-char")
   private UUID jobId;

   @Column(name = "userId")
   private int userId;

   public ReportResult()
   {
   }

   public ReportResult(Date executionTime, UUID reportId, UUID jobId, int userId)
   {
      this.executionTime = executionTime;
      this.reportId = reportId;
      this.jobId = jobId;
      this.userId = userId;
   }

   public Integer getId()
   {
      return id;
   }

   public void setId(Integer id)
   {
      this.id = id;
   }

   public Date getExecutionTime()
   {
      return executionTime;
   }

   public void setExecutionTime(Date executionTime)
   {
      this.executionTime = executionTime;
   }

   public UUID getReportId()
   {
      return reportId;
   }

   public void setReportId(UUID reportId)
   {
      this.reportId = reportId;
   }

   public UUID getJobId()
   {
      return jobId;
   }

   public void setJobId(UUID jobId)
   {
      this.jobId = jobId;
   }

   public int getUserId()
   {
      return userId;
   }

   public void setUserId(int userId)
   {
      this.userId = userId;
   }

   @Override
   public String toString()
   {
      return "ReportResult{" + "id=" + id + ", executionTime=" + executionTime + ", reportId=" + reportId + ", jobId=" + jobId
            + ", userId='" + userId + '\'' + '}';
   }
}
