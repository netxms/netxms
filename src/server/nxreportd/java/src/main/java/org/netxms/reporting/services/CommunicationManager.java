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
import java.io.FileOutputStream;
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
   private FileOutputStream pendingFileStream;
   private File pendingTempFile;
   private String pendingFinalName;
   private long pendingFileRequestId;

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
   public void start(final Socket socket)
   {
      // Previous receiver must be gone before the new one starts, as both would use report package upload state
      shutdown();
      synchronized(mutex)
      {
         this.socket = socket;
         receiverThread = new Thread(new Runnable() {
            @Override
            public void run()
            {
               logger.info("Communication manager started");
               receiverThread(socket);
               logger.info("Communication manager stopped");
            }
         }, "Network Receiver");
         receiverThread.start();
      }
   }

   /**
    * Shutdown communication session. Receiver thread is joined without holding the lock, because the receiver needs it to
    * send replies.
    */
   public void shutdown()
   {
      Socket oldSocket;
      Thread oldThread;
      synchronized(mutex)
      {
         oldSocket = socket;
         oldThread = receiverThread;
         socket = null;
         receiverThread = null;
      }

      if (oldSocket != null)
      {
         try
         {
            oldSocket.close();
         }
         catch(IOException e)
         {
            logger.error("Error closing existing socket", e);
         }
      }
      if (oldThread != null)
      {
         try
         {
            oldThread.join();
         }
         catch(InterruptedException e)
         {
            logger.warn("Interrupted while waiting for receiver thread", e);
            Thread.currentThread().interrupt();
         }
      }
   }

   /**
    * Send message to core server over current connection.
    *
    * @param message message to send
    * @return true on success
    */
   public boolean sendMessage(NXCPMessage message)
   {
      Socket s;
      synchronized(mutex)
      {
         s = socket;
      }
      return sendMessage(s, message);
   }

   /**
    * Send message to core server over given connection. Message is dropped if that connection is no longer the current one,
    * so that reply to a request received over closed connection is not delivered to the new one.
    *
    * @param target connection to send message over
    * @param message message to send
    * @return true on success
    */
   private boolean sendMessage(Socket target, NXCPMessage message)
   {
      try
      {
         synchronized(mutex)
         {
            if ((target == null) || (target != socket))
            {
               logger.debug("Message with code 0x{} and ID {} dropped (connection closed)", Integer.toHexString(message.getMessageCode()), message.getMessageId());
               return false;
            }
            target.getOutputStream().write(message.createNXCPMessage(false));
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
    * @param target connection to send file over
    * @param requestId request ID
    * @param inputStream input stream for file
    * @throws IOException on file read error
    */
   private void sendFileStream(Socket target, long requestId, InputStream inputStream) throws IOException
   {
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_FILE_DATA, requestId);
      msg.setBinaryMessage(true);

      final byte[] buffer = new byte[FILE_BUFFER_SIZE];
      try
      {
         while(true)
         {
            final int bytesRead = inputStream.read(buffer);
            if (bytesRead < FILE_BUFFER_SIZE)
            {
               msg.setEndOfFile(true);
            }

            msg.setBinaryData(bytesRead == -1 ? new byte[0] : Arrays.copyOf(buffer, bytesRead));
            if (!sendMessage(target, msg))
               return;  // Connection lost, no point in sending the rest

            if (bytesRead < FILE_BUFFER_SIZE)
               break;
         }
      }
      catch(IOException e)
      {
         NXCPMessage abortMessage = new NXCPMessage(NXCPCodes.CMD_ABORT_FILE_TRANSFER, requestId);
         abortMessage.setBinaryMessage(true);
         sendMessage(target, abortMessage);
         throw e;
      }
   }

   /**
    * Receiver thread for communication session
    *
    * @param s connection to receive messages from
    */
   private void receiverThread(Socket s)
   {
      final NXCPMessageReceiver messageReceiver = new NXCPMessageReceiver(262144, 4194304);   // 256KB, 4MB
      try
      {
         while(!Thread.currentThread().isInterrupted())
         {
            try
            {
               final NXCPMessage message = messageReceiver.receiveMessage(s.getInputStream(), null);
               if (message == null)
                  continue;

               if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE)
               {
                  logger.debug("RECV: " + message.toString());
               }

               // Handle incoming file data for report package upload
               if ((message.getMessageCode() == NXCPCodes.CMD_FILE_DATA || message.getMessageCode() == NXCPCodes.CMD_ABORT_FILE_TRANSFER) && pendingFileStream != null &&
                     pendingFileRequestId == message.getMessageId())
               {
                  processIncomingFileData(message, s);
                  continue;
               }

               final NXCPMessage response = processMessage(message, s);
               if (response != null)
               {
                  if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE)
                  {
                     logger.debug("SEND: " + response.toString());
                  }
                  sendMessage(s, response);
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
      finally
      {
         if (pendingFileStream != null)
         {
            logger.warn("Upload of report package " + pendingFinalName + " interrupted by connection loss");
            discardPendingUpload();
         }
      }
   }

   /**
    * Process incoming message
    *
    * @param request input message
    * @param s connection request was received over
    * @return response message or null if response will be sent asynchronously
    */
   private NXCPMessage processMessage(NXCPMessage request, Socket s)
   {
      NXCPMessage reply = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());
      switch(request.getMessageCode())
      {
         case NXCPCodes.CMD_ISC_CONNECT_TO_SERVICE: // ignore and reply "Ok"
         case NXCPCodes.CMD_KEEPALIVE:
            reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
            break;
         case NXCPCodes.CMD_GET_NXCP_CAPS:
            reply.setMessageCode(NXCPCodes.CMD_NXCP_CAPS);
            reply.setControl(true);
            reply.setControlData(5 << 24); // NXCP version 5 in upper 8 bits, no capability flags
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
            renderResult(request, s);
            return null;
         case NXCPCodes.CMD_RS_DELETE_RESULT:
            deleteResult(request, reply);
            break;
         case NXCPCodes.CMD_RS_LIST_REPORT_PACKAGES:
            listPackages(reply);
            break;
         case NXCPCodes.CMD_RS_DELETE_REPORT_PACKAGE:
            deletePackage(request, reply);
            break;
         case NXCPCodes.CMD_RS_DEPLOY_REPORT_PACKAGE:
            prepareFileReceive(request, reply);
            break;
         default:
            reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.NOT_IMPLEMENTED);
            break;
      }
      return reply;
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
    * Render report result in background and send response followed by rendered document over the connection request was
    * received over. Rendering can take long time, and doing it on receiver thread would block all other requests.
    *
    * @param request request message
    * @param s connection request was received over
    */
   private void renderResult(NXCPMessage request, final Socket s)
   {
      final long requestId = request.getMessageId();
      final UUID reportId = request.getFieldAsUUID(NXCPCodes.VID_REPORT_DEFINITION);
      final UUID jobId = request.getFieldAsUUID(NXCPCodes.VID_TASK_ID);
      final ReportRenderFormat format = ReportRenderFormat.valueOf(request.getFieldAsInt32(NXCPCodes.VID_RENDER_FORMAT));
      final int userId = request.getFieldAsInt32(NXCPCodes.VID_USER_ID);
      server.executeBackgroundTask(new Runnable() {
         @Override
         public void run()
         {
            File file = server.getReportManager().renderResult(reportId, jobId, userId, format);
            NXCPMessage response = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, requestId);
            response.setFieldInt32(NXCPCodes.VID_RCC, (file != null) ? RCC.SUCCESS : RCC.IO_ERROR);
            logger.debug("SEND: " + response.toString());
            if (sendMessage(s, response) && (file != null))
            {
               logger.debug("Sending rendered file {}", file.getName());
               try (FileInputStream in = new FileInputStream(file))
               {
                  sendFileStream(s, requestId, in);
               }
               catch(IOException e)
               {
                  logger.error("Unexpected I/O exception while sending rendered file", e);
               }
            }
            if (file != null)
               file.delete();
         }
      });
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
    * List report packages in definitions directory with metadata.
    *
    * @param reply response message
    */
   private void listPackages(NXCPMessage reply)
   {
      List<Object[]> packages = server.getReportManager().listPackagesWithMetadata();
      reply.setFieldInt32(NXCPCodes.VID_INSTANCE_COUNT, packages.size());
      long fieldId = NXCPCodes.VID_INSTANCE_LIST_BASE;
      for(Object[] pkg : packages)
      {
         File f = (File)pkg[0];
         UUID guid = (UUID)pkg[1];
         String reportName = (String)pkg[2];
         boolean deployed = (Boolean)pkg[3];

         reply.setField(fieldId, f.getName());
         reply.setFieldInt64(fieldId + 1, f.length());
         reply.setFieldInt64(fieldId + 2, f.lastModified() / 1000);
         if (guid != null)
            reply.setField(fieldId + 3, guid);
         if (reportName != null)
            reply.setField(fieldId + 4, reportName);
         reply.setField(fieldId + 5, deployed);
         fieldId += 10;
      }
      reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
   }

   /**
    * Delete report package from definitions directory.
    *
    * @param request request message
    * @param reply response message
    */
   private void deletePackage(NXCPMessage request, NXCPMessage reply)
   {
      String fileName = request.getFieldAsString(NXCPCodes.VID_FILE_NAME);
      if (fileName == null || fileName.contains("/") || fileName.contains("\\") || fileName.contains(".."))
      {
         reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.INVALID_ARGUMENT);
         return;
      }

      File file = new File(server.getReportManager().getDefinitionsDirectory(), fileName);
      if (file.exists() && file.delete())
      {
         logger.info("Report package " + fileName + " deleted");
         reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
         sendNotification(SessionNotification.RS_DEFINITIONS_MODIFIED, 0);
      }
      else
      {
         logger.error("Failed to delete report package " + fileName);
         reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.IO_ERROR);
      }
   }

   /**
    * Prepare to receive report package file data.
    *
    * @param request request message
    * @param reply response message
    */
   private void prepareFileReceive(NXCPMessage request, NXCPMessage reply)
   {
      String fileName = request.getFieldAsString(NXCPCodes.VID_FILE_NAME);
      if (fileName == null || fileName.contains("/") || fileName.contains("\\") || fileName.contains(".."))
      {
         reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.INVALID_ARGUMENT);
         return;
      }

      String lc = fileName.toLowerCase();
      if (!lc.endsWith(".jar") && !lc.endsWith(".zip"))
      {
         reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.INVALID_ARGUMENT);
         return;
      }

      if (pendingFileStream != null)
      {
         logger.warn("Upload of report package " + pendingFinalName + " superseded by new upload request");
         discardPendingUpload();
      }

      try
      {
         File defDir = server.getReportManager().getDefinitionsDirectory();
         pendingTempFile = new File(defDir, fileName + ".tmp");
         pendingFinalName = fileName;
         pendingFileRequestId = request.getMessageId();
         pendingFileStream = new FileOutputStream(pendingTempFile);
         reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
         logger.info("Ready to receive report package " + fileName);
      }
      catch(IOException e)
      {
         logger.error("Cannot create file for report package upload", e);
         discardPendingUpload();
         reply.setFieldInt32(NXCPCodes.VID_RCC, RCC.IO_ERROR);
      }
   }

   /**
    * Process incoming file data for report package upload.
    *
    * @param msg incoming file data message
    * @param s connection message was received over
    */
   private void processIncomingFileData(NXCPMessage msg, Socket s)
   {
      if (msg.getMessageCode() == NXCPCodes.CMD_ABORT_FILE_TRANSFER)
      {
         logger.warn("Report package upload aborted");
         discardPendingUpload();
         return;
      }

      try
      {
         byte[] data = msg.getBinaryData();
         if (data != null && data.length > 0)
         {
            pendingFileStream.write(data);
         }

         if (msg.isEndOfFile())
         {
            pendingFileStream.close();
            pendingFileStream = null;

            // Rename from .tmp to final name to trigger FileMonitor
            File finalFile = new File(pendingTempFile.getParentFile(), pendingFinalName);
            if (finalFile.exists())
               finalFile.delete();
            if (pendingTempFile.renameTo(finalFile))
            {
               logger.info("Report package " + pendingFinalName + " received and deployed");
               NXCPMessage response = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, msg.getMessageId());
               response.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
               sendMessage(s, response);
               sendNotification(SessionNotification.RS_DEFINITIONS_MODIFIED, 0);
            }
            else
            {
               logger.error("Failed to rename temp file to " + pendingFinalName);
               pendingTempFile.delete();
               NXCPMessage response = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, msg.getMessageId());
               response.setFieldInt32(NXCPCodes.VID_RCC, RCC.IO_ERROR);
               sendMessage(s, response);
            }
            pendingTempFile = null;
            pendingFinalName = null;
         }
      }
      catch(IOException e)
      {
         logger.error("Error writing report package file data", e);
         discardPendingUpload();

         NXCPMessage response = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, msg.getMessageId());
         response.setFieldInt32(NXCPCodes.VID_RCC, RCC.IO_ERROR);
         sendMessage(s, response);
      }
   }

   /**
    * Discard incomplete report package upload, if any
    */
   private void discardPendingUpload()
   {
      if (pendingFileStream != null)
      {
         try
         {
            pendingFileStream.close();
         }
         catch(IOException e)
         {
         }
         pendingFileStream = null;
      }
      if (pendingTempFile != null)
      {
         pendingTempFile.delete();
         pendingTempFile = null;
      }
      pendingFinalName = null;
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
}
