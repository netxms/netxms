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
package org.netxms.reporting.services;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.UUID;
import org.netxms.base.CommonRCC;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.reporting.ReportRenderFormat;
import org.netxms.client.reporting.ReportingJobConfiguration;
import org.netxms.reporting.Server;
import org.netxms.reporting.model.ReportDefinition;
import org.netxms.reporting.model.ReportResult;
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
    * @param message input message
    * @return message processing result
    */
   public MessageProcessingResult processMessage(NXCPMessage message)
   {
      NXCPMessage reply = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, message.getMessageId());
      File file = null;
      switch(message.getMessageCode())
      {
         case NXCPCodes.CMD_ISC_CONNECT_TO_SERVICE: // ignore and reply "Ok"
         case NXCPCodes.CMD_KEEPALIVE:
            reply.setFieldInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
            break;
         case NXCPCodes.CMD_GET_NXCP_CAPS:
            reply = new NXCPMessage(NXCPCodes.CMD_NXCP_CAPS, message.getMessageId());
            reply.setControl(true);
            reply.setControlData(5); // NXCP version 5
            break;
         case NXCPCodes.CMD_RS_LIST_REPORTS:
            listReports(reply);
            break;
         case NXCPCodes.CMD_RS_GET_REPORT_DEFINITION:
            getReportDefinition(message, reply);
            break;
         case NXCPCodes.CMD_RS_EXECUTE_REPORT:
            executeReport(message, reply);
            break;
         case NXCPCodes.CMD_RS_LIST_RESULTS:
            listResults(message, reply);
            break;
         case NXCPCodes.CMD_RS_RENDER_RESULT:
            file = renderResult(message, reply);
            break;
         case NXCPCodes.CMD_RS_DELETE_RESULT:
            deleteResult(message, reply);
            break;
         default:
            reply.setFieldInt32(NXCPCodes.VID_RCC, CommonRCC.NOT_IMPLEMENTED);
            break;
      }
      return new MessageProcessingResult(reply, file);
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
      reply.setFieldInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
   }

   private void getReportDefinition(NXCPMessage request, NXCPMessage reply)
   {
      final UUID reportId = request.getFieldAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
      final String localeString = request.getFieldAsString(NXCPCodes.VID_LOCALE);
      final Locale locale = new Locale(localeString);
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
         reply.setFieldInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
         definition.fillMessage(reply);
      }
      else
      {
         reply.setFieldInt32(NXCPCodes.VID_RCC, CommonRCC.INTERNAL_ERROR);
      }
   }

   private void listResults(NXCPMessage request, NXCPMessage reply)
   {
      reply.setFieldInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
      final int userId = request.getFieldAsInt32(NXCPCodes.VID_USER_ID);
      final UUID reportId = request.getFieldAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
      logger.debug("Loading report results for {} (user={})", reportId, userId);
      final List<ReportResult> list = server.getReportManager().listResults(reportId, userId);
      logger.debug("Got {} records", list.size());
      long index = NXCPCodes.VID_ROW_DATA_BASE;
      int jobNum = 0;
      for(ReportResult record : list)
      {
         reply.setField(index++, record.getJobId());
         reply.setFieldInt32(index++, (int)(record.getExecutionTime().getTime() / 1000));
         reply.setFieldInt32(index++, record.getUserId());
         index += 7;
         jobNum++;
      }
      reply.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, jobNum);
      reply.setFieldInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
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
         jobConfiguration = ReportingJobConfiguration.createFromXml(request.getFieldAsString(NXCPCodes.VID_EXECUTION_PARAMETERS));
      }
      catch(Exception e)
      {
         logger.error("Cannot parse report execution parameters", e);
         response.setFieldInt32(NXCPCodes.VID_RCC, CommonRCC.INVALID_ARGUMENT);
         return;
      }

      final UUID jobId = getOrCreateJobId(request);
      final int userId = request.getFieldAsInt32(NXCPCodes.VID_USER_ID);
      final String idataView = request.getFieldAsString(NXCPCodes.VID_VIEW_NAME);
      requestObjectAccessSnapshotUpdate(userId);
      server.executeBackgroundTask(new Runnable() {
         @Override
         public void run()
         {
            server.getReportManager().execute(userId, jobId, jobConfiguration, idataView, Locale.US);
         }
      });
      response.setField(NXCPCodes.VID_JOB_ID, jobId);
      response.setFieldInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
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
      final UUID jobId = request.getFieldAsUUID(NXCPCodes.VID_JOB_ID);
      final int formatCode = request.getFieldAsInt32(NXCPCodes.VID_RENDER_FORMAT);
      final ReportRenderFormat format = ReportRenderFormat.valueOf(formatCode);
      File file = server.getReportManager().renderResult(reportId, jobId, format);
      response.setFieldInt32(NXCPCodes.VID_RCC, (file != null) ? CommonRCC.SUCCESS : CommonRCC.IO_ERROR);
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
      final UUID jobId = request.getFieldAsUUID(NXCPCodes.VID_JOB_ID);

      server.getReportManager().deleteResult(reportId, jobId);
      reply.setFieldInt32(NXCPCodes.VID_RCC, CommonRCC.SUCCESS);
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
