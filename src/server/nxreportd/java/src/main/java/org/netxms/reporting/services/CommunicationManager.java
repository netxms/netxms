/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.reporting.services;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.Properties;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.reporting.ReportRenderFormat;
import org.netxms.client.reporting.ReportResult;
import org.netxms.client.reporting.ReportingJobConfiguration;
import org.netxms.client.xml.XMLTools;
import org.netxms.reporting.Server;
import org.netxms.reporting.model.ReportDefinition;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Communication manager - manages communications with core server.
 */
public class CommunicationManager
{
   private static final int FILE_BUFFER_SIZE = 128 * 1024; // 128k

   private static Logger logger = LoggerFactory.getLogger(CommunicationManager.class);

   private final Object mutex = new Object();

   private Server server;
   private Socket socket;
   private Thread receiverThread;

   /**
    * Create communication manager on given socket
    */
   public CommunicationManager(Server server)
   {
      this.server = server;
      this.socket = null;
      receiverThread = null;
   }

   /**
    * Start communication session
    */
   public void start(Socket socket)
   {
      synchronized(mutex)
      {
         if (this.socket != null)
         {
            try
            {
               this.socket.close();
            }
            catch(IOException e)
            {
               logger.error("Error closing existing socket", e);
            }
         }
         if (receiverThread != null)
         {
            try
            {
               receiverThread.join();
            }
            catch(InterruptedException e)
            {
               logger.error("Interrupted while waiting for receiver thread", e);
            }
         }
         this.socket = socket;
         receiverThread = new Thread(new Runnable() {
            @Override
            public void run()
            {
               logger.info("Communication manager started");
               receiverThread();
               logger.info("Communication manager stopped");
            }
         }, "Network Receiver");
         receiverThread.start();
      }
   }

   /**
    * Shutdown communication session
    */
   public void shutdown()
   {
      synchronized(mutex)
      {
         if (socket != null)
         {
            try
            {
               socket.close();
            }
            catch(IOException e)
            {
               logger.error("Error closing existing socket", e);
            }
         }
         if (receiverThread != null)
         {
            try
            {
               receiverThread.join();
            }
            catch(InterruptedException e)
            {
               logger.warn("Unexpected exception during comm session shutdown", e);
            }
            receiverThread = null;
         }
         socket = null;
      }
   }

   /**
    * Send message to core server.
    *
    * @param message message to send
    * @return true on success
    */
   public boolean sendMessage(NXCPMessage message)
   {
      try
      {
         synchronized(mutex)
         {
            socket.getOutputStream().write(message.createNXCPMessage(false));
         }
         return true;
      }
      catch(IOException e)
      {
         logger.error("Communication failure", e);
      }
      return false;
   }

   /**
    * Send file stream to core server.
    *
    * @param requestId request ID
    * @param inputStream input stream for file
    * @throws IOException
    */
   public void sendFileStream(final long requestId, final InputStream inputStream) throws IOException
   {
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_FILE_DATA, requestId);
      msg.setBinaryMessage(true);

      boolean success = false;
      final byte[] buffer = new byte[FILE_BUFFER_SIZE];
      while(true)
      {
         final int bytesRead = inputStream.read(buffer);
         if (bytesRead < FILE_BUFFER_SIZE)
         {
            msg.setEndOfFile(true);
         }

         msg.setBinaryData(bytesRead == -1 ? new byte[0] : Arrays.copyOf(buffer, bytesRead));
         sendMessage(msg);

         if (bytesRead < FILE_BUFFER_SIZE)
         {
            success = true;
            break;
         }
      }

      if (!success)
      {
         NXCPMessage abortMessage = new NXCPMessage(NXCPCodes.CMD_ABORT_FILE_TRANSFER, requestId);
         abortMessage.setBinaryMessage(true);
         sendMessage(abortMessage);
      }
   }

   /**
    * Receiver thread for communication session
    */
   private void receiverThread()
   {
      final NXCPMessageReceiver messageReceiver = new NXCPMessageReceiver(262144, 4194304);   // 256KB, 4MB
      while(!Thread.currentThread().isInterrupted())
      {
         try
         {
            Socket s;
            synchronized(mutex)
            {
               if (socket == null)
                  break;
               s = socket;
            }
            final NXCPMessage message = messageReceiver.receiveMessage(s.getInputStream(), null);
            if (message != null)
            {
               if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE)
               {
                  logger.debug("RECV: " + message.toString());
               }
               final MessageProcessingResult result = processMessage(message);
               if (result.response != null)
               {
                  if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE)
                  {
                     logger.debug("SEND: " + result.response.toString());
                  }
                  sendMessage(result.response);
                  if (result.file != null)
                  {
                     logger.debug("File data found, sending");
                     FileInputStream in = null;
                     try
                     {
                        in = new FileInputStream(result.file);
                        sendFileStream(message.getMessageId(), in);
                     }
                     catch(IOException e)
                     {
                        logger.error("Unexpected I/O exception while sending rendered file");
                     }
                     if (in != null)
                        in.close();
                     result.file.delete();
                  }
               }
            }
         }
         catch(IOException e)
         {
            logger.info("Communication error", e);
            break;
         }
         catch(NXCPException e)
         {
            logger.info("Protocol error", e);
            if (e.getErrorCode() == NXCPException.SESSION_CLOSED)
               break;
         }
      }
   }

   /**
    * Process incoming message
    * 
    * @param request input message
    * @return message processing result
    */
   public MessageProcessingResult processMessage(NXCPMessage request)
   {
      NXCPMessage reply = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());
      File file = null;
      switch(request.getMessageCode())
      {
         case NXCPCodes.CMD_ISC_CONNECT_TO_SERVICE: // ignore and reply "Ok"
         case NXCPCodes.CMD_KEEPALIVE:
            reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
            break;
         case NXCPCodes.CMD_GET_NXCP_CAPS:
            reply.setMessageCode(NXCPCodes.CMD_NXCP_CAPS);
            reply.setControl(true);
            reply.setControlData(5); // NXCP version 5
            break;
         case NXCPCodes.CMD_CONFIGURE_REPORTING_SERVER:
            configureServer(request);
            reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
            break;
         case NXCPCodes.CMD_RS_LIST_REPORTS:
            listReports(reply);
            break;
         case NXCPCodes.CMD_RS_GET_REPORT_DEFINITION:
            getReportDefinition(request, reply);
            break;
         case NXCPCodes.CMD_RS_EXECUTE_REPORT:
            executeReport(request, reply);
            break;
         case NXCPCodes.CMD_RS_LIST_RESULTS:
            getResults(request, reply);
            break;
         case NXCPCodes.CMD_RS_RENDER_RESULT:
            file = renderResult(request, reply);
            break;
         case NXCPCodes.CMD_RS_DELETE_RESULT:
            deleteResult(request, reply);
            break;
         default:
            reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.NOT_IMPLEMENTED);
            break;
      }
      return new MessageProcessingResult(reply, file);
   }

   /**
    * Configure server
    *
    * @param request request message
    */
   private void configureServer(NXCPMessage request)
   {
      Properties properties = new Properties();
      properties.setProperty("netxms.db.driver@remote", request.getFieldAsString(NXCPCodes.VID_DB_DRIVER));
      properties.setProperty("netxms.db.server@remote", request.getFieldAsString(NXCPCodes.VID_DB_SERVER));
      properties.setProperty("netxms.db.name@remote", request.getFieldAsString(NXCPCodes.VID_DB_NAME));
      properties.setProperty("netxms.db.login@remote", request.getFieldAsString(NXCPCodes.VID_DB_LOGIN));
      properties.setProperty("netxms.db.password@remote", request.getFieldAsString(NXCPCodes.VID_DB_PASSWORD));
      properties.setProperty("netxms.db.jdbc.properties@remote", request.getFieldAsString(NXCPCodes.VID_JDBC_OPTIONS));
      properties.setProperty("nxreportd.resultsRetentionTime@remote", request.getFieldAsString(NXCPCodes.VID_RETENTION_TIME));
      properties.setProperty("smtp.fromAddr@remote", request.getFieldAsString(NXCPCodes.VID_SMTP_FROM_ADDRESS));
      properties.setProperty("smtp.fromName@remote", request.getFieldAsString(NXCPCodes.VID_SMTP_FROM_NAME));
      properties.setProperty("smtp.server@remote", request.getFieldAsString(NXCPCodes.VID_SMTP_SERVER));
      properties.setProperty("smtp.port@remote", request.getFieldAsString(NXCPCodes.VID_SMTP_PORT));
      properties.setProperty("smtp.login@remote", request.getFieldAsString(NXCPCodes.VID_SMTP_LOGIN));
      properties.setProperty("smtp.password@remote", request.getFieldAsString(NXCPCodes.VID_SMTP_PASSWORD));
      properties.setProperty("smtp.tlsMode@remote", request.getFieldAsString(NXCPCodes.VID_SMTP_TLS_MODE));
      server.updateConfiguration(properties);
   }

   /**
    * @param reply
    */
   private void listReports(NXCPMessage reply)
   {
      final List<UUID> list = server.getReportManager().listReports();
      reply.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, list.size());
      for(int i = 0, listSize = list.size(); i < listSize; i++)
      {
         UUID uuid = list.get(i);
         reply.setField(NXCPCodes.VID_UUID_LIST_BASE + i, uuid);
      }
      reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
   }

   /**
    * Get report definition.
    *
    * @param request request message
    * @param response response message
    */
   private void getReportDefinition(NXCPMessage request, NXCPMessage response)
   {
      final UUID reportId = request.getFieldAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
      final Locale locale = new Locale(request.getFieldAsString(NXCPCodes.VID_LOCALE));
      final ReportDefinition definition;
      if (reportId != null)
      {
         definition = server.getReportManager().getReportDefinition(reportId, locale);
      }
      else
      {
         definition = null;
      }

      if (definition != null)
      {
         logger.debug("Successfully retrieved report definition: " + definition);
         response.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
         definition.fillMessage(response);
      }
      else
      {
         response.setFieldInt32(NXCPCodes.VID_RCC, RCC.INTERNAL_ERROR);
      }
   }

   /**
    * Get results for given report.
    *
    * @param request request message
    * @param reply response message
    */
   private void getResults(NXCPMessage request, NXCPMessage reply)
   {
      reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
      final int userId = request.getFieldAsInt32(NXCPCodes.VID_USER_ID);
      final UUID reportId = request.getFieldAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
      logger.debug("Loading report results for {} (user={})", reportId, userId);
      final List<ReportResult> list = server.getReportManager().listResults(reportId, userId);
      logger.debug("Got {} records", list.size());
      long fieldId = NXCPCodes.VID_ROW_DATA_BASE;
      for(ReportResult record : list)
      {
         record.fillMessage(reply, fieldId);
         fieldId += 10;
      }
      reply.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, list.size());
      reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
   }

   /**
    * Execute report
    * 
    * @param request request message
    * @param response response message
    */
   private void executeReport(NXCPMessage request, NXCPMessage response)
   {
      final ReportingJobConfiguration jobConfiguration;
      try
      {
         jobConfiguration = XMLTools.createFromXml(ReportingJobConfiguration.class, request.getFieldAsString(NXCPCodes.VID_EXECUTION_PARAMETERS));
      }
      catch(Exception e)
      {
         logger.error("Cannot parse report execution parameters", e);
         response.setFieldInt32(NXCPCodes.VID_RCC, RCC.INVALID_ARGUMENT);
         return;
      }

      final UUID jobId = getOrCreateJobId(request);
      final int userId = request.getFieldAsInt32(NXCPCodes.VID_USER_ID);
      final String idataView = request.getFieldAsString(NXCPCodes.VID_VIEW_NAME);
      final String authToken = request.getFieldAsString(NXCPCodes.VID_AUTH_TOKEN);
      requestObjectAccessSnapshotUpdate(userId);
      server.executeBackgroundTask(new Runnable() {
         @Override
         public void run()
         {
            server.getReportManager().execute(userId, authToken, jobId, jobConfiguration, idataView, Locale.US);
         }
      });
      response.setField(NXCPCodes.VID_TASK_ID, jobId);
      response.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
   }

   /**
    * Get job ID from message or create random one if not provided
    * 
    * @param request request message
    * @return job ID
    */
   private static UUID getOrCreateJobId(NXCPMessage request)
   {
      UUID jobId = request.getFieldAsUUID(NXCPCodes.VID_RS_JOB_ID);
      return (jobId != null) ? jobId : UUID.randomUUID();
   }

   /**
    * Render report result
    *
    * @param request request message
    * @param response response message
    * @return rendered document or null on failure
    */
   private File renderResult(NXCPMessage request, NXCPMessage response)
   {
      final UUID reportId = request.getFieldAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
      final UUID jobId = request.getFieldAsUUID(NXCPCodes.VID_TASK_ID);
      final int formatCode = request.getFieldAsInt32(NXCPCodes.VID_RENDER_FORMAT);
      final ReportRenderFormat format = ReportRenderFormat.valueOf(formatCode);
      final int userId = request.getFieldAsInt32(NXCPCodes.VID_USER_ID);
      File file = server.getReportManager().renderResult(reportId, jobId, userId, format);
      response.setFieldInt32(NXCPCodes.VID_RCC, (file != null) ? RCC.SUCCESS : RCC.IO_ERROR);
      return file;
   }

   /**
    * Delete report execution result.
    *
    * @param request request message
    * @param reply response message
    */
   private void deleteResult(NXCPMessage request, NXCPMessage reply)
   {
      final UUID reportId = request.getFieldAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
      final UUID jobId = request.getFieldAsUUID(NXCPCodes.VID_TASK_ID);
      reply.setFieldInt32(NXCPCodes.VID_RCC,
            server.getReportManager().deleteResult(reportId, jobId) ? RCC.SUCCESS : RCC.IO_ERROR);
      sendNotification(SessionNotification.RS_RESULTS_MODIFIED, 0);
   }

   /**
    * Send notification message
    */
   public void sendNotification(int code, int data)
   {
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_RS_NOTIFY);
      msg.setFieldInt32(NXCPCodes.VID_NOTIFICATION_CODE, code);
      msg.setFieldInt32(NXCPCodes.VID_NOTIFICATION_DATA, data);
      logger.debug("SEND: " + msg.toString());
      sendMessage(msg);
   }

   /**
    * Request core server to update object access snapshot for given user
    *
    * @param userId user ID
    */
   public void requestObjectAccessSnapshotUpdate(int userId)
   {
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_CREATE_OBJECT_ACCESS_SNAPSHOT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_CLASS, AbstractObject.OBJECT_NODE);
      msg.setFieldInt32(NXCPCodes.VID_USER_ID, userId);
      logger.debug("SEND: " + msg.toString());
      sendMessage(msg);
   }

   /**
    * Message processing result
    */
   private static class MessageProcessingResult
   {
      public NXCPMessage response;
      public File file;

      /**
       * @param response
       * @param file
       */
      public MessageProcessingResult(NXCPMessage response, File file)
      {
         this.response = response;
         this.file = file;
      }
   }
}
