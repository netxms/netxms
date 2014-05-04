package com.radensolutions.reporting.application.impl;

import static org.quartz.CronScheduleBuilder.cronSchedule;
import static org.quartz.JobBuilder.newJob;
import static org.quartz.SimpleScheduleBuilder.simpleSchedule;
import static org.quartz.TriggerBuilder.newTrigger;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Set;
import java.util.UUID;

import org.netxms.api.client.SessionNotification;
import org.quartz.JobDataMap;
import org.quartz.JobDetail;
import org.quartz.JobExecutionContext;
import org.quartz.JobExecutionException;
import org.quartz.JobKey;
import org.quartz.JobListener;
import org.quartz.Scheduler;
import org.quartz.SchedulerException;
import org.quartz.SimpleTrigger;
import org.quartz.Trigger;
import org.quartz.impl.StdSchedulerFactory;
import org.quartz.impl.matchers.GroupMatcher;
import org.quartz.impl.matchers.KeyMatcher;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;

import com.radensolutions.reporting.application.ReportScheduler;
import com.radensolutions.reporting.application.ReportingServerFactory;
import com.radensolutions.reporting.application.Session;
import com.radensolutions.reporting.application.jobs.GeneratorJob;
import com.radensolutions.reporting.service.NotificationService;

public class QuartzReportScheduler implements ReportScheduler
{
	
	public class ReportingJobListener implements JobListener
	{
		UUID jobId;
		int jobType;
		
		public ReportingJobListener(UUID jobId, int jobType)
		{
			this.jobId = jobId;
			this.jobType = jobType;
		}

		@Override
		public String getName()
		{
			return "reportingJobListener";
		}

		@Override
		public void jobToBeExecuted(JobExecutionContext context)
		{
		}

		@Override
		public void jobExecutionVetoed(JobExecutionContext context)
		{
		}

		@Override
		public void jobWasExecuted(JobExecutionContext context, JobExecutionException jobException)
		{
			if (jobType == TYPE_ONCE)
				notificationService.delete(jobId);
		}
		
	}
	
	public static final String SYSTEM_USER_NAME = "SYSTEM";
	public static String[] dayOfWeekNames = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
	
	public static final int TYPE_ONCE = 0;
	public static final int TYPE_DAILY = 1;
	public static final int TYPE_WEEKLY = 2;
	public static final int TYPE_MONTHLY = 3;

	private static final Logger log = LoggerFactory.getLogger(QuartzReportScheduler.class);
	private final Scheduler scheduler;
	
	@Autowired
	private NotificationService notificationService;

	public QuartzReportScheduler() throws SchedulerException
	{
		final Properties defaultProperties = loadProperties("org/quartz/quartz.properties");
		final Properties localProperties = loadProperties("nxreporting.properties");
		final Properties mergedProperties = new Properties();
		mergedProperties.putAll(defaultProperties);
		mergedProperties.putAll(localProperties);
		System.out.println("###" + mergedProperties);
		final StdSchedulerFactory factory = new StdSchedulerFactory(mergedProperties);
		scheduler = factory.getScheduler();
		scheduler.start();
	}

	private Properties loadProperties(String name)
	{
		ClassLoader classLoader = getClass().getClassLoader();
		Properties properties = new Properties();
		InputStream stream = null;
		try
		{
			stream = classLoader.getResourceAsStream(name);
			properties.load(stream);
		} catch (IOException e)
		{
			e.printStackTrace();
		} finally
		{
			if (stream != null)
			{
				try
				{
					stream.close();
				} catch (IOException e)
				{/* ignore */
				}
			}
		}
		return properties;
	}

	@Override
	public UUID execute(UUID jobId, int userId, UUID reportUuid, Map<String, Object> parameters)
	{
		if (jobId == null)
			jobId = UUID.randomUUID();
		log.debug("New job " + jobId + " for report " + reportUuid);
		JobDetail job = newJob(GeneratorJob.class).withIdentity(jobId.toString(), reportUuid.toString()).build();
		final JobDataMap dataMap = job.getJobDataMap();
		dataMap.put("jobId", jobId);
		dataMap.put("reportId", reportUuid);
		dataMap.put("parameters", parameters);
		dataMap.put("userId", userId);

		Trigger trigger = newTrigger().withIdentity(reportUuid + "_trigger", reportUuid.toString()).startNow().withSchedule(simpleSchedule().withIntervalInSeconds(0)).build();

		try
		{
			scheduler.scheduleJob(job, trigger);
		}
		catch (SchedulerException e)
		{
			log.error("Can't schedule job", e);
			jobId = null;
		}
		return jobId;
	}

	@Override
	public UUID addRecurrent(UUID jobId, UUID reportUuid, int jobType, int daysOfWeek, int daysOfMonth, Date startTime, 
			Map<String, Object> parameters, int userId)
	{
		Calendar dateTime = Calendar.getInstance();
		dateTime.setTime(startTime);

		int hours = dateTime.get(Calendar.HOUR_OF_DAY);
		int minutes = dateTime.get(Calendar.MINUTE);
		log.debug(String.format("reportId=%1$s, hours=%2$d, minutes=%3$d, dow=%4$d, dom=%5$d", reportUuid, hours, minutes, daysOfWeek, daysOfMonth));

		if (jobId == null)
			jobId = UUID.randomUUID();
		// Remove old jobs and triggers
		// TODO: check for running jobs
		final String reportKey = reportUuid.toString();
		JobDetail job = newJob(GeneratorJob.class).withIdentity(jobId.toString(), reportKey).build();
		final JobDataMap dataMap = job.getJobDataMap();
		dataMap.put("jobId", jobId);
		dataMap.put("jobType", jobType);
		dataMap.put("reportId", reportUuid);
		dataMap.put("userId", userId);
		dataMap.put("parameters", parameters);
		dataMap.put("startDate", startTime);
		dataMap.put("daysOfWeek", daysOfWeek);
		dataMap.put("daysOfMonth", daysOfMonth);

		Trigger trigger = null;
		switch (jobType)
		{
			case TYPE_DAILY:
				trigger = newTrigger().withIdentity(jobId.toString() + "_trigger", reportKey).withSchedule(cronSchedule("0 " + minutes + " " + hours + " * * ?")).build();
				log.debug("Created daily cron (" + "0 " + minutes + " " + hours + " * * ?" + ")");
				break;
			case TYPE_WEEKLY:
				String sDays = "";
				for (int i = 0; i < 7; i++)
				{
					if (((daysOfWeek >> i) & 0x01) != 0)
					{
						sDays += (sDays.length() > 0 ? "," : "") + dayOfWeekNames[(7 - i) - 1];
					}
				}
				if (sDays.length() > 0)
				{
					trigger = newTrigger().withIdentity(jobId.toString() + "_trigger", reportKey).withSchedule(cronSchedule("0 " + minutes + " " + hours + " ? * " + sDays)).build();
				}
				log.debug("Created weekly cron (" + "0 " + minutes + " " + hours + " ? * " + sDays + ")");
				break;
			case TYPE_MONTHLY:
				String sDayOfM = "";
				for (int i = 0; i < 31; i++)
				{
					if (((daysOfMonth >> i) & 0x01) != 0)
						sDayOfM += (sDayOfM.length() > 0 ? "," : "") + String.valueOf(31 - i);
				}
				if (sDayOfM.length() > 0)
				{
					trigger = newTrigger().withIdentity(jobId.toString() + "_trigger", reportKey).withSchedule(cronSchedule("0 " + minutes + " " + hours + " " + sDayOfM + " * ?")).build();
				}
				log.debug("Created monthly cron (" + "0 " + minutes + " " + hours + " " + sDayOfM + " * ?" + ")");
				break;
			case TYPE_ONCE:
				trigger = (SimpleTrigger) newTrigger().startAt(dateTime.getTime()).withIdentity(jobId.toString() + "_trigger", reportKey).build();
				log.debug("Created once cron (" + dateTime.getTime() + ")");
				break;
			default:
				break;
		}
		try
		{
			scheduler.getListenerManager().addJobListener(new ReportingJobListener(jobId, jobType), KeyMatcher.keyEquals(job.getKey()));
			scheduler.scheduleJob(job, trigger);
		} catch (SchedulerException e)
		{
			log.error("Can't schedule job", e);
			jobId = null;
		}
		return jobId;
	}

	public List<JobDetail> getSchedules(UUID reportUuid)
	{
		List<JobDetail> jobDetailsList = new ArrayList<JobDetail>(0);
		try
		{
			for (String groupName : scheduler.getJobGroupNames())
			{
				for (JobKey jobKey : scheduler.getJobKeys(GroupMatcher.jobGroupEquals(groupName)))
					jobDetailsList.add(scheduler.getJobDetail(jobKey));
			}
		} catch (SchedulerException e)
		{
			e.printStackTrace();
		}

		return jobDetailsList;
	}

	@Override
	public boolean deleteScheduleJob(UUID reportId, UUID jobId)
	{
		try
		{
			final Set<JobKey> jobKeys = scheduler.getJobKeys(GroupMatcher.jobGroupEquals(reportId.toString()));
			for (JobKey key : jobKeys)
			{
				if (key.getName().equalsIgnoreCase(jobId.toString()))
					scheduler.deleteJob(key);
			}
		} catch (SchedulerException e)
		{
			Session session = ReportingServerFactory.getApp().getSession(ReportingServerFactory.getApp().getConnector());
			session.sendNotify(SessionNotification.RS_SCHEDULES_MODIFIED, 0);
			notificationService.delete(jobId);
			log.error("Can't find or remove old triggers", e);
			return false;
		}
		return true;
	}
}
