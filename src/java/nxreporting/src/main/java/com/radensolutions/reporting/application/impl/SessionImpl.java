package com.radensolutions.reporting.application.impl;

import com.radensolutions.reporting.application.ReportScheduler;
import com.radensolutions.reporting.application.Session;
import com.radensolutions.reporting.domain.Notification;
import com.radensolutions.reporting.domain.ReportDefinition;
import com.radensolutions.reporting.domain.ReportResult;
import com.radensolutions.reporting.infrastructure.Connector;
import com.radensolutions.reporting.infrastructure.ReportManager;
import com.radensolutions.reporting.service.NotificationService;
import org.netxms.api.client.SessionNotification;
import org.netxms.api.client.reporting.ReportRenderFormat;
import org.netxms.base.CommonRCC;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.quartz.JobDetail;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;

import java.io.IOException;
import java.io.OutputStream;
import java.util.*;

public class SessionImpl implements Session {
    public static final Logger logger = LoggerFactory.getLogger(SessionImpl.class);
    public static final int NXCP_VERSION = 0x02000000;
    private Connector connector;
    @Autowired
    private ReportManager reportManager;
    @Autowired
    private ReportScheduler scheduler;
    @Autowired
    private NotificationService notificationService;

    public SessionImpl(Connector connector) {
        this.connector = connector;
    }

    @Override
    public NXCPMessage processMessage(NXCPMessage message, OutputStream fileStream) {
        NXCPMessage reply = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, message.getMessageId());
        switch (message.getMessageCode()) {
            case NXCPCodes.CMD_ISC_CONNECT_TO_SERVICE: // ignore and reply "Ok"
            case NXCPCodes.CMD_KEEPALIVE:
                reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
                break;
            case NXCPCodes.CMD_GET_NXCP_CAPS:
                reply = new NXCPMessage(NXCPCodes.CMD_NXCP_CAPS, message.getMessageId());
                reply.setControl(true);
                reply.setControlData(NXCP_VERSION);
                break;
            case NXCPCodes.CMD_RS_LIST_REPORTS:
                listReports(reply);
                break;
            case NXCPCodes.CMD_RS_GET_REPORT_DEFINITION:
                getReportDefinition(message, reply);
                break;
            case NXCPCodes.CMD_RS_SCHEDULE_EXECUTION:
                scheduleExecution(message, reply);
                break;
            case NXCPCodes.CMD_RS_LIST_SCHEDULES:
                listSchedules(message, reply);
                break;
            case NXCPCodes.CMD_RS_DELETE_SCHEDULE:
                deleteSchedule(message, reply);
                break;
            case NXCPCodes.CMD_RS_LIST_RESULTS:
                listResults(message, reply);
                break;
            case NXCPCodes.CMD_RS_RENDER_RESULT:
                renderAndSendResult(message, reply, fileStream);
                break;
            case NXCPCodes.CMD_RS_DELETE_RESULT:
                deleteResult(message, reply);
                break;
            case NXCPCodes.CMD_RS_ADD_REPORT_NOTIFY:
                reportNotify(message, reply);
                break;
            default:
                reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.NOT_IMPLEMENTED);
        }
        return reply;
    }

    private void listReports(NXCPMessage reply) {
        final List<UUID> list = reportManager.listReports();
        reply.setVariableInt32(NXCPCodes.VID_NUM_ITEMS, list.size());
        for (int i = 0, listSize = list.size(); i < listSize; i++) {
            UUID uuid = list.get(i);
            reply.setVariable(NXCPCodes.VID_UUID_LIST_BASE + i, uuid);
        }
        reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
    }

    private void getReportDefinition(NXCPMessage request, NXCPMessage reply) {
        final UUID reportId = request.getVariableAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
        final String localeString = request.getVariableAsString(NXCPCodes.VID_LOCALE);
        final Locale locale = new Locale(localeString);
        final ReportDefinition definition;
        if (reportId != null) {
            definition = reportManager.getReportDefinition(reportId, locale);
        } else {
            definition = null;
        }

        if (definition != null) {
            reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
            definition.fillMessage(reply);
        } else {
            reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.INTERNAL_ERROR);
        }
    }

    private void listResults(NXCPMessage request, NXCPMessage reply) {
        reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
        final int userId = request.getVariableAsInteger(NXCPCodes.VID_USER_ID);
        final UUID reportId = request.getVariableAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
        final List<ReportResult> list = reportManager.listResults(reportId, userId);
        long index = NXCPCodes.VID_ROW_DATA_BASE;
        int jobNum = 0;
        for (ReportResult record : list) {
            reply.setVariable(index++, record.getJobId());
            reply.setVariableInt32(index++, (int) (record.getExecutionTime().getTime() / 1000));
            reply.setVariableInt32(index++, record.getUserId());
            index += 7;
            jobNum++;
        }
        reply.setVariableInt32(NXCPCodes.VID_NUM_ITEMS, jobNum);
        reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
    }

    private void scheduleExecution(NXCPMessage request, NXCPMessage reply) {
        int retCode;
        final UUID reportUuid = request.getVariableAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
        if (reportUuid != null) {
            Map<String, Object> parameters = getParametersFromRequest(request);
            final UUID jobUuid = request.getVariableAsUUID(NXCPCodes.VID_RS_JOB_ID);
            final int jobType = request.getVariableAsInteger(NXCPCodes.VID_RS_JOB_TYPE);
            final long timestamp = request.getVariableAsInt64(NXCPCodes.VID_TIMESTAMP);
            final int userId = request.getVariableAsInteger(NXCPCodes.VID_USER_ID);
            if (timestamp == 0) {
                final UUID jobId = scheduler.execute(jobUuid, userId, reportUuid, parameters);
                if (jobId != null) {
                    reply.setVariable(NXCPCodes.VID_JOB_ID, jobId);
                }
                sendNotify(SessionNotification.RS_RESULTS_MODIFIED, 0);
            } else {
                final int daysOfWeek = request.getVariableAsInteger(NXCPCodes.VID_DAY_OF_WEEK);
                final int daysOfMonth = request.getVariableAsInteger(NXCPCodes.VID_DAY_OF_MONTH);
                scheduler.addRecurrent(jobUuid, reportUuid, jobType, daysOfWeek, daysOfMonth, new Date(timestamp), parameters, userId);
                sendNotify(SessionNotification.RS_SCHEDULES_MODIFIED, 0);
            }

            retCode = CommonRCC.SUCCESS;
        } else {
            retCode = CommonRCC.INVALID_ARGUMENT;
        }

        reply.setVariableInt32(NXCPCodes.VID_RCC, retCode);
    }

    private void listSchedules(NXCPMessage request, NXCPMessage reply) {
        final UUID reportId = request.getVariableAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
        final int userId = request.getVariableAsInteger(NXCPCodes.VID_USER_ID);
        List<JobDetail> jobDetailList = scheduler.getSchedules(reportId);
        int jobDetailsNum = 0;
        long varId = NXCPCodes.VID_ROW_DATA_BASE;
        for (JobDetail jobDetail : jobDetailList) {
            Map<String, Object> jobDataMap = jobDetail.getJobDataMap().getWrappedMap();
            try {
                if (reportId.equals(jobDataMap.get("reportId")) && ((Integer) jobDataMap.get("userId") == userId || userId == 0)) {
                    reply.setVariable(varId, (UUID) jobDataMap.get("jobId"));
                    reply.setVariable(varId + 1, (UUID) jobDataMap.get("reportId"));
                    reply.setVariableInt32(varId + 2, (Integer) jobDataMap.get("userId"));
                    reply.setVariableInt64(varId + 3, ((Date) jobDataMap.get("startDate")).getTime());
                    reply.setVariableInt32(varId + 4, (Integer) jobDataMap.get("daysOfWeek"));
                    reply.setVariableInt32(varId + 5, (Integer) jobDataMap.get("daysOfMonth"));
                    reply.setVariableInt32(varId + 6, (Integer) jobDataMap.get("jobType"));
                    reply.setVariable(varId + 7, (String) jobDataMap.get("comments"));

                    varId += 20;
                    jobDetailsNum++;
                }
            } catch (Exception e) {
                logger.error("Application error: ", e);
            }
        }
        reply.setVariableInt32(NXCPCodes.VID_NUM_ITEMS, jobDetailsNum);
        reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
    }

    private Map<String, Object> getParametersFromRequest(NXCPMessage request) {
        final HashMap<String, Object> map = new HashMap<String, Object>();
        final int count = request.getVariableAsInteger(NXCPCodes.VID_NUM_PARAMETERS);
        long id = NXCPCodes.VID_PARAM_LIST_BASE;
        for (int i = 0; i < count; i++) {
            String key = request.getVariableAsString(id++);
            String value = request.getVariableAsString(id++);
            map.put(key, value);
        }
        return map;
    }

    private void renderAndSendResult(NXCPMessage request, NXCPMessage reply, OutputStream fileStream) {
        final UUID reportId = request.getVariableAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
        final UUID jobId = request.getVariableAsUUID(NXCPCodes.VID_JOB_ID);
        final int formatCode = request.getVariableAsInteger(NXCPCodes.VID_RENDER_FORMAT);
        final ReportRenderFormat format = ReportRenderFormat.valueOf(formatCode);

        reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
        sendNotify(SessionNotification.RS_RESULTS_MODIFIED, 0);
        byte[] output = reportManager.renderResult(reportId, jobId, format);
        if (output != null) {
            try {
                fileStream.write(output);
            } catch (IOException e) {
                logger.error("I/O error", e);
            }
        }
    }

    private void deleteResult(NXCPMessage request, NXCPMessage reply) {
        final UUID reportId = request.getVariableAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
        final UUID jobId = request.getVariableAsUUID(NXCPCodes.VID_JOB_ID);

        reportManager.deleteResult(reportId, jobId);
        reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
        sendNotify(SessionNotification.RS_RESULTS_MODIFIED, 0);
    }

    private void deleteSchedule(NXCPMessage request, NXCPMessage reply) {
        final UUID reportId = request.getVariableAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
        final UUID jobId = request.getVariableAsUUID(NXCPCodes.VID_JOB_ID);

        scheduler.deleteScheduleJob(reportId, jobId);
        notificationService.delete(jobId);

        reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
        sendNotify(SessionNotification.RS_SCHEDULES_MODIFIED, 0);
    }

    /**
     * Add notification when job is done then send a notice to mail
     */
    private void reportNotify(NXCPMessage request, NXCPMessage reply) {
        final UUID jobId = request.getVariableAsUUID(NXCPCodes.VID_RS_JOB_ID);
        final int attachFormatCode = request.getVariableAsInteger(NXCPCodes.VID_RENDER_FORMAT);
        String reportName = request.getVariableAsString(NXCPCodes.VID_RS_REPORT_NAME);
        final int count = request.getVariableAsInteger(NXCPCodes.VID_NUM_ITEMS);
        long index = NXCPCodes.VID_ITEM_LIST;
        for (int i = 0; i < count; i++) {
            String mail = request.getVariableAsString(index + i);
            if (mail != null && jobId != null)
                notificationService.create(new Notification(jobId, mail, attachFormatCode, reportName));
        }
        reply.setVariableInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
    }

    @Override
    public String toString() {
        return "Session{" + "connector=" + connector + '}';
    }

    /**
     * Send notification message
     */
    public void sendNotify(int code, int data) {
        NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_RS_NOTIFY);
        msg.setVariableInt32(NXCPCodes.VID_NOTIFICATION_CODE, code);
        msg.setVariableInt32(NXCPCodes.VID_NOTIFICATION_DATA, data);
        try {
            connector.sendBroadcast(msg);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
