package com.radensolutions.reporting.application;

import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.quartz.JobDetail;

public interface ReportScheduler
{

    /**
     * Schedule report for one time, immediate execution
     *
     *
     * @param userId
     * @param reportUuid report UUID
     * @param parameters map of report parameters
     * @return job UUID or null
     */
    UUID execute(UUID jobId, int userId, UUID reportUuid, Map<String, Object> parameters);

    /**
     * Schedule report for recurrent execution, based on schedule
     *
     * @param reportUuid  report UUID
     * @param hours       execute at .. HOURS (0-23)
     * @param minutes     execute at .. MINUTES (0-59)
     * @param daysOfWeek  execute on Mon-Sat, bit mask (b00000001 - Sunday ... b01000000 - Monday)
     * @param daysOfMonth execute on day(s) of month, bit mask in network byte order
     * @param parameters  map of report parameters
     * @return job UUID or null
     */
    UUID addRecurrent(UUID jobId, UUID reportUuid, int jobType, int daysOfWeek, int daysOfMonth, Date startDate, Map<String, Object> parameters, int userId);
    
    /**
     * List of schedules
     *
     * @param reportUuid  report UUID
     * @return
     */
    List<JobDetail> getSchedules(UUID reportUuid);
    
    
    /**
     * Delete schedule by job uuid
     * 
     * @param reportId - report uuid
     * @param jobId - schedule uuid id
     * @return
     */
    boolean deleteScheduleJob(UUID reportId, UUID jobId);
}
