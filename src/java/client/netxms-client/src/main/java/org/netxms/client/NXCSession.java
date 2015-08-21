/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.client;

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.Writer;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.security.GeneralSecurityException;
import java.security.Signature;
import java.security.SignatureException;
import java.security.cert.Certificate;
import java.security.cert.CertificateEncodingException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.netxms.base.CompatTools;
import org.netxms.base.EncryptionContext;
import org.netxms.base.GeoLocation;
import org.netxms.base.InetAddressEx;
import org.netxms.base.Logger;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPDataInputStream;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.netxms.base.NXCPMsgWaitQueue;
import org.netxms.base.NXCommon;
import org.netxms.client.agent.config.ConfigContent;
import org.netxms.client.agent.config.ConfigListElement;
import org.netxms.client.constants.AggregationFunction;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.client.constants.NodePollType;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.RCC;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ConditionDciInfo;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.DciPushData;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.client.datacollection.SimpleDciValue;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.client.datacollection.TransformationTestResult;
import org.netxms.client.datacollection.WinPerfObject;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.AlarmComment;
import org.netxms.client.events.Event;
import org.netxms.client.events.EventInfo;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.events.SyslogRecord;
import org.netxms.client.log.Log;
import org.netxms.client.maps.MapDCIInstance;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.mt.MappingTable;
import org.netxms.client.mt.MappingTableDescriptor;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.client.objects.AgentPolicyConfig;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServiceRoot;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.ClusterResource;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DashboardRoot;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.NetworkMapGroup;
import org.netxms.client.objects.NetworkMapRoot;
import org.netxms.client.objects.NetworkService;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.NodeLink;
import org.netxms.client.objects.PolicyGroup;
import org.netxms.client.objects.PolicyRoot;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceCheck;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.TemplateGroup;
import org.netxms.client.objects.TemplateRoot;
import org.netxms.client.objects.UnknownObject;
import org.netxms.client.objects.VPNConnector;
import org.netxms.client.objects.Zone;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.client.packages.PackageDeploymentListener;
import org.netxms.client.packages.PackageInfo;
import org.netxms.client.reporting.ReportDefinition;
import org.netxms.client.reporting.ReportRenderFormat;
import org.netxms.client.reporting.ReportResult;
import org.netxms.client.reporting.ReportingJob;
import org.netxms.client.server.AgentFile;
import org.netxms.client.server.ServerConsoleListener;
import org.netxms.client.server.ServerFile;
import org.netxms.client.server.ServerJob;
import org.netxms.client.server.ServerVariable;
import org.netxms.client.situations.Situation;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.client.snmp.SnmpTrapLogRecord;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.client.snmp.SnmpWalkListener;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.client.topology.FdbEntry;
import org.netxms.client.topology.NetworkPath;
import org.netxms.client.topology.Route;
import org.netxms.client.topology.VlanInfo;
import org.netxms.client.topology.WirelessStation;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.AuthCertificate;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;

/**
 * Communication session with NetXMS server.
 */
public class NXCSession
{
   // Various public constants
   public static final int DEFAULT_CONN_PORT = 4701;

   // Notification channels
   public static final int CHANNEL_EVENTS = 0x0001;
   public static final int CHANNEL_SYSLOG = 0x0002;
   public static final int CHANNEL_ALARMS = 0x0004;
   public static final int CHANNEL_OBJECTS = 0x0008;
   public static final int CHANNEL_SNMP_TRAPS = 0x0010;
   public static final int CHANNEL_AUDIT_LOG = 0x0020;
   public static final int CHANNEL_SITUATIONS = 0x0040;

   // Object sync options
   public static final int OBJECT_SYNC_NOTIFY = 0x0001;
   public static final int OBJECT_SYNC_WAIT = 0x0002;

   // Configuration import options
   public static final int CFG_IMPORT_REPLACE_EVENT_BY_CODE = 0x0001;
   public static final int CFG_IMPORT_REPLACE_EVENT_BY_NAME = 0x0002;

   // Address list IDs
   public static final int ADDRESS_LIST_DISCOVERY_TARGETS = 1;
   public static final int ADDRESS_LIST_DISCOVERY_FILTER = 2;

   // Server components
   public static final int SERVER_COMPONENT_DISCOVERY_MANAGER = 1;

   // Client types
   public static final int DESKTOP_CLIENT = 0;
   public static final int WEB_CLIENT = 1;
   public static final int MOBILE_CLIENT = 2;
   public static final int TABLET_CLIENT = 3;
   public static final int APPLICATION_CLIENT = 4;

   // Private constants
   private static final int CLIENT_CHALLENGE_SIZE = 256;
   private static final int MAX_DCI_DATA_ROWS = 200000;
   private static final int MAX_DCI_STRING_VALUE_LENGTH = 256;
   private static final int RECEIVED_FILE_TTL = 300000; // 300 seconds
   private static final int FILE_BUFFER_SIZE = 128 * 1024; // 128k

   // Internal synchronization objects
   private final Semaphore syncObjects = new Semaphore(1);
   private final Semaphore syncUserDB = new Semaphore(1);

   // Connection-related attributes
   private String connAddress;
   private int connPort;
   private boolean connUseEncryption;
   private String connClientInfo = "nxjclient/" + NXCommon.VERSION;
   private int clientType = DESKTOP_CLIENT;
   private String clientAddress = null;
   private boolean ignoreProtocolVersion = false;
   private String clientLanguage = "en";

   // Information about logged in user
   private int sessionId;
   private int userId;
   private String userName;
   private AuthenticationType authenticationMethod;
   private long userSystemRights;
   private boolean passwordExpired;

   // Internal communication data
   private Socket connSocket = null;
   private NXCPMsgWaitQueue msgWaitQueue = null;
   private ReceiverThread recvThread = null;
   private HousekeeperThread housekeeperThread = null;
   private AtomicLong requestId = new AtomicLong(1);
   private boolean isConnected = false;
   private boolean isDisconnected = false;
   private boolean serverConsoleConnected = false;
   private EncryptionContext encryptionContext = null;

   // Communication parameters
   private int defaultRecvBufferSize = 4194304; // Default is 4MB
   private int maxRecvBufferSize = 33554432;    // Max is 32MB
   private int commandTimeout = 30000; // Default is 30 sec

   // Notification listeners and queue
   private LinkedBlockingQueue<SessionNotification> notificationQueue = new LinkedBlockingQueue<SessionNotification>(8192);
   private Set<SessionListener> listeners = new HashSet<SessionListener>(0);
   private Set<ServerConsoleListener> consoleListeners = new HashSet<ServerConsoleListener>(0);
   
   // Message subscriptions
   private Map<MessageSubscription, MessageHandler> messageSubscriptions = new HashMap<MessageSubscription, MessageHandler>(0);

   // Received files
   private Map<Long, NXCReceivedFile> receivedFiles = new HashMap<Long, NXCReceivedFile>();
   
   // Received file updates(for file monitoring)
   private Map<String, String> recievedUpdates = new HashMap<String, String>();

   // Server information
   private ProtocolVersion protocolVersion;
   private String serverVersion = "(unknown)";
   private long serverId = 0;
   private String serverTimeZone;
   private byte[] serverChallenge = new byte[CLIENT_CHALLENGE_SIZE];
   private boolean zoningEnabled = false;
   private boolean helpdeskLinkActive = false;
   private String tileServerURL;
   private String dateFormat;
   private String timeFormat;
   private String shortTimeFormat;
   private int defaultDciRetentionTime;
   private int defaultDciPollingInterval;
   private boolean strictAlarmStatusFlow;
   private boolean timedAlarmAckEnabled;
   private int minViewRefreshInterval;
   private long serverTime = System.currentTimeMillis();
   private long serverTimeRecvTime = System.currentTimeMillis();
   private int alarmListDisplayLimit;

   // Objects
   private Map<Long, AbstractObject> objectList = new HashMap<Long, AbstractObject>();
   private boolean objectsSynchronized = false;

   // Users
   private Map<Long, AbstractUserObject> userDB = new HashMap<Long, AbstractUserObject>();

   // Event templates
   private Map<Long, EventTemplate> eventTemplates = new HashMap<Long, EventTemplate>();
   private boolean eventTemplatesNeedSync = false;
   
   /**
    * Message subscription class
    */
   final protected class MessageSubscription
   {
      protected int messageCode;
      protected long messageId;
      
      /**
       * @param messageCode
       * @param messageId
       * @param handler
       */
      protected MessageSubscription(int messageCode, long messageId)
      {
         this.messageCode = messageCode;
         this.messageId = messageId;
      }
      
      /* (non-Javadoc)
       * @see java.lang.Object#hashCode()
       */
      @Override
      public int hashCode()
      {
         final int prime = 31;
         int result = 1;
         result = prime * result + messageCode;
         result = prime * result + (int)(messageId ^ (messageId >>> 32));
         return result;
      }

      /* (non-Javadoc)
       * @see java.lang.Object#equals(java.lang.Object)
       */
      @Override
      public boolean equals(Object obj)
      {
         if (this == obj)
            return true;
         if (obj == null)
            return false;
         if (getClass() != obj.getClass())
            return false;
         MessageSubscription other = (MessageSubscription)obj;
         if (messageCode != other.messageCode)
            return false;
         if (messageId != other.messageId)
            return false;
         return true;
      }
   }

   /**
    * Receiver thread for NXCSession
    */
   private class ReceiverThread extends Thread
   {
      ReceiverThread()
      {
         setDaemon(true);
         start();
      }

      @Override
      public void run()
      {
         final NXCPMessageReceiver receiver = new NXCPMessageReceiver(defaultRecvBufferSize, maxRecvBufferSize);
         InputStream in;

         try
         {
            in = connSocket.getInputStream();
         }
         catch(IOException e)
         {
            return; // Stop receiver thread if input stream cannot be obtained
         }

         while(connSocket.isConnected())
         {
            try
            {
               NXCPMessage msg = receiver.receiveMessage(in, encryptionContext);
               switch(msg.getMessageCode())
               {
                  case NXCPCodes.CMD_REQUEST_SESSION_KEY:
                     setupEncryption(msg);
                     break;
                  case NXCPCodes.CMD_KEEPALIVE:
                     serverTime = msg.getFieldAsInt64(NXCPCodes.VID_TIMESTAMP) * 1000;
                     serverTimeRecvTime = System.currentTimeMillis();
                     break;
                  case NXCPCodes.CMD_OBJECT:
                  case NXCPCodes.CMD_OBJECT_UPDATE:
                     if (!msg.getFieldAsBoolean(NXCPCodes.VID_IS_DELETED))
                     {
                        final AbstractObject obj = createObjectFromMessage(msg);
                        synchronized(objectList)
                        {
                           objectList.put(obj.getObjectId(), obj);
                        }
                        if (msg.getMessageCode() == NXCPCodes.CMD_OBJECT_UPDATE)
                        {
                           sendNotification(new SessionNotification(SessionNotification.OBJECT_CHANGED, obj.getObjectId(), obj));
                        }
                     }
                     else
                     {
                        long objectId = msg.getFieldAsInt32(NXCPCodes.VID_OBJECT_ID);
                        synchronized(objectList)
                        {
                           objectList.remove(objectId);
                        }
                        sendNotification(new SessionNotification(SessionNotification.OBJECT_DELETED, objectId));
                     }
                     break;
                  case NXCPCodes.CMD_OBJECT_LIST_END:
                     completeSync(syncObjects);
                     break;
                  case NXCPCodes.CMD_USER_DATA:
                     final User user = new User(msg);
                     synchronized(userDB)
                     {
                        if (user.isDeleted())
                        {
                           userDB.remove(user.getId());
                        }
                        else
                        {
                           userDB.put(user.getId(), user);
                        }
                     }
                     break;
                  case NXCPCodes.CMD_GROUP_DATA:
                     final UserGroup group = new UserGroup(msg);
                     synchronized(userDB)
                     {
                        if (group.isDeleted())
                        {
                           userDB.remove(group.getId());
                        }
                        else
                        {
                           userDB.put(group.getId(), group);
                        }
                     }
                     break;
                  case NXCPCodes.CMD_USER_DB_EOF:
                     completeSync(syncUserDB);
                     break;
                  case NXCPCodes.CMD_USER_DB_UPDATE:
                     processUserDBUpdate(msg);
                     break;
                  case NXCPCodes.CMD_ALARM_UPDATE:
                     sendNotification(new SessionNotification(
                        msg.getFieldAsInt32(NXCPCodes.VID_NOTIFICATION_CODE) + SessionNotification.NOTIFY_BASE,
                        new Alarm(msg)));
                     break;
                  case NXCPCodes.CMD_JOB_CHANGE_NOTIFICATION:
                     sendNotification(new SessionNotification(SessionNotification.JOB_CHANGE, new ServerJob(msg)));
                     break;
                  case NXCPCodes.CMD_FILE_DATA:
                     processFileData(msg);
                     break;
                  case NXCPCodes.CMD_FILE_MONITORING:
                     processFileTail(msg);
                     break;
                  case NXCPCodes.CMD_ABORT_FILE_TRANSFER:
                     processFileTransferError(msg);
                     break;
                  case NXCPCodes.CMD_NOTIFY:
                     processNotificationMessage(msg, true);
                     break;
                  case NXCPCodes.CMD_RS_NOTIFY:
                     processNotificationMessage(msg, false);
                     break;
                  case NXCPCodes.CMD_EVENTLOG_RECORDS:
                     processNewEvents(msg);
                     break;
                  case NXCPCodes.CMD_TRAP_LOG_RECORDS:
                     processNewTraps(msg);
                     break;
                  case NXCPCodes.CMD_SYSLOG_RECORDS:
                     processSyslogRecords(msg);
                     break;
                  case NXCPCodes.CMD_ACTION_DB_UPDATE:
                     processActionConfigChange(msg);
                     break;
                  case NXCPCodes.CMD_EVENT_DB_UPDATE:
                     processEventConfigChange(msg);
                     break;
                  case NXCPCodes.CMD_TRAP_CFG_UPDATE:
                     processTrapConfigChange(msg);
                     break;
                  case NXCPCodes.CMD_SITUATION_CHANGE:
                     processSituationChange(msg);
                     break;
                  case NXCPCodes.CMD_ADM_MESSAGE:
                     processConsoleOutput(msg);
                     break;
                  case NXCPCodes.CMD_IMAGE_LIBRARY_UPDATE:
                     processImageLibraryUpdate(msg);
                     break;
                  case NXCPCodes.CMD_GRAPH_UPDATE:
                     GraphSettings graph = new GraphSettings(msg, NXCPCodes.VID_GRAPH_LIST_BASE);
                     sendNotification(new SessionNotification(SessionNotification.PREDEFINED_GRAPHS_CHANGED, graph.getId(), graph));
                     break;
                  default:
                     // Check subscriptions
                     synchronized(messageSubscriptions)
                     {
                        MessageSubscription s = new MessageSubscription(msg.getMessageCode(), msg.getMessageId());
                        MessageHandler handler = messageSubscriptions.get(s);
                        if (handler != null)
                        { 
                           if (handler.processMessage(msg))
                              msg = null;
                           if (handler.isComplete())
                              messageSubscriptions.remove(s);
                           else
                              handler.setLastMessageTimestamp(System.currentTimeMillis());
                        }
                     }
                     if (msg != null)
                     {
                        if (msg.getMessageCode() >= 0x1000)
                        {
                           // Custom message
                           sendNotification(new SessionNotification(SessionNotification.CUSTOM_MESSAGE, msg));
                        }
                        msgWaitQueue.putMessage(msg);
                     }
                     break;
               }
            }
            catch(IOException e)
            {
               break; // Stop on read errors
            }
            catch(NXCPException e)
            {
               if (e.getErrorCode() == NXCPException.SESSION_CLOSED) break;
            }
            catch(NXCException e)
            {
               if (e.getErrorCode() == RCC.ENCRYPTION_ERROR) break;
            }
         }
      }

      /**
       * Process server console output
       *
       * @param msg notification message
       */
      private void processConsoleOutput(NXCPMessage msg)
      {
         final String text = msg.getFieldAsString(NXCPCodes.VID_MESSAGE);
         synchronized(consoleListeners)
         {
            for(ServerConsoleListener l : consoleListeners)
            {
               l.onConsoleOutput(text);
            }
         }
      }

      /**
       * Process event notification messages.
       *
       * @param msg NXCP message
       */
      private void processNewEvents(final NXCPMessage msg)
      {
         int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
         int order = msg.getFieldAsInt32(NXCPCodes.VID_RECORDS_ORDER);
         long varId = NXCPCodes.VID_EVENTLOG_MSG_BASE;
         for(int i = 0; i < count; i++)
         {
            Event event = new Event(msg, varId);
            sendNotification(new SessionNotification(SessionNotification.NEW_EVENTLOG_RECORD, order, event));
         }
      }

      /**
       * Process SNMP trap notification messages.
       *
       * @param msg NXCP message
       */
      private void processNewTraps(final NXCPMessage msg)
      {
         int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
         int order = msg.getFieldAsInt32(NXCPCodes.VID_RECORDS_ORDER);
         long varId = NXCPCodes.VID_TRAP_LOG_MSG_BASE;
         for(int i = 0; i < count; i++)
         {
            SnmpTrapLogRecord trap = new SnmpTrapLogRecord(msg, varId);
            sendNotification(new SessionNotification(SessionNotification.NEW_SNMP_TRAP, order, trap));
         }
      }

      /**
       * Process syslog messages.
       *
       * @param msg NXCP message
       */
      private void processSyslogRecords(final NXCPMessage msg)
      {
         int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
         int order = msg.getFieldAsInt32(NXCPCodes.VID_RECORDS_ORDER);
         long varId = NXCPCodes.VID_SYSLOG_MSG_BASE;
         for(int i = 0; i < count; i++)
         {
            SyslogRecord record = new SyslogRecord(msg, varId);
            sendNotification(new SessionNotification(SessionNotification.NEW_SYSLOG_RECORD, order, record));
         }
      }

      /**
       * Process CMD_NOTIFY message
       *
       * @param msg NXCP message
       */
      private void processNotificationMessage(final NXCPMessage msg, boolean shiftCode)
      {
         int code = msg.getFieldAsInt32(NXCPCodes.VID_NOTIFICATION_CODE);
         if (shiftCode)
            code += SessionNotification.NOTIFY_BASE;
         long data = msg.getFieldAsInt64(NXCPCodes.VID_NOTIFICATION_DATA);
         
         switch(code)
         {
            case SessionNotification.ALARM_STATUS_FLOW_CHANGED:
               strictAlarmStatusFlow = ((int)data != 0);
               break;
            case SessionNotification.RELOAD_EVENT_DB:
               if (eventTemplatesNeedSync)
               {
                  resyncEventTemplates();
               }
               break;
         }
         
         sendNotification(new SessionNotification(code, data));
      }
      
      /**
       * Process CMD_FILE_MONITORING message
       *
       * @param msg NXCP message
       */
      private void processFileTail(final NXCPMessage msg)
      {
         String fileName = msg.getFieldAsString(NXCPCodes.VID_FILE_NAME);
         String fileContent = msg.getFieldAsString(NXCPCodes.VID_FILE_DATA);
         
         synchronized(recievedUpdates)
         {
            recievedUpdates.put(fileName, fileContent);
            recievedUpdates.notifyAll();
         }
      }

      /**
       * Process file data
       *
       * @param msg
       */
      private void processFileData(final NXCPMessage msg)
      {
         long id = msg.getMessageId();
         NXCReceivedFile file;
         synchronized(receivedFiles)
         {
            file = receivedFiles.get(id);
            if (file == null)
            {
               file = new NXCReceivedFile(id);
               receivedFiles.put(id, file);
            }
         }
         file.writeData(msg.getBinaryData());
         if (msg.isEndOfFile())
         {
            file.close();
            synchronized(receivedFiles)
            {
               receivedFiles.notifyAll();
            }
         }
      }

      /**
       * Process file transfer error
       *
       * @param msg
       */
      private void processFileTransferError(final NXCPMessage msg)
      {
         long id = msg.getMessageId();
         NXCReceivedFile file;
         synchronized(receivedFiles)
         {
            file = receivedFiles.get(id);
            if (file == null)
            {
               file = new NXCReceivedFile(id);
               receivedFiles.put(id, file);
            }
            file.close();
            receivedFiles.notifyAll();
         }
      }

      /**
       * Process updates in user database
       *
       * @param msg Notification message
       */
      private void processUserDBUpdate(final NXCPMessage msg)
      {
         final int code = msg.getFieldAsInt32(NXCPCodes.VID_UPDATE_TYPE);
         final long id = msg.getFieldAsInt64(NXCPCodes.VID_USER_ID);

         AbstractUserObject object = null;
         switch(code)
         {
            case SessionNotification.USER_DB_OBJECT_CREATED:
            case SessionNotification.USER_DB_OBJECT_MODIFIED:
               object = ((id & 0x80000000) != 0) ? new UserGroup(msg) : new User(msg);
               synchronized(userDB)
               {
                  userDB.put(id, object);
               }
               break;
            case SessionNotification.USER_DB_OBJECT_DELETED:
               synchronized(userDB)
               {
                  object = userDB.get(id);
                  if (object != null)
                  {
                     userDB.remove(id);
                  }
               }
               break;
         }

         // Send notification if changed object was found in local database copy
         // or added to it and notification code was known
         if (object != null) sendNotification(new SessionNotification(SessionNotification.USER_DB_CHANGED, code, object));
      }

      /**
       * Process server notification on SNMP trap configuration change
       *
       * @param msg notification message
       */
      private void processTrapConfigChange(final NXCPMessage msg)
      {
         int code = msg.getFieldAsInt32(NXCPCodes.VID_NOTIFICATION_CODE) + SessionNotification.NOTIFY_BASE;
         long id = msg.getFieldAsInt64(NXCPCodes.VID_TRAP_ID);
         SnmpTrap trap = (code != SessionNotification.TRAP_CONFIGURATION_DELETED) ? new SnmpTrap(msg) : null;
         sendNotification(new SessionNotification(code, id, trap));
      }

      /**
       * Process server notification on situation object change
       *
       * @param msg notification message
       */
      private void processSituationChange(final NXCPMessage msg)
      {
         int code = msg.getFieldAsInt32(NXCPCodes.VID_NOTIFICATION_CODE) + SessionNotification.SITUATION_BASE;
         Situation s = new Situation(msg);
         sendNotification(new SessionNotification(code, s.getId(), s));
      }

      /**
       * Process server notification on action configuration change
       *
       * @param msg notification message
       */
      private void processActionConfigChange(final NXCPMessage msg)
      {
         int code = msg.getFieldAsInt32(NXCPCodes.VID_NOTIFICATION_CODE) + SessionNotification.NOTIFY_BASE;
         long id = msg.getFieldAsInt64(NXCPCodes.VID_ACTION_ID);
         ServerAction action = (code != SessionNotification.ACTION_DELETED) ? new ServerAction(msg) : null;
         sendNotification(new SessionNotification(code, id, action));
      }

      /**
       * Process server notification on event configuration change
       *
       * @param msg notification message
       */
      private void processEventConfigChange(final NXCPMessage msg)
      {
         int code = msg.getFieldAsInt32(NXCPCodes.VID_NOTIFICATION_CODE) + SessionNotification.NOTIFY_BASE;
         long eventCode = msg.getFieldAsInt64(NXCPCodes.VID_EVENT_CODE);
         EventTemplate et = (code != SessionNotification.EVENT_TEMPLATE_DELETED) ? new EventTemplate(msg) : null;
         if (eventTemplatesNeedSync)
         {
            synchronized(eventTemplates)
            {
               if (code == SessionNotification.EVENT_TEMPLATE_DELETED)
               {
                  eventTemplates.remove(eventCode);
               }
               else
               {
                  eventTemplates.put(eventCode, et);
               }
            }
         }
         sendNotification(new SessionNotification(code, eventCode, et));
      }

      /**
       * Process server notification on image library change
       *
       * @param msg notification message
       */
      public void processImageLibraryUpdate(NXCPMessage msg)
      {
         final UUID imageGuid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
         final int flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
         sendNotification(new SessionNotification(SessionNotification.IMAGE_LIBRARY_CHANGED,
            flags == 0 ? SessionNotification.IMAGE_UPDATED : SessionNotification.IMAGE_DELETED, imageGuid));
      }
   }

   /**
    * Housekeeper thread - cleans received files, etc.
    *
    * @author Victor
    */
   private class HousekeeperThread extends Thread
   {
      private boolean stopFlag = false;

      HousekeeperThread()
      {
         setDaemon(true);
         start();
      }

      /*
       * (non-Javadoc)
       * 
       * @see java.lang.Thread#run()
       */
      @Override
      public void run()
      {
         while(!stopFlag)
         {
            try
            {
               sleep(1000);
            }
            catch(InterruptedException e)
            {
            }

            long currTime = System.currentTimeMillis();

            // Check for old entries in received files
            synchronized(receivedFiles)
            {
               Iterator<NXCReceivedFile> it = receivedFiles.values().iterator();
               while(it.hasNext())
               {
                  NXCReceivedFile file = it.next();
                  if (file.getTimestamp() + RECEIVED_FILE_TTL < currTime)
                  {
                     file.getFile().delete();
                     it.remove();
                  }
               }
            }
            
            // Check timeouts on message subscriptions
            synchronized(messageSubscriptions)
            {
               Iterator<Entry<MessageSubscription, MessageHandler>> it = messageSubscriptions.entrySet().iterator();
               while(it.hasNext())
               {
                  Entry<MessageSubscription, MessageHandler> e = it.next();
                  MessageHandler h = e.getValue();
                  if (currTime - h.getLastMessageTimestamp() > h.getMessageWaitTimeout())
                  {
                     h.setTimeout();
                     h.setComplete();
                     it.remove();
                  }
               }
            }
         }
      }

      /**
       * @param stopFlag the stopFlag to set
       */
      public void setStopFlag(boolean stopFlag)
      {
         this.stopFlag = stopFlag;
      }
   }
   
   /**
    * Notification processor
    */
   private class NotificationProcessor extends Thread
   {
      private SessionListener[] cachedListenerList = new SessionListener[0];
      
      NotificationProcessor()
      {
         setName("Session Notification Processor");
         setDaemon(true);
         start();
      }

      /* (non-Javadoc)
       * @see java.lang.Thread#run()
       */
      @Override
      public void run()
      {
         while(true)
         {
            SessionNotification n;
            try
            {
               n = notificationQueue.take();
            }
            catch(InterruptedException e)
            {
               continue;
            }
            
            if (n.getCode() == SessionNotification.STOP_PROCESSING_THREAD)
               break;
            
            if (n.getCode() == SessionNotification.UPDATE_LISTENER_LIST)
            {
               synchronized(listeners)
               {
                  cachedListenerList = listeners.toArray(new SessionListener[listeners.size()]);
               }
               continue;
            }

            // loop must be on listeners set copy to prevent 
            // possible deadlock when one of the listeners calls 
            // syncExec on UI thread while UI thread trying to add
            // new listener and stays locked inside addListener
            for(SessionListener l : cachedListenerList)
            {
               try
               {
                  l.notificationHandler(n);
               }
               catch(Exception e)
               {
                  Logger.error("NXCSession.NotificationProcessor", "Unhandled exception in notification handler", e);
               }
            }
         }
      }
   }

   /**
    * @param connAddress
    * @param connLoginName
    * @param connPassword
    */
   public NXCSession(String connAddress)
   {
      this(connAddress, DEFAULT_CONN_PORT, false);
   }

   /**
    * @param connAddress
    * @param connPort
    * @param connLoginName
    * @param connPassword
    */
   public NXCSession(String connAddress, int connPort)
   {
      this(connAddress, connPort, false);
   }

   /**
    * @param connAddress
    * @param connPort
    * @param connLoginName
    * @param connPassword
    * @param connUseEncryption
    */
   public NXCSession(String connAddress, int connPort, boolean connUseEncryption)
   {
      this.connAddress = connAddress;
      this.connPort = connPort;
      this.connUseEncryption = connUseEncryption;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#finalize()
    */
   @Override
   protected void finalize() throws Throwable
   {
      try
      {
         disconnect();
      }
      finally
      {
         super.finalize();
      }
   }

   /**
    * Create custom object from NXCP message. May be overridden by derived classes to create custom
    * NetXMS objects. This method called before standard object creation, so it can be used for
    * overriding standard object classes. If this method returns null, standard object
    * creation mechanism will be used. Default implementation will always return null.
    *
    * @param objectClass NetXMS object class ID
    * @param msg         Source NXCP message
    * @return NetXMS object or null if object cannot be created
    */
   protected AbstractObject createCustomObjectFromMessage(int objectClass, NXCPMessage msg)
   {
      return null;
   }

   /**
    * Create object from message
    *
    * @param msg Source NXCP message
    * @return NetXMS object
    */
   private AbstractObject createObjectFromMessage(NXCPMessage msg)
   {
      final int objectClass = msg.getFieldAsInt32(NXCPCodes.VID_OBJECT_CLASS);

      AbstractObject object = createCustomObjectFromMessage(objectClass, msg);
      if (object != null) 
         return object;

      switch(objectClass)
      {
         case AbstractObject.OBJECT_ACCESSPOINT:
            object = new AccessPoint(msg, this);
            break;
         case AbstractObject.OBJECT_AGENTPOLICY:
            object = new AgentPolicy(msg, this);
            break;
         case AbstractObject.OBJECT_AGENTPOLICY_CONFIG:
            object = new AgentPolicyConfig(msg, this);
            break;
         case AbstractObject.OBJECT_BUSINESSSERVICE:
            object = new BusinessService(msg, this);
            break;
         case AbstractObject.OBJECT_BUSINESSSERVICEROOT:
            object = new BusinessServiceRoot(msg, this);
            break;
         case AbstractObject.OBJECT_CLUSTER:
            object = new Cluster(msg, this);
            break;
         case AbstractObject.OBJECT_CONDITION:
            object = new Condition(msg, this);
            break;
         case AbstractObject.OBJECT_CONTAINER:
            object = new Container(msg, this);
            break;
         case AbstractObject.OBJECT_DASHBOARD:
            object = new Dashboard(msg, this);
            break;
         case AbstractObject.OBJECT_DASHBOARDROOT:
            object = new DashboardRoot(msg, this);
            break;
         case AbstractObject.OBJECT_INTERFACE:
            object = new Interface(msg, this);
            break;
         case AbstractObject.OBJECT_MOBILEDEVICE:
            object = new MobileDevice(msg, this);
            break;
         case AbstractObject.OBJECT_NETWORK:
            object = new EntireNetwork(msg, this);
            break;
         case AbstractObject.OBJECT_NETWORKMAP:
            object = new NetworkMap(msg, this);
            break;
         case AbstractObject.OBJECT_NETWORKMAPGROUP:
            object = new NetworkMapGroup(msg, this);
            break;
         case AbstractObject.OBJECT_NETWORKMAPROOT:
            object = new NetworkMapRoot(msg, this);
            break;
         case AbstractObject.OBJECT_NETWORKSERVICE:
            object = new NetworkService(msg, this);
            break;
         case AbstractObject.OBJECT_NODE:
            object = new Node(msg, this);
            break;
         case AbstractObject.OBJECT_NODELINK:
            object = new NodeLink(msg, this);
            break;
         case AbstractObject.OBJECT_POLICYGROUP:
            object = new PolicyGroup(msg, this);
            break;
         case AbstractObject.OBJECT_POLICYROOT:
            object = new PolicyRoot(msg, this);
            break;
         case AbstractObject.OBJECT_RACK:
            object = new Rack(msg, this);
            break;
         case AbstractObject.OBJECT_SERVICEROOT:
            object = new ServiceRoot(msg, this);
            break;
         case AbstractObject.OBJECT_SLMCHECK:
            object = new ServiceCheck(msg, this);
            break;
         case AbstractObject.OBJECT_SUBNET:
            object = new Subnet(msg, this);
            break;
         case AbstractObject.OBJECT_TEMPLATE:
            object = new Template(msg, this);
            break;
         case AbstractObject.OBJECT_TEMPLATEGROUP:
            object = new TemplateGroup(msg, this);
            break;
         case AbstractObject.OBJECT_TEMPLATEROOT:
            object = new TemplateRoot(msg, this);
            break;
         case AbstractObject.OBJECT_VPNCONNECTOR:
            object = new VPNConnector(msg, this);
            break;
         case AbstractObject.OBJECT_ZONE:
            object = new Zone(msg, this);
            break;
         default:
            object = new GenericObject(msg, this);
            break;
      }

      return object;
   }

   /**
    * Setup encryption
    *
    * @param msg CMD_REQUEST_SESSION_KEY message
    * @throws IOException
    * @throws NXCException
    */
   private void setupEncryption(NXCPMessage msg) throws IOException, NXCException
   {
      final NXCPMessage response = new NXCPMessage(NXCPCodes.CMD_SESSION_KEY, msg.getMessageId());
      response.setEncryptionDisabled(true);

      try
      {
         encryptionContext = EncryptionContext.createInstance(msg);
         response.setField(NXCPCodes.VID_SESSION_KEY, encryptionContext.getEncryptedSessionKey(msg));
         response.setField(NXCPCodes.VID_SESSION_IV, encryptionContext.getEncryptedIv(msg));
         response.setFieldInt16(NXCPCodes.VID_CIPHER, encryptionContext.getCipher());
         response.setFieldInt16(NXCPCodes.VID_KEY_LENGTH, encryptionContext.getKeyLength());
         response.setFieldInt16(NXCPCodes.VID_IV_LENGTH, encryptionContext.getIvLength());
         response.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
         Logger.debug("NXCSession.setupEncryption", "Cipher selected: " + EncryptionContext.getCipherName(encryptionContext.getCipher()));
      }
      catch(Exception e)
      {
         response.setFieldInt32(NXCPCodes.VID_RCC, RCC.NO_CIPHERS);
      }

      sendMessage(response);
   }
   
   /**
    * Wait for synchronization completion
    */
   private void waitForSync(final Semaphore syncObject, final int timeout) throws NXCException
   {
      if (timeout == 0)
      {
         syncObject.acquireUninterruptibly();
      }
      else
      {
         long actualTimeout = timeout;
         boolean success = false;

         while(actualTimeout > 0)
         {
            long startTime = System.currentTimeMillis();
            try
            {
               if (syncObjects.tryAcquire(actualTimeout, TimeUnit.MILLISECONDS))
               {
                  success = true;
                  syncObjects.release();
                  break;
               }
            }
            catch(InterruptedException e)
            {
            }
            actualTimeout -= System.currentTimeMillis() - startTime;
         }
         if (!success) throw new NXCException(RCC.TIMEOUT);
      }
   }

   /**
    * Report synchronization completion
    *
    * @param syncObject Synchronization object
    */
   private void completeSync(final Semaphore syncObject)
   {
      syncObject.release();
   }

   /**
    * Add notification listener
    * 
    * @param listener Listener to add
    */
   public void addListener(SessionListener listener)
   {
      boolean changed;
      synchronized(listeners)
      {
         changed = listeners.add(listener);
      }
      if (changed)
         notificationQueue.offer(new SessionNotification(SessionNotification.UPDATE_LISTENER_LIST));
   }

   /**
    * Remove notification listener
    * 
    * @param listener Listener to remove
    */
   public void removeListener(SessionListener listener)
   {
      boolean changed;
      synchronized(listeners)
      {
         changed = listeners.remove(listener);
      }
      if (changed)
         notificationQueue.offer(new SessionNotification(SessionNotification.UPDATE_LISTENER_LIST));
   }

   /**
    * Add server console listener
    *
    * @param listener
    */
   public void addConsoleListener(ServerConsoleListener listener)
   {
      synchronized(consoleListeners)
      {
         consoleListeners.add(listener);
      }
   }

   /**
    * Remove server console listener
    *
    * @param listener
    */
   public void removeConsoleListener(ServerConsoleListener listener)
   {
      synchronized(consoleListeners)
      {
         consoleListeners.remove(listener);
      }
   }
   
   /**
    * Subscribe to specific messages
    * 
    * @param messageCode
    * @param messageId
    * @param handler
    */
   public void addMessageSubscription(int messageCode, long messageId, MessageHandler handler)
   {
      synchronized(messageSubscriptions)
      {
         messageSubscriptions.put(new MessageSubscription(messageCode, messageId), handler);
      }
   }
   
   /**
    * @param messageCode
    * @param messageId
    */
   public void removeMessageSubscription(int messageCode, long messageId)
   {
      synchronized(messageSubscriptions)
      {
         messageSubscriptions.remove(new MessageSubscription(messageCode, messageId));
      }
   }
   
   /**
    * Call notification handlers on all registered listeners
    *
    * @param n Notification object
    */
   protected void sendNotification(SessionNotification n)
   {
      if (!notificationQueue.offer(n))
      {
         Logger.debug("NXCSession.sendNotification", "Notification processing queue is full");
      }
   }

   /**
    * Send message to server
    *
    * @param msg Message to sent
    * @throws IOException  in case of socket communication failure
    * @throws NXCException in case of encryption error
    */
   public synchronized void sendMessage(final NXCPMessage msg) throws IOException, NXCException
   {
      if (connSocket == null)
      {
         throw new IllegalStateException("Not connected to the server. Did you forgot to call connect() first?");
      }
      final OutputStream outputStream = connSocket.getOutputStream();
      byte[] message;
      if ((encryptionContext != null) && !msg.isEncryptionDisabled())
      {
         try
         {
            message = encryptionContext.encryptMessage(msg);
         }
         catch(GeneralSecurityException e)
         {
            throw new NXCException(RCC.ENCRYPTION_ERROR);
         }
      }
      else
      {
         message = msg.createNXCPMessage();
      }
      outputStream.write(message);
   }

   /**
    * Send file over CSCP
    *
    * @param requestId
    * @param file      source file to be sent
    * @throws IOException
    * @throws NXCException
    */
   protected void sendFile(final long requestId, final File file, ProgressListener listener)
      throws IOException, NXCException
   {
      if (listener != null) 
         listener.setTotalWorkAmount(file.length());
      final InputStream inputStream = new BufferedInputStream(new FileInputStream(file));
      sendFileStream(requestId, inputStream, listener);
      inputStream.close();
   }

   /**
    * Send block of data as binary message
    *
    * @param requestId
    * @param data
    * @throws IOException
    * @throws NXCException
    */
   protected void sendFile(final long requestId, final byte[] data, ProgressListener listener) throws IOException, NXCException
   {
      if (listener != null) 
         listener.setTotalWorkAmount(data.length);
      final InputStream inputStream = new ByteArrayInputStream(data);
      sendFileStream(requestId, inputStream, listener);
      inputStream.close();
   }

   /**
    * Send binary message, data loaded from provided input stream and splitted
    * into chunks of {@value FILE_BUFFER_SIZE} bytes
    *
    * @param requestId
    * @param inputStream
    * @throws IOException
    * @throws NXCException
    */
   private void sendFileStream(final long requestId, final InputStream inputStream, ProgressListener listener) throws IOException, NXCException
   {
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_FILE_DATA, requestId);
      msg.setBinaryMessage(true);

      boolean success = false;
      final byte[] buffer = new byte[FILE_BUFFER_SIZE];
      long bytesSent = 0;
      while(true)
      {         
         final int bytesRead = inputStream.read(buffer);
         if (bytesRead < FILE_BUFFER_SIZE)
         {
            msg.setEndOfFile(true);
         }

         msg.setBinaryData(bytesRead  == -1 ? new byte[0] : CompatTools.arrayCopy(buffer, bytesRead));
         sendMessage(msg);

         bytesSent += bytesRead == -1 ? 0 : bytesRead;
         if (listener != null) 
            listener.markProgress(bytesSent);

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
         waitForRCC(abortMessage.getMessageId());
      }
   }

   /**
    * Wait for message with specific code and id.
    * 
    * @param code
    *           Message code
    * @param id
    *           Message id
    * @param timeout
    *           Wait timeout in milliseconds
    * @return Message object
    * @throws NXCException
    *            if message was not arrived within timeout interval
    */
   public NXCPMessage waitForMessage(final int code, final long id, final int timeout) throws NXCException
   {
      final NXCPMessage msg = msgWaitQueue.waitForMessage(code, id, timeout);
      if (msg == null) 
         throw new NXCException(RCC.TIMEOUT);
      return msg;
   }

   /**
    * Wait for message with specific code and id.
    * 
    * @param code
    *           Message code
    * @param id
    *           Message id
    * @return Message object
    * @throws NXCException
    *            if message was not arrived within timeout interval
    */
   public NXCPMessage waitForMessage(final int code, final long id) throws NXCException
   {
      final NXCPMessage msg = msgWaitQueue.waitForMessage(code, id);
      if (msg == null) 
         throw new NXCException(RCC.TIMEOUT);
      return msg;
   }

   /**
    * Wait for CMD_REQUEST_COMPLETED message with given id using default timeout
    * 
    * @param id
    *           Message id
    * @return received message
    * @throws NXCException
    *            if message was not arrived within timeout interval or contains RCC other than RCC.SUCCESS
    */
   public NXCPMessage waitForRCC(final long id) throws NXCException
   {
      return waitForRCC(id, msgWaitQueue.getDefaultTimeout());
   }

   /**
    * Wait for CMD_REQUEST_COMPLETED message with given id
    * 
    * @param id
    *           Message id
    * @param timeout Timeout in milliseconds
    * @return received message
    * @throws NXCException
    *            if message was not arrived within timeout interval or contains RCC other than RCC.SUCCESS
    */
   public NXCPMessage waitForRCC(final long id, final int timeout) throws NXCException
   {
      final NXCPMessage msg = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, id, timeout);
      final int rcc = msg.getFieldAsInt32(NXCPCodes.VID_RCC);
      if (rcc != RCC.SUCCESS)
      {
         if ((rcc == RCC.COMPONENT_LOCKED) && (msg.findField(NXCPCodes.VID_LOCKED_BY) != null))
         {
            throw new NXCException(rcc, msg.getFieldAsString(NXCPCodes.VID_LOCKED_BY));
         }
         else if (msg.findField(NXCPCodes.VID_ERROR_TEXT) != null)
         {
            throw new NXCException(rcc, msg.getFieldAsString(NXCPCodes.VID_ERROR_TEXT));
         }
         else if (msg.findField(NXCPCodes.VID_VALUE) != null)
         {
            throw new NXCException(rcc, msg.getFieldAsString(NXCPCodes.VID_VALUE));
         }
         else
         {
            throw new NXCException(rcc);
         }
      }
      return msg;
   }

   /**
    * Create new NXCP message with unique id
    * 
    * @param code
    *           Message code
    * @return New message object
    */
   public final NXCPMessage newMessage(int code)
   {
      return new NXCPMessage(code, requestId.getAndIncrement());
   }

   /**
    * Wait for specific file to arrive
    *
    * @param id      Message ID
    * @param timeout Wait timeout in milliseconds
    * @return Received file or null in case of failure
    */
   public File waitForFile(final long id, final int timeout)
   {
      int timeRemaining = timeout;
      File file = null;

      while(timeRemaining > 0)
      {
         synchronized(receivedFiles)
         {
            NXCReceivedFile rf = receivedFiles.get(id);
            if (rf != null)
            {
               if (rf.getStatus() != NXCReceivedFile.OPEN)
               {
                  if (rf.getStatus() == NXCReceivedFile.RECEIVED) file = rf.getFile();
                  break;
               }
            }

            long startTime = System.currentTimeMillis();
            try
            {
               receivedFiles.wait(timeRemaining);
            }
            catch(InterruptedException e)
            {
            }
            timeRemaining -= System.currentTimeMillis() - startTime;
         }
      }
      return file;
   }
   
   /**
    * Wait for specific file tail to arrive
    *
    * @param fileName      Waiting file name
    * @param timeout       Wait timeout in milliseconds
    * @return Received tail string or null in case of failure
    */
   public String waitForFileTail(String fileName, final int timeout)
   {
      int timeRemaining = timeout;
      String tail = null;

      while(timeRemaining > 0)
      {
         synchronized(recievedUpdates)
         {
            tail = recievedUpdates.get(fileName);
            if (tail != null)
            {
               recievedUpdates.remove(fileName);
               break;
            }

            long startTime = System.currentTimeMillis();
            try
            {
               recievedUpdates.wait(timeRemaining);
            }
            catch(InterruptedException e)
            {
            }
            timeRemaining -= System.currentTimeMillis() - startTime;
         }
      }
      return tail;
   }   

   /**
    * Execute simple commands (without arguments and only returning RCC)
    *
    * @param command Command code
    * @throws IOException
    * @throws NXCException
    */
   protected void executeSimpleCommand(int command) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(command);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Receive table from server.
    *
    * @param requestId request ID
    * @param msgCode   Message code
    * @return Received table
    * @throws NXCException if operation was timed out
    */
   public Table receiveTable(long requestId, int msgCode) throws NXCException
   {
      NXCPMessage msg = waitForMessage(msgCode, requestId);
      Table table = new Table(msg);
      while(!msg.isEndOfSequence())
      {
         msg = waitForMessage(msgCode, requestId);
         table.addDataFromMessage(msg);
      }
      return table;
   }

   /**
    * Connect to NetMS server. Establish connection with the server and set up encryption if required.
    * Only base protocol version check will be performed. Login must be performed before using session
    * after successful connect.
    * 
    * @throws IOException
    * @throws UnknownHostException
    * @throws NXCException
    * @throws IllegalStateException
    */
   public void connect() throws IOException, UnknownHostException, NXCException, IllegalStateException
   {
      connect(null);
   }
   
   /**
    * Connect to NetMS server. Establish connection with the server and set up encryption if required.
    * Versions of protocol components given in *componentVersions* will be validated. Login must be 
    * performed before using session after successful connect.
    * 
    * @param componentVersions
    * @throws IOException
    * @throws UnknownHostException
    * @throws NXCException
    * @throws IllegalStateException
    */
   public void connect(int[] componentVersions) throws IOException, UnknownHostException, NXCException, IllegalStateException
   {
      if (isConnected)
         throw new IllegalStateException("Session already connected");

      if (isDisconnected)
         throw new IllegalStateException("Session already disconnected and cannot be reused");
      
      encryptionContext = null;
      Logger.info("NXCSession.connect", "Connecting to " + connAddress + ":" + connPort);
      try
      {
         connSocket = new Socket(connAddress, connPort);
         msgWaitQueue = new NXCPMsgWaitQueue(commandTimeout);
         recvThread = new ReceiverThread();
         housekeeperThread = new HousekeeperThread();
         new NotificationProcessor();

         // get server information
         Logger.debug("NXCSession.connect", "connection established, retrieving server info");
         NXCPMessage request = newMessage(NXCPCodes.CMD_GET_SERVER_INFO);
         sendMessage(request);
         NXCPMessage response = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());

         protocolVersion = new ProtocolVersion(response);
         if (!ignoreProtocolVersion)
         {
            if (!protocolVersion.isCorrectVersion(ProtocolVersion.INDEX_BASE) ||
                ((componentVersions != null) && !validateProtocolVersions(componentVersions)))
            {
               Logger.warning("NXCSession.connect", "connection failed (" + protocolVersion.toString() + ")");
               throw new NXCException(RCC.BAD_PROTOCOL);
            }
         }
         else
         {
            Logger.debug("NXCSession.connect", "protocol version ignored");
         }

         serverVersion = response.getFieldAsString(NXCPCodes.VID_SERVER_VERSION);
         serverId = response.getFieldAsInt64(NXCPCodes.VID_SERVER_ID);
         serverTimeZone = response.getFieldAsString(NXCPCodes.VID_TIMEZONE);
         serverTime = response.getFieldAsInt64(NXCPCodes.VID_TIMESTAMP) * 1000;
         serverTimeRecvTime = System.currentTimeMillis();
         serverChallenge = response.getFieldAsBinary(NXCPCodes.VID_CHALLENGE);

         tileServerURL = response.getFieldAsString(NXCPCodes.VID_TILE_SERVER_URL);
         if (tileServerURL != null)
         {
            if (!tileServerURL.endsWith("/")) tileServerURL = tileServerURL.concat("/");
         }
         else
         {
            tileServerURL = "http://tile.openstreetmap.org/";
         }

         dateFormat = response.getFieldAsString(NXCPCodes.VID_DATE_FORMAT);
         if ((dateFormat == null) || (dateFormat.length() == 0)) 
            dateFormat = "dd.MM.yyyy";

         timeFormat = response.getFieldAsString(NXCPCodes.VID_TIME_FORMAT);
         if ((timeFormat == null) || (timeFormat.length() == 0)) 
            timeFormat = "HH:mm:ss";

         shortTimeFormat = response.getFieldAsString(NXCPCodes.VID_SHORT_TIME_FORMAT);
         if ((shortTimeFormat == null) || (shortTimeFormat.length() == 0)) 
            shortTimeFormat = "HH:mm";

         // Setup encryption if required
         if (connUseEncryption)
         {
            request = newMessage(NXCPCodes.CMD_REQUEST_ENCRYPTION);
            request.setFieldInt16(NXCPCodes.VID_USE_X509_KEY_FORMAT, 1);
            sendMessage(request);
            waitForRCC(request.getMessageId());
         }

         Logger.debug("NXCSession.connect", "Connected to server version " + serverVersion);
         isConnected = true;
      }
      finally
      {
         if (!isConnected) 
            disconnect();
      }
   }
   
   /**
    * Login to server using login name and password.
    * 
    * @param login login name
    * @param password password
    * @throws NXCException
    * @throws IOException
    * @throws IllegalStateException
    */
   public void login(String login, String password) throws NXCException, IOException, IllegalStateException
   {
      login(AuthenticationType.PASSWORD, login, password, null, null);
   }
   
   /**
    * Login to server using certificate.
    * 
    * @param authType authentication type
    * @param login login name
    * @param certificate user's certificate
    * @param signature user's digital signature
    * @throws NXCException
    * @throws IOException
    * @throws IllegalStateException
    */
   public void login(String login, Certificate certificate, Signature signature) throws NXCException, IOException, IllegalStateException
   {
      login(AuthenticationType.CERTIFICATE, login, null, certificate, signature);
   }
   
   /**
    * Login to server.
    * 
    * @param authType authentication type
    * @param login login name
    * @param password password
    * @param certificate user's certificate
    * @param signature user's digital signature
    * @throws NXCException
    * @throws IOException
    * @throws IllegalStateException
    */
   public void login(AuthenticationType authType, String login, String password, Certificate certificate, Signature signature) throws NXCException, IOException, IllegalStateException
   {
      if (!isConnected)
         throw new IllegalStateException("Session not connected");

      if (isDisconnected)
         throw new IllegalStateException("Session already disconnected and cannot be reused");
      
      authenticationMethod = authType;
      userName = login;
      
      final NXCPMessage request = newMessage(NXCPCodes.CMD_LOGIN);
      request.setFieldInt16(NXCPCodes.VID_AUTH_TYPE, authType.getValue());
      request.setField(NXCPCodes.VID_LOGIN_NAME, login);

      if ((authType == AuthenticationType.PASSWORD) || (authType == AuthenticationType.SSO_TICKET))
      {
         request.setField(NXCPCodes.VID_PASSWORD, password);
      }
      else if (authType == AuthenticationType.CERTIFICATE)
      {
         if ((serverChallenge == null) || (signature == null) || (certificate == null))
         {
            throw new NXCException(RCC.ENCRYPTION_ERROR);
         }
         byte[] signedChallenge = signChallenge(signature, serverChallenge);
         request.setField(NXCPCodes.VID_SIGNATURE, signedChallenge);
         try
         {
            request.setField(NXCPCodes.VID_CERTIFICATE, certificate.getEncoded());
         }
         catch(CertificateEncodingException e)
         {
            throw new NXCException(RCC.ENCRYPTION_ERROR);
         }
      }

      request.setField(NXCPCodes.VID_LIBNXCL_VERSION, NXCommon.VERSION);
      request.setField(NXCPCodes.VID_CLIENT_INFO, connClientInfo);
      request.setField(NXCPCodes.VID_OS_INFO, System.getProperty("os.name") + " " + System.getProperty("os.version"));
      request.setFieldInt16(NXCPCodes.VID_CLIENT_TYPE, clientType);
      if (clientAddress != null) 
         request.setField(NXCPCodes.VID_CLIENT_ADDRESS, clientAddress);
      if (clientLanguage != null)
         request.setField(NXCPCodes.VID_LANGUAGE, clientLanguage);
      sendMessage(request);
      
      final NXCPMessage response = waitForMessage(NXCPCodes.CMD_LOGIN_RESP, request.getMessageId());
      int rcc = response.getFieldAsInt32(NXCPCodes.VID_RCC);
      Logger.debug("NXCSession.connect", "CMD_LOGIN_RESP received, RCC=" + rcc);
      if (rcc != RCC.SUCCESS)
      {
         Logger.warning("NXCSession.connect", "Login failed, RCC=" + rcc);
         throw new NXCException(rcc);
      }
      userId = response.getFieldAsInt32(NXCPCodes.VID_USER_ID);
      sessionId = response.getFieldAsInt32(NXCPCodes.VID_SESSION_ID);
      userSystemRights = response.getFieldAsInt64(NXCPCodes.VID_USER_SYS_RIGHTS);
      passwordExpired = response.getFieldAsBoolean(NXCPCodes.VID_CHANGE_PASSWD_FLAG);
      zoningEnabled = response.getFieldAsBoolean(NXCPCodes.VID_ZONING_ENABLED);
      helpdeskLinkActive = response.getFieldAsBoolean(NXCPCodes.VID_HELPDESK_LINK_ACTIVE);

      defaultDciPollingInterval = response.getFieldAsInt32(NXCPCodes.VID_POLLING_INTERVAL);
      if (defaultDciPollingInterval == 0) 
         defaultDciPollingInterval = 60;

      defaultDciRetentionTime = response.getFieldAsInt32(NXCPCodes.VID_RETENTION_TIME);
      if (defaultDciRetentionTime == 0) 
         defaultDciRetentionTime = 30;
      
      minViewRefreshInterval = response.getFieldAsInt32(NXCPCodes.VID_VIEW_REFRESH_INTERVAL);
      if (minViewRefreshInterval <= 0)
         minViewRefreshInterval = 200;

      strictAlarmStatusFlow = response.getFieldAsBoolean(NXCPCodes.VID_ALARM_STATUS_FLOW_STATE);
      timedAlarmAckEnabled = response.getFieldAsBoolean(NXCPCodes.VID_TIMED_ALARM_ACK_ENABLED);

      alarmListDisplayLimit = response.getFieldAsInt32(NXCPCodes.VID_ALARM_LIST_DISP_LIMIT);
      Logger.info("NXCSession.connect", "alarmListDisplayLimit = " + alarmListDisplayLimit);

      Logger.info("NXCSession.connect", "succesfully logged in, userId=" + userId);
   }

   /**
    * Disconnect from server.
    */
   public void disconnect()
   {
      if (connSocket != null)
      {
         try
         {
            connSocket.shutdownInput();
            connSocket.shutdownOutput();
         }
         catch(IOException e)
         {
         }

         try
         {
            connSocket.close();
         }
         catch(IOException e)
         {
         }
      }
      
      // cause notification processing thread to stop
      notificationQueue.clear();
      notificationQueue.offer(new SessionNotification(SessionNotification.STOP_PROCESSING_THREAD));

      if (recvThread != null)
      {
         while(recvThread.isAlive())
         {
            try
            {
               recvThread.join();
            }
            catch(InterruptedException e)
            {
            }
         }
         recvThread = null;
      }

      if (housekeeperThread != null)
      {
         housekeeperThread.setStopFlag(true);
         while(housekeeperThread.isAlive())
         {
            try
            {
               housekeeperThread.join();
            }
            catch(InterruptedException e)
            {
            }
         }
         housekeeperThread = null;
      }

      connSocket = null;

      if (msgWaitQueue != null)
      {
         msgWaitQueue.shutdown();
         msgWaitQueue = null;
      }

      isConnected = false;
      isDisconnected = true;
   }

   /**
    * Get connection state
    * @return connection state
    */
   public boolean isConnected()
   {
      return isConnected;
   }

   /**
    * Get encryption state for current session.
    * 
    * @return true if session is encrypted
    */
   public boolean isEncrypted()
   {
      return connUseEncryption;
   }

   /**
    * @return
    */
   public boolean isIgnoreProtocolVersion()
   {
      return ignoreProtocolVersion;
   }

   /**
    * If set to true, protocol version is not checked at connect.
    *
    * @param ignoreProtocolVersion
    */
   public void setIgnoreProtocolVersion(boolean ignoreProtocolVersion)
   {
      this.ignoreProtocolVersion = ignoreProtocolVersion;
   }
   
   /**
    * Validate protocol versions
    * 
    * @param versions
    * @return
    */
   public boolean validateProtocolVersions(int[] versions)
   {
      if (protocolVersion == null)
         return false;
      for(int index : versions)
         if (!protocolVersion.isCorrectVersion(index))
            return false;
      return true;
   }

   /**
    * Get default receiver buffer size.
    * 
    * @return Default receiver buffer size in bytes.
    */
   public int getDefaultRecvBufferSize()
   {
      return defaultRecvBufferSize;
   }

   /**
    * Get max receiver buffer size.
    * 
    * @return Max receiver buffer size in bytes.
    */
   public int getMaxRecvBufferSize()
   {
      return maxRecvBufferSize;
   }

   /**
    * Set receiver buffer size. This method should be called before connect(). It will not have any effect after
    * connect().
    * 
    * @param defaultBufferSize default size of receiver buffer in bytes.
    * @param maxBufferSize max size of receiver buffer in bytes.
    */
   public void setRecvBufferSize(int defaultBufferSize, int maxBufferSize)
   {
      this.defaultRecvBufferSize = defaultBufferSize;
      this.maxRecvBufferSize = maxBufferSize;
   }

   /**
    * Get server address
    * 
    * @return Server address
    */
   public String getServerAddress()
   {
      return connAddress;
   }

   /**
    * Get NetXMS server version.
    * 
    * @return Server version
    */
   public String getServerVersion()
   {
      return serverVersion;
   }

   /**
    * Get NetXMS server UID.
    * 
    * @return Server UID
    */
   public long getServerId()
   {
      return serverId;
   }

   /**
    * Get server time zone.
    * 
    * @return server's time zone string
    */
   public String getServerTimeZone()
   {
      return serverTimeZone;
   }

   /**
    * Get server time
    * 
    * @return the serverTime
    */
   public long getServerTime()
   {
      long offset = System.currentTimeMillis() - serverTimeRecvTime;
      return serverTime + offset;
   }
   
   /**
    * @return the serverChallenge
    */
   public byte[] getServerChallenge()
   {
      return serverChallenge;
   }

   /**
    * @return the tileServerURL
    */
   public String getTileServerURL()
   {
      return tileServerURL;
   }

   /**
    * @return the zoningEnabled
    */
   public boolean isZoningEnabled()
   {
      return zoningEnabled;
   }

   /**
    * Get status of helpdesk integration module on server.
    * 
    * @return true if helpdesk integration module loaded on server
    */
   public boolean isHelpdeskLinkActive()
   {
      return helpdeskLinkActive;
   }

   /**
    * Get client information string
    * 
    * @return
    */
   public String getClientInfo()
   {
      return connClientInfo;
   }

   /**
    * Set client information string
    * 
    * @param connClientInfo
    */
   public void setClientInfo(final String connClientInfo)
   {
      this.connClientInfo = connClientInfo;
   }

   /**
    * Set command execution timeout.
    * 
    * @param commandTimeout
    *           New command timeout
    */
   public void setCommandTimeout(final int commandTimeout)
   {
      this.commandTimeout = commandTimeout;
   }

   /**
    * Get identifier of logged in user.
    * 
    * @return Identifier of logged in user
    */
   public int getUserId()
   {
      return userId;
   }

   /**
    * @return the userName
    */
   public String getUserName()
   {
      return userName;
   }

   /**
    * @return the authenticationMethod
    */
   public AuthenticationType getAuthenticationMethod()
   {
      return authenticationMethod;
   }

   /**
    * Get system-wide rights of currently logged in user.
    * 
    * @return System-wide rights of currently logged in user
    */
   public long getUserSystemRights()
   {
      return userSystemRights;
   }

   /**
    * Check if password is expired for currently logged in user.
    * 
    * @return true if password is expired
    */
   public boolean isPasswordExpired()
   {
      return passwordExpired;
   }
   
   /**
    * Get maximum number of records allowed to be displayed in alarm list
    */
   public int getAlarmListDisplayLimit()
   {
		return alarmListDisplayLimit;
   }

   /**
    * Synchronizes NetXMS objects between server and client. After successful
    * sync, subscribe client to object change notifications.
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public synchronized void syncObjects() throws IOException, NXCException
   {
      syncObjects.acquireUninterruptibly();
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_OBJECTS);
      msg.setFieldInt16(NXCPCodes.VID_SYNC_COMMENTS, 1);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      waitForSync(syncObjects, commandTimeout * 10);
      objectsSynchronized = true;
      sendNotification(new SessionNotification(SessionNotification.OBJECT_SYNC_COMPLETED));
      subscribe(CHANNEL_OBJECTS);
   }

   /**
    * Synchronizes selected object set with the server.
    *
    * @param objects      identifiers of objects need to be synchronized
    * @param syncComments if true, comments for objects will be synchronized as well
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncObjectSet(long[] objects, boolean syncComments) throws IOException, NXCException
   {
      syncObjectSet(objects, syncComments, 0);
   }

   /**
    * Synchronizes selected object set with the server. The following options are accepted:
    * OBJECT_SYNC_NOTIFY - send object update notification for each received object
    * OBJECT_SYNC_WAIT   - wait until all requested objects received
    *
    * @param objects      identifiers of objects need to be synchronized
    * @param syncComments if true, comments for objects will be synchronized as well
    * @param options      sync options (see above)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncObjectSet(long[] objects, boolean syncComments, int options) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SELECTED_OBJECTS);
      msg.setFieldInt16(NXCPCodes.VID_SYNC_COMMENTS, syncComments ? 1 : 0);
      msg.setFieldInt16(NXCPCodes.VID_FLAGS, options);
      msg.setFieldInt32(NXCPCodes.VID_NUM_OBJECTS, objects.length);
      msg.setField(NXCPCodes.VID_OBJECT_LIST, objects);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      if ((options & OBJECT_SYNC_WAIT) != 0) waitForRCC(msg.getMessageId());
   }

   /**
    * Synchronize only those objects from given set which are not synchronized yet.
    *
    * @param objects      identifiers of objects need to be synchronized
    * @param syncComments if true, comments for objects will be synchronized as well
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncMissingObjects(long[] objects, boolean syncComments) throws IOException, NXCException
   {
      syncMissingObjects(objects, syncComments, 0);
   }

   /**
    * Synchronize only those objects from given set which are not synchronized yet.
    * Accepts all options which are valid for syncObjectSet.
    *
    * @param objects      identifiers of objects need to be synchronized
    * @param syncComments if true, comments for objects will be synchronized as well
    * @param options      sync options (see comments for syncObjectSet)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncMissingObjects(long[] objects, boolean syncComments, int options) throws IOException, NXCException
   {
      final long[] syncList = CompatTools.arrayCopy(objects, objects.length);
      int count = syncList.length;
      synchronized(objectList)
      {
         for(int i = 0; i < syncList.length; i++)
         {
            if (objectList.containsKey(syncList[i]))
            {
               syncList[i] = 0;
               count--;
            }
         }
      }

      if (count > 0)
      {
         syncObjectSet(syncList, syncComments, options);
      }
   }

   /**
    * Find NetXMS object by it's identifier.
    *
    * @param id Object identifier
    * @return Object with given ID or null if object cannot be found
    */
   public AbstractObject findObjectById(final long id)
   {
      AbstractObject obj;

      synchronized(objectList)
      {
         obj = objectList.get(id);
      }
      return obj;
   }

   /**
    * Find NetXMS object by it's identifier with additional class checking.
    *
    * @param id            object identifier
    * @param requiredClass required object class
    * @return Object with given ID or null if object cannot be found or is not an instance of required class
    */
   @SuppressWarnings("unchecked")
   public <T extends AbstractObject> T findObjectById(final long id, final Class<T> requiredClass)
   {
      AbstractObject object = findObjectById(id);
      return requiredClass.isInstance(object) ? (T)object : null;
   }

   /**
    * Find multiple NetXMS objects by identifiers
    *
    * @param idList        array of object identifiers
    * @param returnUnknown if true, this method will return UnknownObject placeholders for unknown object identifiers
    * @return list of found objects
    */
   public List<AbstractObject> findMultipleObjects(final long[] idList, boolean returnUnknown)
   {
      return findMultipleObjects(idList, null, returnUnknown);
   }

   /**
    * Find multiple NetXMS objects by identifiers
    *
    * @param idList        array of object identifiers
    * @param classFilter   class filter for objects, or null to disable filtering
    * @param returnUnknown if true, this method will return UnknownObject placeholders for unknown object identifiers
    * @return list of found objects
    */
   public List<AbstractObject> findMultipleObjects(final long[] idList, Class<? extends AbstractObject> classFilter, boolean returnUnknown)
   {
      List<AbstractObject> result = new ArrayList<AbstractObject>(idList.length);

      synchronized(objectList)
      {
         for(int i = 0; i < idList.length; i++)
         {
            final AbstractObject object = objectList.get(idList[i]);
            if ((object != null) && ((classFilter == null) || classFilter.isInstance(object)))
            {
               result.add(object);
            }
            else if (returnUnknown)
            {
               result.add(new UnknownObject(idList[i], this));
            }
         }
      }

      return result;
   }

   /**
    * Find multiple NetXMS objects by identifiers
    *
    * @param idList        array of object identifiers
    * @param returnUnknown if true, this method will return UnknownObject placeholders for unknown object identifiers
    * @return array of found objects
    */
   public List<AbstractObject> findMultipleObjects(final Long[] idList, boolean returnUnknown)
   {
      return findMultipleObjects(idList, null, returnUnknown);
   }

   /**
    * Find multiple NetXMS objects by identifiers
    *
    * @param idList        array of object identifiers
    * @param classFilter   class filter for objects, or null to disable filtering
    * @param returnUnknown if true, this method will return UnknownObject placeholders for unknown object identifiers
    * @return array of found objects
    */
   public List<AbstractObject> findMultipleObjects(final Long[] idList, Class<? extends AbstractObject> classFilter, boolean returnUnknown)
   {
      List<AbstractObject> result = new ArrayList<AbstractObject>(idList.length);

      synchronized(objectList)
      {
         for(int i = 0; i < idList.length; i++)
         {
            final AbstractObject object = objectList.get(idList[i]);
            if ((object != null) && ((classFilter == null) || classFilter.isInstance(object)))
            {
               result.add(object);
            }
            else if (returnUnknown)
            {
               result.add(new UnknownObject(idList[i], this));
            }
         }
      }

      return result;
   }

   /**
    * Find zone object by zone ID.
    *
    * @param zoneId zone ID to find
    * @return zone object or null
    */
   public Zone findZone(long zoneId)
   {
      Zone result = null;
      synchronized(objectList)
      {
         AbstractObject entireNetwork = objectList.get(1L);
         Collection<AbstractObject> objects = (entireNetwork != null) ? 
               entireNetwork.getAllChilds(AbstractObject.OBJECT_ZONE) : objectList.values();
         for(AbstractObject object : objects)
         {
            if ((object instanceof Zone) && ((Zone)object).getZoneId() == zoneId)
            {
               result = (Zone)object;
               break;
            }
         }
      }
      return result;
   }

   
   /**
    * Find object by name. If multiple objects with same name exist,
    * it is not determined what object will be returned. Name comparison
    * is case-insensitive.
    *
    * @param name object name to find
    * @return object with matching name or null
    */
   public AbstractObject findObjectByName(final String name)
   {
      AbstractObject result = null;
      synchronized(objectList)
      {
         for(AbstractObject object : objectList.values())
         {
            if (object.getObjectName().equalsIgnoreCase(name))
            {
               result = object;
               break;
            }
         }
      }
      return result;
   }

   /**
    * Find object by name using regular expression. If multiple objects with same name exist,
    * it is not determined what object will be returned. Name comparison is case-insensitive.
    *
    * @param pattern regular expression for matching object name
    * @return object with matching name or null
    */
   public AbstractObject findObjectByNamePattern(final String pattern)
   {
      AbstractObject result = null;
      Matcher matcher = Pattern.compile(pattern).matcher("");
      synchronized(objectList)
      {
         for(AbstractObject object : objectList.values())
         {
            matcher.reset(object.getObjectName());
            if (matcher.matches())
            {
               result = object;
               break;
            }
         }
      }
      return result;
   }

   /**
    * Get list of top-level objects matching given class filter. Class filter
    * may be null to ignore object class.
    *
    * @return List of all top matching level objects (either without parents or with
    *         inaccessible parents)
    */
   public AbstractObject[] getTopLevelObjects(Set<Integer> classFilter)
   {
      HashSet<AbstractObject> list = new HashSet<AbstractObject>();
      synchronized(objectList)
      {
         for(AbstractObject object : objectList.values())
         {
            if ((classFilter != null) && !classFilter.contains(object.getObjectClass())) continue;

            if (!object.hasParents())
            {
               list.add(object);
            }
            else
            {
               boolean hasParents = false;
               Iterator<Long> it = object.getParents();
               while(it.hasNext())
               {
                  Long parent = it.next();
                  if (classFilter != null)
                  {
                     AbstractObject p = objectList.get(parent);
                     if ((p != null) && classFilter.contains(p.getObjectClass()))
                     {
                        hasParents = true;
                        break;
                     }
                  }
                  else
                  {
                     if (objectList.containsKey(parent))
                     {
                        hasParents = true;
                        break;
                     }
                  }
               }
               if (!hasParents) list.add(object);
            }
         }
      }
      return list.toArray(new AbstractObject[list.size()]);
   }

   /**
    * Get list of top-level objects.
    *
    * @return List of all top level objects (either without parents or with
    *         inaccessible parents)
    */
   public AbstractObject[] getTopLevelObjects()
   {
      return getTopLevelObjects(null);
   }

   /**
    * Get list of all objects
    *
    * @return List of all objects
    */
   public AbstractObject[] getAllObjects()
   {
      AbstractObject[] list;

      synchronized(objectList)
      {
         list = objectList.values().toArray(new AbstractObject[objectList.size()]);
      }
      return list;
   }

   /**
    * Get object name by ID.
    *
    * @param objectId object ID
    * @return object name if object is known, or string in form [<object_id>] for unknown objects
    */
   public String getObjectName(long objectId)
   {
      AbstractObject object = findObjectById(objectId);
      return (object != null) ? object.getObjectName() : ("[" + Long.toString(objectId) + "]");
   }

   /**
    * Get list of active alarms. For accessing terminated alarms log view API should be used.
    *
    * @return Hash map containing alarms
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public HashMap<Long, Alarm> getAlarms() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ALL_ALARMS);
      final long rqId = msg.getMessageId();
      sendMessage(msg);

      final HashMap<Long, Alarm> alarmList = new HashMap<Long, Alarm>(0);
      while(true)
      {
         msg = waitForMessage(NXCPCodes.CMD_ALARM_DATA, rqId);
         long alarmId = msg.getFieldAsInt32(NXCPCodes.VID_ALARM_ID);
         if (alarmId == 0) 
            break; // ALARM_ID == 0 indicates end of list
         alarmList.put(alarmId, new Alarm(msg));
      }

      return alarmList;
   }

   /**
    * Get information about single active alarm. Terminated alarms cannot be accessed with this call.
    *
    * @param alarmId alarm ID
    * @return alarm object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Alarm getAlarm(long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ALARM);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new Alarm(response);
   }

   /**
    * Get information about events related to single active alarm. Information for terminated alarms cannot be accessed with this call.
    * User must have "view alarms" permission on alarm's source node and "view event log" system-wide access.
    *
    * @param alarmId alarm ID
    * @return list of related events
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<EventInfo> getAlarmEvents(long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ALARM_EVENTS);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<EventInfo> list = new ArrayList<EventInfo>(count);
      long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         EventInfo parent = null;
         long rootId = response.getFieldAsInt64(varId + 1);
         if (rootId != 0)
         {
            for(EventInfo e : list)
            {
               if (e.getId() == rootId)
               {
                  parent = e;
                  break;
               }
            }
         }
         list.add(new EventInfo(response, varId, parent));
         varId += 10;
      }
      return list;
   }

   /**
    * Acknowledge alarm.
    *
    * @param alarmId Identifier of alarm to be acknowledged.
    * @param sticky  if set to true, acknowledged state will be made "sticky" (duplicate alarms with same key will not revert it back to outstanding)
    * @param time timeout for sticky acknowledge in seconds (0 for infinite)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void acknowledgeAlarm(final long alarmId, boolean sticky, int time) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_ACK_ALARM);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
      msg.setFieldInt16(NXCPCodes.VID_STICKY_FLAG, sticky ? 1 : 0);
      msg.setFieldInt32(NXCPCodes.VID_TIMESTAMP, time);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Acknowledge alarm.
    *
    * @param alarmId Identifier of alarm to be acknowledged.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void acknowledgeAlarm(final long alarmId) throws IOException, NXCException
   {
      acknowledgeAlarm(alarmId, false, 0);
   }

   /**
    * Acknowledge alarm by helpdesk reference.
    *
    * @param helpdeskReference Helpdesk issue reference (e.g. JIRA issue key)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void acknowledgeAlarm(String helpdeskReference) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_ACK_ALARM);
      msg.setField(NXCPCodes.VID_HELPDESK_REF, helpdeskReference);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Resolve alarm.
    *
    * @param alarmId Identifier of alarm to be resolved.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void resolveAlarm(final long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_RESOLVE_ALARM);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Resolve alarm by helpdesk reference.
    *
    * @param helpdeskReference  Identifier of alarm to be resolved.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void resolveAlarm(final String helpdeskReference) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_RESOLVE_ALARM);
      msg.setField(NXCPCodes.VID_HELPDESK_REF, helpdeskReference);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Terminate alarm.
    *
    * @param alarmId Identifier of alarm to be terminated.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void terminateAlarm(final long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_TERMINATE_ALARM);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Terminate alarm by helpdesk reference.
    *
    * @param helpdeskReference  Identifier of alarm to be resolved.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void terminateAlarm(final String helpdeskReference) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_TERMINATE_ALARM);
      msg.setField(NXCPCodes.VID_HELPDESK_REF, helpdeskReference);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete alarm.
    *
    * @param alarmId Identifier of alarm to be deleted.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAlarm(final long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_ALARM);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Open issue in helpdesk system from given alarm
    *
    * @param alarmId alarm identifier
    * @return helpdesk issue identifier
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String openHelpdeskIssue(long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_OPEN_HELPDESK_ISSUE);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int)alarmId);
      sendMessage(msg);
      return waitForRCC(msg.getMessageId()).getFieldAsString(NXCPCodes.VID_HELPDESK_REF);
   }
   
   /**
    * Get URL for helpdesk issue associated with given alarm
    * 
    * @param alarmId
    * @return
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String getHelpdeskIssueUrl(long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_HELPDESK_URL);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int)alarmId);
      sendMessage(msg);
      return waitForRCC(msg.getMessageId()).getFieldAsString(NXCPCodes.VID_URL);
   }
   
   /**
    * Unlink helpdesk issue from alarm. User must have OBJECT_ACCESS_UPDATE_ALARMS access right
    * on alarm's source object and SYSTEM_ACCESS_UNLINK_ISSUES system wide access right.
    * 
    * @param helpdeskReference
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unlinkHelpdeskIssue(String helpdeskReference) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UNLINK_HELPDESK_ISSUE);
      msg.setField(NXCPCodes.VID_HELPDESK_REF, helpdeskReference);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Unlink helpdesk issue from alarm. User must have OBJECT_ACCESS_UPDATE_ALARMS access right
    * on alarm's source object and SYSTEM_ACCESS_UNLINK_ISSUES system wide access right.
    * 
    * @param helpdeskReference
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unlinkHelpdeskIssue(long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UNLINK_HELPDESK_ISSUE);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int)alarmId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of comments for given alarm.
    *
    * @param alarmId alarm ID
    * @return list of alarm comments
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AlarmComment> getAlarmComments(long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ALARM_COMMENTS);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      final List<AlarmComment> comments = new ArrayList<AlarmComment>(count);
      long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         comments.add(new AlarmComment(response, varId));
         varId += 10;
      }
      return comments;
   }

   /**
    * Delete alarm comment. 
    *
    * @param alarmId alarm ID
    * @param commentId  comment ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAlarmComment(long alarmId, long commentId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_ALARM_COMMENT);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
      msg.setFieldInt32(NXCPCodes.VID_COMMENT_ID, (int) commentId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Create alarm comment.
    * 
    * @param alarmId
    * @param text
    * @throws IOException
    * @throws NXCException
    */
   public void createAlarmComment(long alarmId, String text) throws IOException, NXCException 
   {
   	updateAlarmComment(alarmId, 0, text);
   }

   /**
    * Create alarm comment by helpdesk reference.
    * 
    * @param helpdeskReference
    * @param text
    * @throws IOException
    * @throws NXCException
    */
   public void createAlarmComment(final String helpdeskReference, String text) throws IOException, NXCException 
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_ALARM_COMMENT);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, 0);
      msg.setField(NXCPCodes.VID_HELPDESK_REF, helpdeskReference);
      msg.setField(NXCPCodes.VID_COMMENTS, text);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Update alarm comment.
    * If alarmId == 0 — new comment will be created.
    *
    * @param alarmId alarm ID
    * @param commentId  comment ID or 0 for creating new comment
    * @param text    message text
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateAlarmComment(long alarmId, long commentId, String text) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_ALARM_COMMENT);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
      msg.setFieldInt32(NXCPCodes.VID_COMMENT_ID, (int) commentId);
      msg.setField(NXCPCodes.VID_COMMENTS, text);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Changes state of alarm status flow. Strict or not - terminate state can be set only after 
    * resolve state or after any state. 
    * 
    * @param state state of alarm status flow - strict or not (1 or 0)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setAlarmFlowState(int state) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_ALARM_STATUS_FLOW);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_STATUS_FLOW_STATE, state);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get server configuration variables
    * 
    * @return
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<String, ServerVariable> getServerVariables() throws IOException, NXCException
   {
      NXCPMessage request = newMessage(NXCPCodes.CMD_GET_CONFIG_VARLIST);
      sendMessage(request);

      final NXCPMessage response = waitForRCC(request.getMessageId());

      long id;
      int i, count = response.getFieldAsInt32(NXCPCodes.VID_NUM_VARIABLES);
      final HashMap<String, ServerVariable> varList = new HashMap<String, ServerVariable>(count);
      for(i = 0, id = NXCPCodes.VID_VARLIST_BASE; i < count; i++, id += 3)
      {
         String name = response.getFieldAsString(id);
         varList.put(name,
            new ServerVariable(name, response.getFieldAsString(id + 1), response.getFieldAsBoolean(id + 2)));
      }

      return varList;
   }
   
   /**
    * Get server public configuration variable
    * 
    * @param name configuration variable name
    * @return value of requested configuration variable
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String getPublicServerVariable(String name) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PUBLIC_CONFIG_VAR);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsString(NXCPCodes.VID_VALUE);
   }

   /**
    * Get server public configuration variable as boolen value
    * 
    * @param name configuration variable name
    * @return value of requested configuration variable
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public boolean getPublicServerVariableAsBoolean(String name) throws IOException, NXCException
   {
      String value = getPublicServerVariable(name);
      if ((value.equalsIgnoreCase("true")) || (value.equalsIgnoreCase("yes")))
         return true;
      if ((value.equalsIgnoreCase("false")) || (value.equalsIgnoreCase("no")))
         return false;
      try
      {
         int n = Integer.parseInt(value);
         return n != 0;
      }
      catch(NumberFormatException e)
      {
         return false;
      }
   }
   
   /**
    * Set server configuration variable
    * 
    * @param name
    * @param value
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setServerVariable(final String name, final String value) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_CONFIG_VARIABLE);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_VALUE, value);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete server configuration variable
    * 
    * @param name
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteServerVariable(final String name) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_CONFIG_VARIABLE);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get server config CLOB
    *
    * @param name
    * @return
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String getServerConfigClob(final String name) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CONFIG_GET_CLOB);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsString(NXCPCodes.VID_VALUE);
   }

   /**
    * Set server config CLOB
    *
    * @param name
    * @param value
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setServerConfigClob(final String name, final String value) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_CONFIG_SET_CLOB);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_VALUE, value);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Subscribe to notification channel(s)
    *
    * @param channels Notification channels to subscribe to. Multiple channels can be
    *                 specified by combining them with OR operation.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void subscribe(int channels) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_SUBSCRIPTION);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, channels);
      msg.setFieldInt16(NXCPCodes.VID_OPERATION, 1);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Unsubscribe from notification channel(s)
    *
    * @param channels Notification channels to unsubscribe from. Multiple channels can
    *                 be specified by combining them with OR operation.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unsubscribe(int channels) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_SUBSCRIPTION);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, channels);
      msg.setFieldInt16(NXCPCodes.VID_OPERATION, 0);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Synchronize user database
    * 
    * @throws IOException
    *            if socket I/O error occurs
    * @throws NXCException
    *            if NetXMS server returns an error or operation was timed out
    */
   public void syncUserDatabase() throws IOException, NXCException
   {
      syncUserDB.acquireUninterruptibly();
      NXCPMessage msg = newMessage(NXCPCodes.CMD_LOAD_USER_DB);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      waitForSync(syncUserDB, commandTimeout * 10);
   }

   /**
    * Find user by ID
    * 
    * @return User object with given ID or null if such user does not exist
    */
   public AbstractUserObject findUserDBObjectById(final long id)
   {
      AbstractUserObject object;

      synchronized(userDB)
      {
         object = userDB.get(id);
      }
      return object;
   }

   /**
    * Get list of all user database objects
    * 
    * @return List of all user database objects
    */
   public AbstractUserObject[] getUserDatabaseObjects()
   {
      AbstractUserObject[] list;

      synchronized(userDB)
      {
         Collection<AbstractUserObject> values = userDB.values();
         list = values.toArray(new AbstractUserObject[values.size()]);
      }
      return list;
   }

   /**
    * Create user or group on server
    *
    * @param name Login name for new user
    * @return ID assigned to newly created user
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private long createUserDBObject(final String name, final boolean isGroup) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_USER);
      msg.setField(NXCPCodes.VID_USER_NAME, name);
      msg.setField(NXCPCodes.VID_IS_GROUP, isGroup);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_USER_ID);
   }

   /**
    * Create user on server
    * 
    * @param name
    *           Login name for new user
    * @return ID assigned to newly created user
    * @throws IOException
    *            if socket I/O error occurs
    * @throws NXCException
    *            if NetXMS server returns an error or operation was timed out
    */
   public long createUser(final String name) throws IOException, NXCException
   {
      return createUserDBObject(name, false);
   }

   /**
    * Create user group on server
    * 
    * @param name
    *           Name for new user group
    * @return ID assigned to newly created user group
    * @throws IOException
    *            if socket I/O error occurs
    * @throws NXCException
    *            if NetXMS server returns an error or operation was timed out
    */
   public long createUserGroup(final String name) throws IOException, NXCException
   {
      return createUserDBObject(name, true);
   }

   /**
    * Delete user or group on server
    * 
    * @param id
    *           User or group ID
    * @throws IOException
    *            if socket I/O error occurs
    * @throws NXCException
    *            if NetXMS server returns an error or operation was timed out
    */
   public void deleteUserDBObject(final long id) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_USER);
      msg.setFieldInt32(NXCPCodes.VID_USER_ID, (int) id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Set password for user
    * 
    * @param id User ID
    * @param newPassword New password
    * @param oldPassword Old password
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setUserPassword(final long id, final String newPassword, final String oldPassword) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_PASSWORD);
      msg.setFieldInt32(NXCPCodes.VID_USER_ID, (int) id);
      msg.setField(NXCPCodes.VID_PASSWORD, newPassword);
      if (oldPassword != null) 
         msg.setField(NXCPCodes.VID_OLD_PASSWORD, oldPassword);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Validate password for currently logged in user
    * 
    * @param password password to validate
    * @return true if password is valid
    * @throws IOException
    * @throws NXCException
    */
   public boolean validateUserPassword(String password) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_VALIDATE_PASSWORD);
      msg.setField(NXCPCodes.VID_PASSWORD, password);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsBoolean(NXCPCodes.VID_PASSWORD_IS_VALID);
   }

   /**
    * Modify user database object
    * 
    * @param object User data
    * @param fields bit mask indicating fields to modify
    * @throws IOException
    *            if socket I/O error occurs
    * @throws NXCException
    *            if NetXMS server returns an error or operation was timed out
    */
   public void modifyUserDBObject(final AbstractUserObject object, final int fields) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_USER);
      msg.setFieldInt32(NXCPCodes.VID_FIELDS, fields);
      object.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Modify user database object
    * 
    * @param object User data
    * @param fields bit mask indicating fields to modify
    * @throws IOException
    *            if socket I/O error occurs
    * @throws NXCException
    *            if NetXMS server returns an error or operation was timed out
    */
   public void detachUserFromLdap(final AbstractUserObject object) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DETACH_LDAP_USER);
      msg.setFieldInt32(NXCPCodes.VID_USER_ID, (int)object.getId());
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Modify user database object
    * 
    * @param object User data
    * @throws IOException
    *            if socket I/O error occurs
    * @throws NXCException
    *            if NetXMS server returns an error or operation was timed out
    */
   public void modifyUserDBObject(final AbstractUserObject object) throws IOException, NXCException
   {
      modifyUserDBObject(object, 0x7FFFFFFF);
   }

   /**
    * Lock user database
    * 
    * @throws IOException
    *            if socket I/O error occurs
    * @throws NXCException
    *            if NetXMS server returns an error or operation was timed out
    */
   public void lockUserDatabase() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_LOCK_USER_DB);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Unlock user database
    * 
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unlockUserDatabase() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UNLOCK_USER_DB);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Set custom attribute for currently logged in user. Server will allow to change
    * only attributes whose name starts with dot.
    * 
    * @param name Attribute's name
    * @param value New attribute's value
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setAttributeForCurrentUser(final String name, final String value) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_CURRENT_USER_ATTR);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_VALUE, value);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get custom attribute for currently logged in user. If attribute is not set, empty string will
    * be returned.
    * 
    * @param name Attribute's name
    * @return Attribute's value
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String getAttributeForCurrentUser(final String name) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_CURRENT_USER_ATTR);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsString(NXCPCodes.VID_VALUE);
   }

   /**
    * Get last DCI values for given node
    *
    * @param nodeId                ID of the node to get DCI values for
    * @param objectTooltipOnly     if set to true, only DCIs with DCF_SHOW_ON_OBJECT_TOOLTIP flag set are returned
    * @param overviewOnly          if set to true, only DCIs with DCF_SHOW_IN_OBJECT_OVERVIEW flag set are returned
    * @param includeNoValueObjects if set to true, objects with no value (like instance discovery DCIs) will be returned as well
    * @return List of DCI values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciValue[] getLastValues(final long nodeId, boolean objectTooltipOnly, boolean overviewOnly, boolean includeNoValueObjects) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_LAST_VALUES);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_OBJECT_TOOLTIP_ONLY, objectTooltipOnly);
      msg.setField(NXCPCodes.VID_OVERVIEW_ONLY, overviewOnly);
      msg.setField(NXCPCodes.VID_INCLUDE_NOVALUE_OBJECTS, includeNoValueObjects);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      DciValue[] list = new DciValue[count];
      long base = NXCPCodes.VID_DCI_VALUES_BASE;
      for(int i = 0; i < count; i++, base += 50)
      {
         list[i] = DciValue.createFromMessage(nodeId, response, base);
      }

      return list;
   }

   /**
    * Get last DCI values for given node
    *
    * @param nodeId ID of the node to get DCI values for
    * @return List of DCI values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciValue[] getLastValues(final long nodeId) throws IOException, NXCException
   {
      return getLastValues(nodeId, false, false, false);
   }
   
   /**
    * Get last DCI values for given DCI id list(list can contain simple DCIss and table DCIs) 
    *
    * @param dciIDList            List with all required data to get last DCI id
    * @return List of DCI values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciValue[] getLastValues(Set<MapDCIInstance> dciIDList) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DCI_VALUES);
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, dciIDList.size());
      long base = NXCPCodes.VID_DCI_ID_LIST_BASE;
      for(MapDCIInstance item : dciIDList)
      {
         item.fillMessage(msg, base);
         base +=10;
      }

      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      DciValue[] list = new DciValue[count];
      base = NXCPCodes.VID_DCI_ID_LIST_BASE;
      for(int i = 0; i < count; i++, base += 10)
      {
         list[i] = (DciValue)new SimpleDciValue(response.getFieldAsInt64(base), response.getFieldAsString(base+1), response.getFieldAsInt32(base+2), response.getFieldAsInt32(base+3));
      }

      return list;
   }

   /**
    * Get last values for given table DCI on given node
    *
    * @param nodeId ID of the node to get DCI values for
    * @param dciId  DCI ID
    * @return Table object with last values for table DCI
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table getTableLastValues(final long nodeId, final long dciId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_TABLE_LAST_VALUES);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setFieldInt32(NXCPCodes.VID_DCI_ID, (int) dciId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new Table(response);
   }

   /**
    * Get list of DCIs configured to be shown on performance tab in console for
    * given node.
    *
    * @param nodeId Node object ID
    * @return List of performance tab DCIs
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public PerfTabDci[] getPerfTabItems(final long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PERFTAB_DCI_LIST);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      PerfTabDci[] list = new PerfTabDci[count];
      long base = NXCPCodes.VID_SYSDCI_LIST_BASE;
      for(int i = 0; i < count; i++, base += 10)
      {
         list[i] = new PerfTabDci(response, base);
      }

      return list;
   }

   /**
    * Get threshold violation summary for all nodes under given parent object. Parent object could
    * be container, subnet, zone, entire network, or infrastructure service root.
    *
    * @param objectId parent object ID
    * @return list of threshold violation summary objects for all nodes below given root
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<ThresholdViolationSummary> getThresholdSummary(final long objectId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_THRESHOLD_SUMMARY);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      List<ThresholdViolationSummary> list = new ArrayList<ThresholdViolationSummary>();
      long varId = NXCPCodes.VID_THRESHOLD_BASE;
      while(response.getFieldAsInt64(varId) != 0)
      {
         final ThresholdViolationSummary t = new ThresholdViolationSummary(response, varId);
         list.add(t);
         varId += 50 * t.getDciList().size() + 2;
      }

      return list;
   }

   /**
    * Parse data from raw message CMD_DCI_DATA
    *
    * @param input Raw data
    * @param data  Data object to add rows to
    * @return number of received data rows
    */
   private int parseDataRows(final byte[] input, DciData data)
   {
      final NXCPDataInputStream inputStream = new NXCPDataInputStream(input);
      int rows = 0;

      try
      {
         inputStream.skipBytes(4); // DCI ID
         rows = inputStream.readInt();
         final int dataType = inputStream.readInt();
         data.setDataType(dataType);
         inputStream.skipBytes(4); // padding

         for(int i = 0; i < rows; i++)
         {
            long timestamp = inputStream.readUnsignedInt() * 1000; // convert to
            // milliseconds

            switch(dataType)
            {
               case DataCollectionItem.DT_INT:
                  data.addDataRow(new DciDataRow(new Date(timestamp), new Long(inputStream.readInt())));
                  break;
               case DataCollectionItem.DT_UINT:
                  data.addDataRow(new DciDataRow(new Date(timestamp), new Long(inputStream.readUnsignedInt())));
                  break;
               case DataCollectionItem.DT_INT64:
               case DataCollectionItem.DT_UINT64:
                  inputStream.skipBytes(4); // padding
                  data.addDataRow(new DciDataRow(new Date(timestamp), new Long(inputStream.readLong())));
                  break;
               case DataCollectionItem.DT_FLOAT:
                  inputStream.skipBytes(4); // padding
                  data.addDataRow(new DciDataRow(new Date(timestamp), new Double(inputStream.readDouble())));
                  break;
               case DataCollectionItem.DT_STRING:
                  StringBuilder sb = new StringBuilder(256);
                  int count;
                  for(count = MAX_DCI_STRING_VALUE_LENGTH; count > 0; count--)
                  {
                     char ch = inputStream.readChar();
                     if (ch == 0)
                     {
                        count--;
                        break;
                     }
                     sb.append(ch);
                  }
                  inputStream.skipBytes(count * 2);
                  data.addDataRow(new DciDataRow(new Date(timestamp), sb.toString()));
                  break;
            }
         }
      }
      catch(IOException e)
      {
      }

      inputStream.close();
      return rows;
   }

   /**
    * Get collected DCI data from server. Please note that you should specify
    * either row count limit or time from/to limit.
    *
    * @param nodeId     Node ID
    * @param dciId      DCI ID
    * @param instance   instance value (for table DCI only)
    * @param dataColumn name of column to retrieve data from (for table DCI only)
    * @param from       Start of time range or null for no limit
    * @param to         End of time range or null for no limit
    * @param maxRows    Maximum number of rows to retrieve or 0 for no limit
    * @return DCI data set
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private DciData getCollectedDataInternal(
      long nodeId, long dciId, String instance, String dataColumn, Date from, Date to, int maxRows)
      throws IOException, NXCException
   {
      NXCPMessage msg;
      if (instance != null) // table DCI
      {
         msg = newMessage(NXCPCodes.CMD_GET_TABLE_DCI_DATA);
         msg.setField(NXCPCodes.VID_INSTANCE, instance);
         msg.setField(NXCPCodes.VID_DATA_COLUMN, dataColumn);
      }
      else
      {
         msg = newMessage(NXCPCodes.CMD_GET_DCI_DATA);
      }
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setFieldInt32(NXCPCodes.VID_DCI_ID, (int) dciId);

      DciData data = new DciData(nodeId, dciId);

      int rowsReceived, rowsRemaining = maxRows;
      int timeFrom = (from != null) ? (int) (from.getTime() / 1000) : 0;
      int timeTo = (to != null) ? (int) (to.getTime() / 1000) : 0;

      do
      {
         msg.setMessageId(requestId.getAndIncrement());
         msg.setFieldInt32(NXCPCodes.VID_MAX_ROWS, maxRows);
         msg.setFieldInt32(NXCPCodes.VID_TIME_FROM, timeFrom);
         msg.setFieldInt32(NXCPCodes.VID_TIME_TO, timeTo);
         sendMessage(msg);

         waitForRCC(msg.getMessageId());

         NXCPMessage response = waitForMessage(NXCPCodes.CMD_DCI_DATA, msg.getMessageId());
         if (!response.isBinaryMessage()) throw new NXCException(RCC.INTERNAL_ERROR);

         rowsReceived = parseDataRows(response.getBinaryData(), data);
         if (((rowsRemaining == 0) || (rowsRemaining > MAX_DCI_DATA_ROWS)) && (rowsReceived == MAX_DCI_DATA_ROWS))
         {
            // adjust boundaries for next request
            if (rowsRemaining > 0) rowsRemaining -= rowsReceived;

            // Rows goes in newest to oldest order, so if we need to
            // retrieve additional data, we should update timeTo limit
            if (to != null)
            {
               DciDataRow row = data.getLastValue();
               if (row != null)
               {
                  // There should be only one value per second, so we set
                  // last row's timestamp - 1 second as new boundary
                  timeTo = (int) (row.getTimestamp().getTime() / 1000) - 1;
               }
            }
         }
      }
      while(rowsReceived == MAX_DCI_DATA_ROWS);

      return data;
   }

   /**
    * Get collected DCI data from server. Please note that you should specify
    * either row count limit or time from/to limit.
    *
    * @param nodeId  Node ID
    * @param dciId   DCI ID
    * @param from    Start of time range or null for no limit
    * @param to      End of time range or null for no limit
    * @param maxRows Maximum number of rows to retrieve or 0 for no limit
    * @return DCI data set
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciData getCollectedData(long nodeId, long dciId, Date from, Date to, int maxRows)
      throws IOException, NXCException
   {
      return getCollectedDataInternal(nodeId, dciId, null, null, from, to, maxRows);
   }

   /**
    * Get collected table DCI data from server. Please note that you should specify
    * either row count limit or time from/to limit.
    *
    * @param nodeId     Node ID
    * @param dciId      DCI ID
    * @param instance   instance value
    * @param dataColumn name of column to retrieve data from
    * @param from       Start of time range or null for no limit
    * @param to         End of time range or null for no limit
    * @param maxRows    Maximum number of rows to retrieve or 0 for no limit
    * @return DCI data set
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciData getCollectedTableData(
      long nodeId, long dciId, String instance, String dataColumn, Date from, Date to, int maxRows)
      throws IOException, NXCException
   {
      if (instance == null || dataColumn == null) throw new NXCException(RCC.INVALID_ARGUMENT);
      return getCollectedDataInternal(nodeId, dciId, instance, dataColumn, from, to, maxRows);
   }

   /**
    * Clear collected data for given DCI
    *
    * @param nodeId Node object ID
    * @param dciId  DCI ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void clearCollectedData(long nodeId, long dciId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CLEAR_DCI_DATA);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setFieldInt32(NXCPCodes.VID_DCI_ID, (int) dciId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Force DCI poll for given DCI
    *
    * @param nodeId Node object ID
    * @param dciId  DCI ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void forceDCIPoll(long nodeId, long dciId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FORCE_DCI_POLL);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setFieldInt32(NXCPCodes.VID_DCI_ID, (int) dciId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of thresholds configured for given DCI
    *
    * @param nodeId Node object ID
    * @param dciId  DCI ID
    * @return List of configured thresholds
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Threshold[] getThresholds(final long nodeId, final long dciId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DCI_THRESHOLDS);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setFieldInt32(NXCPCodes.VID_DCI_ID, (int) dciId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_THRESHOLDS);
      final Threshold[] list = new Threshold[count];

      long varId = NXCPCodes.VID_DCI_THRESHOLD_BASE;
      for(int i = 0; i < count; i++)
      {
         list[i] = new Threshold(response, varId);
         varId += 20;
      }

      return list;
   }

   /**
    * Resolve names of given DCIs
    *
    * @param nodeIds node identifiers
    * @param dciIds  DCI identifiers (length must match length of node identifiers list)
    * @return array of resolved DCI names
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String[] resolveDciNames(long[] nodeIds, long[] dciIds) throws IOException, NXCException
   {
      if (nodeIds.length == 0) return new String[0];

      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RESOLVE_DCI_NAMES);
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, nodeIds.length);
      msg.setField(NXCPCodes.VID_NODE_LIST, nodeIds);
      msg.setField(NXCPCodes.VID_DCI_LIST, dciIds);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      String[] result = new String[nodeIds.length];
      long varId = NXCPCodes.VID_DCI_LIST_BASE;
      for(int i = 0; i < result.length; i++)
      {
         result[i] = response.getFieldAsString(varId++);
      }
      return result;
   }

   /**
    * Resolve names of given DCIs
    *
    * @param dciList DCI list
    * @return array of resolved DCI names
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String[] resolveDciNames(Collection<ConditionDciInfo> dciList) throws IOException, NXCException
   {
      final long[] nodeIds = new long[dciList.size()];
      final long[] dciIds = new long[dciList.size()];
      int i = 0;
      for(ConditionDciInfo dci : dciList)
      {
         nodeIds[i] = dci.getNodeId();
         dciIds[i] = dci.getDciId();
         i++;
      }
      return resolveDciNames(nodeIds, dciIds);
   }

   /**
    * Query parameter immediately. This call will cause server to do actual call
    * to managed node and will return current value for given parameter. Result
    * is not cached.
    *
    * @param nodeId node object ID
    * @param origin parameter's origin (NetXMS agent, SNMP, etc.)
    * @param name   parameter's name
    * @return current parameter's value
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String queryParameter(long nodeId, int origin, String name) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_QUERY_PARAMETER);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setFieldInt16(NXCPCodes.VID_DCI_SOURCE_TYPE, origin);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsString(NXCPCodes.VID_VALUE);
   }

   /**
    * Query agent's table immediately. This call will cause server to do actual
    * call to managed node and will return current value for given table. Result
    * is not cached.
    *
    * @param nodeId node object ID
    * @param name   table's name
    * @return current table's value
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table queryAgentTable(long nodeId, String name) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_QUERY_TABLE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new Table(response);
   }

   /**
    * Hook method to allow adding of custom object creation data to NXCP message.
    * Default implementation does nothing.
    *
    * @param data     object creation data passed to createObject
    * @param userData user-defined data for object creation passed to createObject
    * @param msg      NXCP message that will be sent to server
    */
   protected void createCustomObject(NXCObjectCreationData data, Object userData, NXCPMessage msg)
   {
   }

   /**
    * Create new NetXMS object.
    *
    * @param data     Object creation data
    * @param userData User-defined data for custom object creation
    * @return ID of new object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long createObject(final NXCObjectCreationData data, final Object userData) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_OBJECT);

      // Common attributes
      msg.setFieldInt32(NXCPCodes.VID_PARENT_ID, (int) data.getParentId());
      msg.setFieldInt16(NXCPCodes.VID_OBJECT_CLASS, data.getObjectClass());
      msg.setField(NXCPCodes.VID_OBJECT_NAME, data.getName());
      msg.setFieldInt32(NXCPCodes.VID_ZONE_ID, (int) data.getZoneId());
      if (data.getComments() != null) msg.setField(NXCPCodes.VID_COMMENTS, data.getComments());

      // Class-specific attributes
      switch(data.getObjectClass())
      {
         case AbstractObject.OBJECT_NODE:
            if (data.getPrimaryName() != null) msg.setField(NXCPCodes.VID_PRIMARY_NAME, data.getPrimaryName());
            msg.setField(NXCPCodes.VID_IP_ADDRESS, data.getIpAddress());
            msg.setFieldInt16(NXCPCodes.VID_AGENT_PORT, data.getAgentPort());
            msg.setFieldInt16(NXCPCodes.VID_SNMP_PORT, data.getSnmpPort());
            msg.setFieldInt32(NXCPCodes.VID_CREATION_FLAGS, data.getCreationFlags());
            msg.setFieldInt32(NXCPCodes.VID_AGENT_PROXY, (int) data.getAgentProxyId());
            msg.setFieldInt32(NXCPCodes.VID_SNMP_PROXY, (int) data.getSnmpProxyId());
            break;
         case AbstractObject.OBJECT_NETWORKMAP:
            msg.setFieldInt16(NXCPCodes.VID_MAP_TYPE, data.getMapType());
            msg.setFieldInt32(NXCPCodes.VID_SEED_OBJECT, (int) data.getSeedObjectId());
            msg.setFieldInt32(NXCPCodes.VID_FLAGS, (int) data.getFlags());
            break;
         case AbstractObject.OBJECT_NETWORKSERVICE:
            msg.setFieldInt16(NXCPCodes.VID_SERVICE_TYPE, data.getServiceType());
            msg.setFieldInt16(NXCPCodes.VID_IP_PROTO, data.getIpProtocol());
            msg.setFieldInt16(NXCPCodes.VID_IP_PORT, data.getIpPort());
            msg.setField(NXCPCodes.VID_SERVICE_REQUEST, data.getRequest());
            msg.setField(NXCPCodes.VID_SERVICE_RESPONSE, data.getResponse());
            msg.setFieldInt16(NXCPCodes.VID_CREATE_STATUS_DCI, data.isCreateStatusDci() ? 1 : 0);
            break;
         case AbstractObject.OBJECT_NODELINK:
            msg.setFieldInt32(NXCPCodes.VID_NODE_ID, (int) data.getLinkedNodeId());
            break;
         case AbstractObject.OBJECT_SLMCHECK:
            msg.setFieldInt16(NXCPCodes.VID_IS_TEMPLATE, data.isTemplate() ? 1 : 0);
            break;
         case AbstractObject.OBJECT_INTERFACE:
            msg.setField(NXCPCodes.VID_MAC_ADDR, data.getMacAddress().getValue());
            msg.setField(NXCPCodes.VID_IP_ADDRESS, data.getIpAddress());
            msg.setFieldInt32(NXCPCodes.VID_IF_TYPE, data.getIfType());
            msg.setFieldInt32(NXCPCodes.VID_IF_INDEX, data.getIfIndex());
            msg.setFieldInt32(NXCPCodes.VID_IF_SLOT, data.getSlot());
            msg.setFieldInt32(NXCPCodes.VID_IF_PORT, data.getPort());
            msg.setFieldInt16(NXCPCodes.VID_IS_PHYS_PORT, data.isPhysicalPort() ? 1 : 0);
            break;
         case AbstractObject.OBJECT_MOBILEDEVICE:
            msg.setField(NXCPCodes.VID_DEVICE_ID, data.getDeviceId());
            break;
         case AbstractObject.OBJECT_RACK:
            msg.setFieldInt16(NXCPCodes.VID_HEIGHT, data.getHeight());
            break;
      }

      if (userData != null) createCustomObject(data, userData, msg);

      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID);
   }

   /**
    * Create new NetXMS object. Equivalent of calling createObject(data, null).
    *
    * @param data Object creation data
    * @return ID of new object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long createObject(final NXCObjectCreationData data) throws IOException, NXCException
   {
      return createObject(data, null);
   }

   /**
    * Delete object
    *
    * @param objectId ID of an object which should be deleted
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteObject(final long objectId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_OBJECT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      // If server reports success, delete object from cache and generate
      // appropriate notification without waiting for actual server update
      synchronized(objectList)
      {
         AbstractObject object = objectList.get(objectId);
         if (object != null)
         {
            objectList.remove(objectId);
            removeOrphanedObjects(object);
         }
      }
      sendNotification(new SessionNotification(SessionNotification.OBJECT_DELETED, objectId));
   }
   
   /**
    * Remove orphaned objects (with last parent left)
    * 
    * @param parent
    */
   private void removeOrphanedObjects(AbstractObject parent)
   {
      Iterator<Long> it = parent.getChildren();
      while(it.hasNext())
      {
         AbstractObject object = objectList.get(it.next());
         if ((object != null) && (object.getParentCount() == 1))
         {
            objectList.remove(object.getObjectId());
            removeOrphanedObjects(object);
         }
      }
   }

   /**
    * Hook method to populate NXCP message with custom object's data on object modification.
    * Default implementation does nothing.
    *
    * @param data     object modification data passed to modifyObject
    * @param userData user-defined data passed to modifyObject
    * @param msg      NXCP message to be sent to server
    */
   protected void modifyCustomObject(NXCObjectModificationData data, Object userData, NXCPMessage msg)
   {
   }

   /**
    * Modify object (generic interface, in most cases wrapper functions should
    * be used instead).
    *
    * @param data     Object modification data
    * @param userData user-defined data for custom object modification
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyObject(final NXCObjectModificationData data, final Object userData) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_OBJECT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) data.getObjectId());

      long flags = data.getFlags();

      // Object name
      if ((flags & NXCObjectModificationData.MODIFY_NAME) != 0)
      {
         msg.setField(NXCPCodes.VID_OBJECT_NAME, data.getName());
      }

      // Primary IP
      if ((flags & NXCObjectModificationData.MODIFY_PRIMARY_IP) != 0)
      {
         msg.setField(NXCPCodes.VID_IP_ADDRESS, data.getPrimaryIpAddress());
      }

      // Access control list
      if ((flags & NXCObjectModificationData.MODIFY_ACL) != 0)
      {
         final AccessListElement[] acl = data.getACL();
         msg.setFieldInt32(NXCPCodes.VID_ACL_SIZE, acl.length);
         msg.setFieldInt16(NXCPCodes.VID_INHERIT_RIGHTS, data.isInheritAccessRights() ? 1 : 0);

         long id1 = NXCPCodes.VID_ACL_USER_BASE;
         long id2 = NXCPCodes.VID_ACL_RIGHTS_BASE;
         for(int i = 0; i < acl.length; i++)
         {
            msg.setFieldInt32(id1++, (int) acl[i].getUserId());
            msg.setFieldInt32(id2++, acl[i].getAccessRights());
         }
      }

      if ((flags & NXCObjectModificationData.MODIFY_CUSTOM_ATTRIBUTES) != 0)
      {
         Map<String, String> attrList = data.getCustomAttributes();
         Iterator<String> it = attrList.keySet().iterator();
         long id = NXCPCodes.VID_CUSTOM_ATTRIBUTES_BASE;
         int count = 0;
         while(it.hasNext())
         {
            String key = it.next();
            String value = attrList.get(key);
            msg.setField(id++, key);
            msg.setField(id++, value);
            count++;
         }
         msg.setFieldInt32(NXCPCodes.VID_NUM_CUSTOM_ATTRIBUTES, count);
      }

      if ((flags & NXCObjectModificationData.MODIFY_AUTOBIND_FILTER) != 0)
      {
         msg.setField(NXCPCodes.VID_AUTOBIND_FILTER, data.getAutoBindFilter());
      }

      if ((flags & NXCObjectModificationData.MODIFY_FILTER) != 0)
      {
         msg.setField(NXCPCodes.VID_FILTER, data.getFilter());
      }

      if ((flags & NXCObjectModificationData.MODIFY_DESCRIPTION) != 0)
      {
         msg.setField(NXCPCodes.VID_DESCRIPTION, data.getDescription());
      }

      if ((flags & NXCObjectModificationData.MODIFY_VERSION) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_VERSION, data.getVersion());
      }

      // Configuration file
      if ((flags & NXCObjectModificationData.MODIFY_POLICY_CONFIG) != 0)
      {
         msg.setField(NXCPCodes.VID_CONFIG_FILE_DATA, data.getConfigFileContent());
      }

      if ((flags & NXCObjectModificationData.MODIFY_AGENT_PORT) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_AGENT_PORT, data.getAgentPort());
      }

      if ((flags & NXCObjectModificationData.MODIFY_AGENT_PROXY) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_AGENT_PROXY, (int) data.getAgentProxy());
      }

      if ((flags & NXCObjectModificationData.MODIFY_AGENT_AUTH) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_AUTH_METHOD, data.getAgentAuthMethod());
         msg.setField(NXCPCodes.VID_SHARED_SECRET, data.getAgentSecret());
      }

      if ((flags & NXCObjectModificationData.MODIFY_TRUSTED_NODES) != 0)
      {
         final long[] nodes = data.getTrustedNodes();
         msg.setFieldInt32(NXCPCodes.VID_NUM_TRUSTED_NODES, nodes.length);
         msg.setField(NXCPCodes.VID_TRUSTED_NODES, nodes);
      }

      if ((flags & NXCObjectModificationData.MODIFY_SNMP_VERSION) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_SNMP_VERSION, data.getSnmpVersion());
      }

      if ((flags & NXCObjectModificationData.MODIFY_SNMP_AUTH) != 0)
      {
         msg.setField(NXCPCodes.VID_SNMP_AUTH_OBJECT, data.getSnmpAuthName());
         msg.setField(NXCPCodes.VID_SNMP_AUTH_PASSWORD, data.getSnmpAuthPassword());
         msg.setField(NXCPCodes.VID_SNMP_PRIV_PASSWORD, data.getSnmpPrivPassword());
         int methods = data.getSnmpAuthMethod() | (data.getSnmpPrivMethod() << 8);
         msg.setFieldInt16(NXCPCodes.VID_SNMP_USM_METHODS, methods);
      }

      if ((flags & NXCObjectModificationData.MODIFY_SNMP_PROXY) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_SNMP_PROXY, (int) data.getSnmpProxy());
      }

      if ((flags & NXCObjectModificationData.MODIFY_SNMP_PORT) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_SNMP_PORT, data.getSnmpPort());
      }

      if ((flags & NXCObjectModificationData.MODIFY_ICMP_PROXY) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_ICMP_PROXY, (int) data.getIcmpProxy());
      }

      if ((flags & NXCObjectModificationData.MODIFY_GEOLOCATION) != 0)
      {
         final GeoLocation gl = data.getGeolocation();
         msg.setFieldInt16(NXCPCodes.VID_GEOLOCATION_TYPE, gl.getType());
         msg.setField(NXCPCodes.VID_LATITUDE, gl.getLatitude());
         msg.setField(NXCPCodes.VID_LONGITUDE, gl.getLongitude());
         msg.setFieldInt16(NXCPCodes.VID_ACCURACY, gl.getAccuracy());
         if (gl.getTimestamp() != null)
         {
            msg.setFieldInt64(NXCPCodes.VID_GEOLOCATION_TIMESTAMP, gl.getTimestamp().getTime() / 1000);
         }
      }

      if ((flags & NXCObjectModificationData.MODIFY_MAP_LAYOUT) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_LAYOUT, data.getMapLayout().getValue());
      }

      if ((flags & NXCObjectModificationData.MODIFY_MAP_BACKGROUND) != 0)
      {
         msg.setField(NXCPCodes.VID_BACKGROUND, data.getMapBackground());
         msg.setField(NXCPCodes.VID_BACKGROUND_LATITUDE, data.getMapBackgroundLocation().getLatitude());
         msg.setField(NXCPCodes.VID_BACKGROUND_LONGITUDE, data.getMapBackgroundLocation().getLongitude());
         msg.setFieldInt16(NXCPCodes.VID_BACKGROUND_ZOOM, data.getMapBackgroundZoom());
         msg.setFieldInt32(NXCPCodes.VID_BACKGROUND_COLOR, data.getMapBackgroundColor());
      }

      if ((flags & NXCObjectModificationData.MODIFY_IMAGE) != 0)
      {
         msg.setField(NXCPCodes.VID_IMAGE, data.getImage());
      }

      if ((flags & NXCObjectModificationData.MODIFY_MAP_CONTENT) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_NUM_ELEMENTS, data.getMapElements().size());
         long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
         for(NetworkMapElement e : data.getMapElements())
         {
            e.fillMessage(msg, varId);
            varId += 100;
         }

         msg.setFieldInt32(NXCPCodes.VID_NUM_LINKS, data.getMapLinks().size());
         varId = NXCPCodes.VID_LINK_LIST_BASE;
         for(NetworkMapLink l : data.getMapLinks())
         {
            l.fillMessage(msg, varId);
            varId += 20;
         }
      }

      if ((flags & NXCObjectModificationData.MODIFY_COLUMN_COUNT) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_NUM_COLUMNS, data.getColumnCount());
      }

      if ((flags & NXCObjectModificationData.MODIFY_DASHBOARD_ELEMENTS) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_NUM_ELEMENTS, data.getDashboardElements().size());
         long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
         for(DashboardElement e : data.getDashboardElements())
         {
            e.fillMessage(msg, varId);
            varId += 10;
         }
      }

      if ((flags & NXCObjectModificationData.MODIFY_SCRIPT) != 0)
      {
         msg.setField(NXCPCodes.VID_SCRIPT, data.getScript());
      }

      if ((flags & NXCObjectModificationData.MODIFY_ACTIVATION_EVENT) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_ACTIVATION_EVENT, data.getActivationEvent());
      }

      if ((flags & NXCObjectModificationData.MODIFY_DEACTIVATION_EVENT) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_DEACTIVATION_EVENT, data.getDeactivationEvent());
      }

      if ((flags & NXCObjectModificationData.MODIFY_SOURCE_OBJECT) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_SOURCE_OBJECT, (int) data.getSourceObject());
      }

      if ((flags & NXCObjectModificationData.MODIFY_ACTIVE_STATUS) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_ACTIVE_STATUS, data.getActiveStatus());
      }

      if ((flags & NXCObjectModificationData.MODIFY_INACTIVE_STATUS) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_INACTIVE_STATUS, data.getInactiveStatus());
      }

      if ((flags & NXCObjectModificationData.MODIFY_DCI_LIST) != 0)
      {
         List<ConditionDciInfo> dciList = data.getDciList();
         msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, dciList.size());
         long varId = NXCPCodes.VID_DCI_LIST_BASE;
         for(ConditionDciInfo dci : dciList)
         {
            msg.setFieldInt32(varId++, (int) dci.getDciId());
            msg.setFieldInt32(varId++, (int) dci.getNodeId());
            msg.setFieldInt16(varId++, dci.getFunction());
            msg.setFieldInt16(varId++, dci.getPolls());
            varId += 6;
         }
      }

      if ((flags & NXCObjectModificationData.MODIFY_SUBMAP_ID) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_SUBMAP_ID, (int) data.getSubmapId());
      }

      if ((flags & NXCObjectModificationData.MODIFY_SERVICE_TYPE) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_SERVICE_TYPE, data.getServiceType());
      }

      if ((flags & NXCObjectModificationData.MODIFY_IP_PROTOCOL) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_IP_PROTO, data.getIpProtocol());
      }

      if ((flags & NXCObjectModificationData.MODIFY_IP_PORT) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_IP_PORT, data.getIpPort());
      }

      if ((flags & NXCObjectModificationData.MODIFY_POLLER_NODE) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_POLLER_NODE_ID, (int) data.getPollerNode());
      }

      if ((flags & NXCObjectModificationData.MODIFY_REQUIRED_POLLS) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_REQUIRED_POLLS, data.getRequiredPolls());
      }

      if ((flags & NXCObjectModificationData.MODIFY_REQUEST) != 0)
      {
         msg.setField(NXCPCodes.VID_SERVICE_REQUEST, data.getRequest());
      }

      if ((flags & NXCObjectModificationData.MODIFY_RESPONSE) != 0)
      {
         msg.setField(NXCPCodes.VID_SERVICE_RESPONSE, data.getResponse());
      }

      if ((flags & NXCObjectModificationData.MODIFY_OBJECT_FLAGS) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_FLAGS, data.getObjectFlags());
      }

      if ((flags & NXCObjectModificationData.MODIFY_IFXTABLE_POLICY) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_USE_IFXTABLE, data.getIfXTablePolicy());
      }

      if ((flags & NXCObjectModificationData.MODIFY_REPORT_DEFINITION) != 0)
      {
         msg.setField(NXCPCodes.VID_REPORT_DEFINITION, data.getReportDefinition());
      }

      if ((flags & NXCObjectModificationData.MODIFY_CLUSTER_RESOURCES) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_NUM_RESOURCES, data.getResourceList().size());
         long varId = NXCPCodes.VID_RESOURCE_LIST_BASE;
         for(ClusterResource r : data.getResourceList())
         {
            msg.setFieldInt32(varId++, (int) r.getId());
            msg.setField(varId++, r.getName());
            msg.setField(varId++, r.getVirtualAddress());
            varId += 7;
         }
      }

      if ((flags & NXCObjectModificationData.MODIFY_CLUSTER_NETWORKS) != 0)
      {
         int count = data.getNetworkList().size();
         msg.setFieldInt32(NXCPCodes.VID_NUM_SYNC_SUBNETS, count);
         long varId = NXCPCodes.VID_SYNC_SUBNETS_BASE;
         for(InetAddressEx n : data.getNetworkList())
         {
            msg.setField(varId++, n);
         }
      }

      if ((flags & NXCObjectModificationData.MODIFY_PRIMARY_NAME) != 0)
      {
         msg.setField(NXCPCodes.VID_PRIMARY_NAME, data.getPrimaryName());
      }

      if ((flags & NXCObjectModificationData.MODIFY_STATUS_CALCULATION) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_STATUS_CALCULATION_ALG, data.getStatusCalculationMethod());
         msg.setFieldInt16(NXCPCodes.VID_STATUS_PROPAGATION_ALG, data.getStatusPropagationMethod());
         msg.setFieldInt16(NXCPCodes.VID_FIXED_STATUS, data.getFixedPropagatedStatus().getValue());
         msg.setFieldInt16(NXCPCodes.VID_STATUS_SHIFT, data.getStatusShift());
         ObjectStatus[] transformation = data.getStatusTransformation();
         msg.setFieldInt16(NXCPCodes.VID_STATUS_TRANSLATION_1, transformation[0].getValue());
         msg.setFieldInt16(NXCPCodes.VID_STATUS_TRANSLATION_2, transformation[1].getValue());
         msg.setFieldInt16(NXCPCodes.VID_STATUS_TRANSLATION_3, transformation[2].getValue());
         msg.setFieldInt16(NXCPCodes.VID_STATUS_TRANSLATION_4, transformation[3].getValue());
         msg.setFieldInt16(NXCPCodes.VID_STATUS_SINGLE_THRESHOLD, data.getStatusSingleThreshold());
         int[] thresholds = data.getStatusThresholds();
         msg.setFieldInt16(NXCPCodes.VID_STATUS_THRESHOLD_1, thresholds[0]);
         msg.setFieldInt16(NXCPCodes.VID_STATUS_THRESHOLD_2, thresholds[1]);
         msg.setFieldInt16(NXCPCodes.VID_STATUS_THRESHOLD_3, thresholds[2]);
         msg.setFieldInt16(NXCPCodes.VID_STATUS_THRESHOLD_4, thresholds[3]);
      }

      if ((flags & NXCObjectModificationData.MODIFY_EXPECTED_STATE) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_EXPECTED_STATE, data.getExpectedState());
      }

      if ((flags & NXCObjectModificationData.MODIFY_LINK_COLOR) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_LINK_COLOR, data.getLinkColor());
      }

      if ((flags & NXCObjectModificationData.MODIFY_CONNECTION_ROUTING) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_LINK_ROUTING, data.getConnectionRouting());
      }

      if ((flags & NXCObjectModificationData.MODIFY_DISCOVERY_RADIUS) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_DISCOVERY_RADIUS, data.getDiscoveryRadius());
      }

      if ((flags & NXCObjectModificationData.MODIFY_HEIGHT) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_HEIGHT, data.getHeight());
      }

      if ((flags & NXCObjectModificationData.MODIFY_PEER_GATEWAY) != 0)
      {
         msg.setFieldInt32(NXCPCodes.VID_PEER_GATEWAY, (int)data.getPeerGatewayId());
      }

      if ((flags & NXCObjectModificationData.MODIFY_VPN_NETWORKS) != 0)
      {
         long fieldId = NXCPCodes.VID_VPN_NETWORK_BASE;         

         msg.setFieldInt32(NXCPCodes.VID_NUM_LOCAL_NETS, data.getLocalNetworks().size());
         for(InetAddressEx a : data.getLocalNetworks())
         {
            msg.setField(fieldId++, a);
         }

         msg.setFieldInt32(NXCPCodes.VID_NUM_REMOTE_NETS, data.getRemoteNetworks().size());
         for(InetAddressEx a : data.getRemoteNetworks())
         {
            msg.setField(fieldId++, a);
         }
      }
      
      if ((flags & NXCObjectModificationData.MODIFY_POSTAL_ADDRESS) != 0)
      {
         data.getPostalAddress().fillMessage(msg);
      }
      
      if ((flags & NXCObjectModificationData.MODIFY_AGENT_CACHE_MODE) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_AGENT_CACHE_MODE, data.getAgentCacheMode().getValue());
      }
      
      if ((flags & NXCObjectModificationData.MODIFY_MAPOBJ_DISP_MODE) != 0)
      {
         msg.setFieldInt16(NXCPCodes.VID_DISPLAY_MODE, data.getMapObjectDisplayMode().getValue());
      }

      modifyCustomObject(data, userData, msg);

      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Modify object (generic interface, in most cases wrapper functions should
    * be used instead). Equivalent of calling modifyObject(data, null).
    *
    * @param data Object modification data
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyObject(final NXCObjectModificationData data) throws IOException, NXCException
   {
      modifyObject(data, null);
   }

   /**
    * Change object's name (wrapper for modifyObject())
    *
    * @param objectId ID of object to be changed
    * @param name     New object's name
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setObjectName(final long objectId, final String name) throws IOException, NXCException
   {
      NXCObjectModificationData data = new NXCObjectModificationData(objectId);
      data.setName(name);
      modifyObject(data);
   }

   /**
    * Change object's custom attributes (wrapper for modifyObject())
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setObjectCustomAttributes(final long objectId, final Map<String, String> attrList)
      throws IOException, NXCException
   {
      NXCObjectModificationData data = new NXCObjectModificationData(objectId);
      data.setCustomAttributes(attrList);
      modifyObject(data);
   }

   /**
    * Change object's ACL (wrapper for modifyObject())
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setObjectACL(final long objectId, final AccessListElement[] acl, final boolean inheritAccessRights)
      throws IOException, NXCException
   {
      NXCObjectModificationData data = new NXCObjectModificationData(objectId);
      data.setACL(acl);
      data.setInheritAccessRights(inheritAccessRights);
      modifyObject(data);
   }
   
   /**
    * Move object to different zone. Only nodes and clusters can be moved
    * between zones.
    *
    * @param objectId Node or cluster object ID
    * @param zoneId
    * @throws IOException
    * @throws NXCException
    */
   public void changeObjectZone(final long objectId, final long zoneId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_ZONE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_ID, (int) zoneId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Change object's comments.
    *
    * @param objectId Object's ID
    * @param comments New comments
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateObjectComments(final long objectId, final String comments) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_OBJECT_COMMENTS);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      msg.setField(NXCPCodes.VID_COMMENTS, comments);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Set object's managed status.
    *
    * @param objectId  object's identifier
    * @param isManaged object's managed status
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setObjectManaged(final long objectId, final boolean isManaged) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_OBJECT_MGMT_STATUS);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      msg.setFieldInt16(NXCPCodes.VID_MGMT_STATUS, isManaged ? 1 : 0);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get effective rights of currently logged in user to given object.
    * 
    * @param objectId
    * @return
    * @throws IOException
    * @throws NXCException
    */
   public int getEffectiveRights(final long objectId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_EFFECTIVE_RIGHTS);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      sendMessage(msg);
      return waitForRCC(msg.getMessageId()).getFieldAsInt32(NXCPCodes.VID_EFFECTIVE_RIGHTS);
   }


   /**
    * Common internal implementation for bindObject, unbindObject, and
    * removeTemplate
    *
    * @param parentId  parent object's identifier
    * @param childId   Child object's identifier
    * @param bind      true if operation is "bind"
    * @param removeDci true if DCIs created from template should be removed during
    *                  unbind
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private void changeObjectBinding(long parentId, long childId, boolean bind, boolean removeDci)
      throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(bind ? NXCPCodes.CMD_BIND_OBJECT : NXCPCodes.CMD_UNBIND_OBJECT);
      msg.setFieldInt32(NXCPCodes.VID_PARENT_ID, (int) parentId);
      msg.setFieldInt32(NXCPCodes.VID_CHILD_ID, (int) childId);
      msg.setFieldInt16(NXCPCodes.VID_REMOVE_DCI, removeDci ? 1 : 0);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Bind object.
    *
    * @param parentId parent object's identifier
    * @param childId  Child object's identifier
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void bindObject(final long parentId, final long childId) throws IOException, NXCException
   {
      changeObjectBinding(parentId, childId, true, false);
   }

   /**
    * Unbind object.
    *
    * @param parentId parent object's identifier
    * @param childId  Child object's identifier
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unbindObject(final long parentId, final long childId) throws IOException, NXCException
   {
      changeObjectBinding(parentId, childId, false, false);
   }

   /**
    * Remove data collection template from node.
    *
    * @param templateId template object identifier
    * @param nodeId     node object identifier
    * @param removeDci  true if DCIs created from this template should be removed
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void removeTemplate(final long templateId, final long nodeId, final boolean removeDci)
      throws IOException, NXCException
   {
      changeObjectBinding(templateId, nodeId, false, removeDci);
   }

   /**
    * Apply data collection template to node.
    *
    * @param templateId template object ID
    * @param nodeId     node object ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void applyTemplate(long templateId, long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_APPLY_TEMPLATE);
      msg.setFieldInt32(NXCPCodes.VID_SOURCE_OBJECT_ID, (int) templateId);
      msg.setFieldInt32(NXCPCodes.VID_DESTINATION_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Add node to cluster.
    *
    * @param clusterId cluster object ID
    * @param nodeId    node object ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void addClusterNode(final long clusterId, final long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_ADD_CLUSTER_NODE);
      msg.setFieldInt32(NXCPCodes.VID_PARENT_ID, (int) clusterId);
      msg.setFieldInt32(NXCPCodes.VID_CHILD_ID, (int) nodeId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Remove node from cluster.
    *
    * @param clusterId cluster object ID
    * @param nodeId    node object ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void removeClusterNode(final long clusterId, final long nodeId) throws IOException, NXCException
   {
      changeObjectBinding(clusterId, nodeId, false, true);
   }

   /**
    * Query layer 2 topology for node
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public NetworkMapPage queryLayer2Topology(final long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_QUERY_L2_TOPOLOGY);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_OBJECTS);
      long[] idList = response.getFieldAsUInt32Array(NXCPCodes.VID_OBJECT_LIST);
      if (idList.length != count) 
         throw new NXCException(RCC.INTERNAL_ERROR);

      NetworkMapPage page = new NetworkMapPage(msg.getMessageId() + ".L2Topology");
      for(int i = 0; i < count; i++)
      {
         page.addElement(new NetworkMapObject(page.createElementId(), idList[i]));
      }

      count = response.getFieldAsInt32(NXCPCodes.VID_NUM_LINKS);
      long varId = NXCPCodes.VID_OBJECT_LINKS_BASE;
      for(int i = 0; i < count; i++, varId += 4)
      {
         NetworkMapObject obj1 = page.findObjectElement(response.getFieldAsInt64(varId++));
         NetworkMapObject obj2 = page.findObjectElement(response.getFieldAsInt64(varId++));
         int type = response.getFieldAsInt32(varId++);
         String port1 = response.getFieldAsString(varId++);
         String port2 = response.getFieldAsString(varId++);
         int flags = response.getFieldAsInt32(varId++);
         if ((obj1 != null) && (obj2 != null))
         {
            page.addLink(new NetworkMapLink("", type, obj1.getId(), obj2.getId(), port1, port2, flags));
         }
      }
      return page;
   }

   /**
    * Execute action on remote agent
    *
    * @param nodeId Node object ID
    * @param action Action name
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeAction(long nodeId, String action) throws IOException, NXCException
   {
      executeAction(nodeId, action, false, null, null);
   }

   /**
    * Execute action on remote agent
    *
    * @param nodeId Node object ID
    * @param action Action name
    * @param receiveOutput true if action's output has to be read
    * @param listener listener for action's output or null
    * @param writer writer for action's output or null
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeAction(long nodeId, String action, boolean receiveOutput, final TextOutputListener listener, final Writer writer) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_ACTION);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_ACTION_NAME, action);
      msg.setField(NXCPCodes.VID_RECEIVE_OUTPUT, receiveOutput);
      
      MessageHandler handler = receiveOutput ? new MessageHandler() {
         @Override
         public boolean processMessage(NXCPMessage m)
         {
            String text = m.getFieldAsString(NXCPCodes.VID_MESSAGE);
            if (text != null)
            {
               if (listener != null)
                  listener.messageReceived(text);
               if (writer != null)
               {
                  try
                  {
                     writer.write(text);
                  }
                  catch(IOException e)
                  {
                  }
               }
            }
            if (m.isEndOfSequence())
               setComplete();
            return true;
         }
      } : null;
      if (receiveOutput)
         addMessageSubscription(NXCPCodes.CMD_COMMAND_OUTPUT, msg.getMessageId(), handler);
      
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      if (receiveOutput)
      {
         synchronized(handler)
         {
            try
            {
               handler.wait();
            }
            catch(InterruptedException e)
            {
            }
         }
         if (handler.isTimeout())
            throw new NXCException(RCC.TIMEOUT);
      }      
   }
   
   /**
    * Wakeup node by sending wake-on-LAN magic packet. Either node ID or
    * interface ID may be given. If node ID is given, system will send wakeup
    * packets to all active interfaces with IP address.
    *
    * @param objectId node or interface ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void wakeupNode(final long objectId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_WAKEUP_NODE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get node's physical components (obtained from ENTITY-MIB).
    *
    * @param nodeId node object identifier
    * @return root component
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public PhysicalComponent getNodePhysicalComponents(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_NODE_COMPONENTS);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new PhysicalComponent(response, NXCPCodes.VID_COMPONENT_LIST_BASE, null);
   }

   /**
    * Get list of available Windows performance objects. Returns empty list if node
    * does is not a Windows node or does not have WinPerf subagent installed.
    *
    * @param nodeId node object ID
    * @return list of available Windows performance objects
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<WinPerfObject> getNodeWinPerfObjects(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_WINPERF_OBJECTS);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return WinPerfObject.createListFromMessage(response);
   }

   /**
    * Get list of software packages installed on node.
    *
    * @param nodeId node object identifier
    * @return root component
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<SoftwarePackage> getNodeSoftwarePackages(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_NODE_SOFTWARE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<SoftwarePackage> packages = new ArrayList<SoftwarePackage>(count);
      long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         packages.add(new SoftwarePackage(response, varId));
         varId += 10;
      }
      return packages;
   }

   /**
    * Get list of server jobs
    *
    * @return list of server jobs
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ServerJob[] getServerJobList() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_JOB_LIST);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_JOB_COUNT);
      ServerJob[] jobList = new ServerJob[count];
      long baseVarId = NXCPCodes.VID_JOB_LIST_BASE;
      for(int i = 0; i < count; i++, baseVarId += 10)
      {
         jobList[i] = new ServerJob(response, baseVarId);
      }

      return jobList;
   }

   /**
    * Cancel server job
    *
    * @param jobId Job ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void cancelServerJob(long jobId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_CANCEL_JOB);
      msg.setFieldInt32(NXCPCodes.VID_JOB_ID, (int) jobId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Put server job on hold
    *
    * @param jobId Job ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void holdServerJob(long jobId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_HOLD_JOB);
      msg.setFieldInt32(NXCPCodes.VID_JOB_ID, (int) jobId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Put server on hold job to pending state
    *
    * @param jobId Job ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unholdServerJob(long jobId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UNHOLD_JOB);
      msg.setFieldInt32(NXCPCodes.VID_JOB_ID, (int) jobId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Deploy policy on agent
    *
    * @param policyId Policy object ID
    * @param nodeId   Node object ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deployAgentPolicy(final long policyId, final long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DEPLOY_AGENT_POLICY);
      msg.setFieldInt32(NXCPCodes.VID_POLICY_ID, (int) policyId);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Uninstall policy from agent
    *
    * @param policyId Policy object ID
    * @param nodeId   Node object ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void uninstallAgentPolicy(final long policyId, final long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UNINSTALL_AGENT_POLICY);
      msg.setFieldInt32(NXCPCodes.VID_POLICY_ID, (int) policyId);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Internal implementation for open/get event processing policy.
    *
    * @param readOnly true to get read-only copy of the policy
    * @return Event processing policy
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private EventProcessingPolicy getEventProcessingPolicyInternal(boolean readOnly) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_OPEN_EPP);
      msg.setFieldInt16(NXCPCodes.VID_READ_ONLY, readOnly ? 1 : 0);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());

      int numRules = response.getFieldAsInt32(NXCPCodes.VID_NUM_RULES);
      final EventProcessingPolicy policy = new EventProcessingPolicy(numRules);

      for(int i = 0; i < numRules; i++)
      {
         response = waitForMessage(NXCPCodes.CMD_EPP_RECORD, msg.getMessageId());
         policy.addRule(new EventProcessingPolicyRule(response));
      }

      return policy;
   }

   /**
    * Get read-only copy of ebent processing policy.
    *
    * @return Event processing policy
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public EventProcessingPolicy getEventProcessingPolicy() throws IOException, NXCException
   {
      return getEventProcessingPolicyInternal(true);
   }

   /**
    * Open event processing policy for editing. This call will lock event
    * processing policy on server until closeEventProcessingPolicy called or
    * session terminated.
    *
    * @return Event processing policy
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public EventProcessingPolicy openEventProcessingPolicy() throws IOException, NXCException
   {
      return getEventProcessingPolicyInternal(false);
   }

   /**
    * Save event processing policy. If policy was not previously open by current
    * session exception will be thrown.
    *
    * @param epp Modified event processing policy
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void saveEventProcessingPolicy(EventProcessingPolicy epp) throws IOException, NXCException
   {
      final List<EventProcessingPolicyRule> rules = epp.getRules();

      NXCPMessage msg = newMessage(NXCPCodes.CMD_SAVE_EPP);
      msg.setFieldInt32(NXCPCodes.VID_NUM_RULES, rules.size());
      sendMessage(msg);
      final long msgId = msg.getMessageId();
      waitForRCC(msgId);

      int id = 1;
      for(EventProcessingPolicyRule rule : rules)
      {
         msg = new NXCPMessage(NXCPCodes.CMD_EPP_RECORD);
         msg.setMessageId(msgId);
         msg.setFieldInt32(NXCPCodes.VID_RULE_ID, id++);
         rule.fillMessage(msg);
         sendMessage(msg);
      }

      // Wait for final confirmation if there was some rules
      if (rules.size() > 0) waitForRCC(msgId);
   }

   /**
    * Close event processing policy. This call will unlock event processing
    * policy on server. If policy was not previously open by current session
    * exception will be thrown.
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void closeEventProcessingPolicy() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_CLOSE_EPP);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Open data collection configuration for given node. You must call
    * DataCollectionConfiguration.close() to close data collection configuration
    * when it is no longer needed.
    *
    * @param nodeId Node object identifier
    * @return Data collection configuration object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DataCollectionConfiguration openDataCollectionConfiguration(long nodeId) throws IOException, NXCException
   {
      final DataCollectionConfiguration cfg = new DataCollectionConfiguration(this, nodeId);
      cfg.open();
      return cfg;
   }

   /**
    * Test DCI transformation script.
    *
    * @param nodeId     ID of the node object to test script on
    * @param script     script source code
    * @param inputValue input value for the script
    * @return test execution results
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public TransformationTestResult testTransformationScript(long nodeId, String script, String inputValue)
      throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_TEST_DCI_TRANSFORMATION);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_SCRIPT, script);
      msg.setField(NXCPCodes.VID_VALUE, inputValue);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      TransformationTestResult r = new TransformationTestResult();
      r.success = response.getFieldAsBoolean(NXCPCodes.VID_EXECUTION_STATUS);
      r.result = response.getFieldAsString(NXCPCodes.VID_EXECUTION_RESULT);
      return r;
   }
   
   /**
    * Execute script.
    *
    * @param nodeId     ID of the node object to test script on
    * @param script     script source code
    * @return test execution results
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeScript(long nodeId, String script, final TextOutputListener listener) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_SCRIPT, script);

      MessageHandler handler = new MessageHandler() {
         @Override
         public boolean processMessage(NXCPMessage m)
         {
            if (m.getFieldAsInt32(NXCPCodes.VID_RCC) != RCC.SUCCESS)
            {
               String errorMessage = m.getFieldAsString(NXCPCodes.VID_ERROR_TEXT);
               if ((errorMessage != null) && (listener != null))
               {
                  listener.messageReceived(errorMessage + "\n\n");
               }
            }
            
            String text = m.getFieldAsString(NXCPCodes.VID_MESSAGE);
            if ((text != null) && (listener != null))
            {
               listener.messageReceived(text);
            }
            
            if (m.isEndOfSequence())
               setComplete();
            return true;
         }
      };
      addMessageSubscription(NXCPCodes.CMD_EXECUTE_SCRIPT_UPDATE, msg.getMessageId(), handler);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      synchronized(handler)
      {
         try
         {
            handler.wait();
         }
         catch(InterruptedException e)
         {
         }
      }
      if (handler.isTimeout())
         throw new NXCException(RCC.TIMEOUT);
   }

   /**
    * Compile NXSL script on server. Field *success* in compilation result object will indicate compilation status.
    * If compilation fails, field *errorMessage* will contain compilation error message.
    * 
    * @param source script source
    * @param serialize flag to indicate if compiled script should be serialized and sent back to client
    * @return script compilation result object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ScriptCompilationResult compileScript(String source, boolean serialize) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_COMPILE_SCRIPT);
      msg.setField(NXCPCodes.VID_SCRIPT, source);
      msg.setField(NXCPCodes.VID_SERIALIZE, serialize);
      sendMessage(msg);
      return new ScriptCompilationResult(waitForRCC(msg.getMessageId()));
   }

   /**
    * Open server log by name.
    *
    * @param logName Log name
    * @return Log object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Log openServerLog(final String logName) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_OPEN_SERVER_LOG);
      msg.setField(NXCPCodes.VID_LOG_NAME, logName);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      Log log = new Log(this, response, logName);
      return log;
   }

   /**
    * Synchronize event templates configuration. After call to this method
    * session object will maintain internal list of configured event templates.
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncEventTemplates() throws IOException, NXCException
   {
      List<EventTemplate> templates = getEventTemplates();
      synchronized(eventTemplates)
      {
         eventTemplates.clear();
         for(EventTemplate t : templates)
         {
            eventTemplates.put(t.getCode(), t);
         }
         eventTemplatesNeedSync = true;
      }
   }
   
   /**
    * Re-synchronize event templaytes in background
    */
   private void resyncEventTemplates()
   {
      new Thread(new Runnable() {
         @Override
         public void run()
         {
            try
            {
               syncEventTemplates();
            }
            catch(Exception e)
            {
               Logger.error("NXCSession.resyncEventTemplates", "Exception in worker thread", e);
            }
         }
      }).start();
   }

   /**
    * Get cached list event templates
    *
    * @return List of event templates cached by client library
    */
   public EventTemplate[] getCachedEventTemplates()
   {
      EventTemplate[] events = null;
      synchronized(eventTemplates)
      {
         events = eventTemplates.values().toArray(new EventTemplate[eventTemplates.size()]);
      }
      return events;
   }

   /**
    * Find event template by name in event template database internally
    * maintained by session object. You must call
    * NXCSession.syncEventTemplates() first to make local copy of event template
    * database.
    *
    * @param name Event name
    * @return Event template object or null if not found
    */
   public EventTemplate findEventTemplateByName(String name)
   {
      EventTemplate result = null;
      synchronized(eventTemplates)
      {
         for(EventTemplate e : eventTemplates.values())
         {
            if (e.getName().equalsIgnoreCase(name))
            {
               result = e;
               break;
            }
         }
      }
      return result;
   }
   
   /**
    * Get event name from event code
    * 
    * @param code event code
    * @return event name or event code as string if event not found
    */
   public String getEventName(long code)
   {
      synchronized(eventTemplates)
      {
         EventTemplate e = eventTemplates.get(code);
         return (e != null) ? e.getName() : ("[" + Long.toString(code) + "]");
      }
   }

   /**
    * Find event template by code in event template database internally
    * maintained by session object. You must call
    * NXCSession.syncEventTemplates() first to make local copy of event template
    * database.
    *
    * @param code Event code
    * @return Event template object or null if not found
    */
   public EventTemplate findEventTemplateByCode(long code)
   {
      synchronized(eventTemplates)
      {
         return eventTemplates.get(code);
      }
   }

   /**
    * Find multiple event templates by event codes in event template database
    * internally maintained by session object. You must call
    * NXCSession.syncEventTemplates() first to make local copy of event template
    * database.
    *
    * @param codes List of event codes
    * @return List of found event templates
    */
   public List<EventTemplate> findMultipleEventTemplates(final Long[] codes)
   {
      List<EventTemplate> list = new ArrayList<EventTemplate>();
      synchronized(eventTemplates)
      {
         for(long code : codes)
         {
            EventTemplate e = eventTemplates.get(code);
            if (e != null) list.add(e);
         }
      }
      return list;
   }

   /**
    * Find multiple event templates by event codes in event template database
    * internally maintained by session object. You must call
    * NXCSession.syncEventTemplates() first to make local copy of event template
    * database.
    *
    * @param codes List of event codes
    * @return List of found event templates
    */
   public List<EventTemplate> findMultipleEventTemplates(final long[] codes)
   {
      List<EventTemplate> list = new ArrayList<EventTemplate>();
      synchronized(eventTemplates)
      {
         for(long code : codes)
         {
            EventTemplate e = eventTemplates.get(code);
            if (e != null) list.add(e);
         }
      }
      return list;
   }

   /**
    * Get event templates from server
    *
    * @return List of configured event templates
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<EventTemplate> getEventTemplates() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_LOAD_EVENT_DB);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      ArrayList<EventTemplate> list = new ArrayList<EventTemplate>();
      while(true)
      {
         final NXCPMessage response = waitForMessage(NXCPCodes.CMD_EVENT_DB_RECORD, msg.getMessageId());
         if (response.isEndOfSequence()) 
            break;
         list.add(new EventTemplate(response));
      }
      return list;
   }

   /**
    * Generate code for new event template.
    *
    * @return Code for new event template
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long generateEventCode() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GENERATE_EVENT_CODE);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_EVENT_CODE);
   }

   /**
    * Delete event template.
    *
    * @param eventCode Event code
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteEventTemplate(long eventCode) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_EVENT_TEMPLATE);
      msg.setFieldInt32(NXCPCodes.VID_EVENT_CODE, (int) eventCode);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Modify event template.
    *
    * @param evt Event template
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyEventTemplate(EventTemplate evt) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_EVENT_INFO);
      msg.setFieldInt32(NXCPCodes.VID_EVENT_CODE, (int) evt.getCode());
      msg.setFieldInt32(NXCPCodes.VID_SEVERITY, evt.getSeverity().getValue());
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, evt.getFlags());
      msg.setField(NXCPCodes.VID_NAME, evt.getName());
      msg.setField(NXCPCodes.VID_MESSAGE, evt.getMessage());
      msg.setField(NXCPCodes.VID_DESCRIPTION, evt.getDescription());
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Send event to server. Event can be identified either by event code or event name. If event name
    * is given, event code will be ignored.
    * <p/>
    * Node: sending events by name supported by server version 1.1.8 and higher.
    *
    * @param eventCode  event code. Ignored if event name is not null.
    * @param eventName  event name. Must be set to null if event identified by code.
    * @param objectId   Object ID to send event on behalf of. If set to 0, server will determine object ID by client IP address.
    * @param parameters event's parameters
    * @param userTag    event's user tag
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void sendEvent(long eventCode, String eventName, long objectId, String[] parameters, String userTag)
      throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_TRAP);
      msg.setFieldInt32(NXCPCodes.VID_EVENT_CODE, (int) eventCode);
      if (eventName != null) msg.setField(NXCPCodes.VID_EVENT_NAME, eventName);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      msg.setField(NXCPCodes.VID_USER_TAG, (userTag != null) ? userTag : "");
      msg.setFieldInt16(NXCPCodes.VID_NUM_ARGS, parameters.length);
      long varId = NXCPCodes.VID_EVENT_ARG_BASE;
      for(int i = 0; i < parameters.length; i++)
      {
         msg.setField(varId++, parameters[i]);
      }
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Convenience wrapper for sendEvent interface.
    *
    * @param eventCode  event code
    * @param parameters event's parameters
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void sendEvent(long eventCode, String[] parameters) throws IOException, NXCException
   {
      sendEvent(eventCode, null, 0, parameters, null);
   }

   /**
    * Convenience wrapper for sendEvent interface.
    *
    * @param eventName  event name
    * @param parameters event's parameters
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void sendEvent(String eventName, String[] parameters) throws IOException, NXCException
   {
      sendEvent(0, eventName, 0, parameters, null);
   }

   /**
    * Get list of well-known SNMP communities configured on server.
    *
    * @return List of SNMP community strings
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<String> getSnmpCommunities() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_COMMUNITY_LIST);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_STRINGS);
      ArrayList<String> list = new ArrayList<String>(count);
      long varId = NXCPCodes.VID_STRING_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(response.getFieldAsString(varId++));
      }

      return list;
   }

   /**
    * Update list of well-known SNMP community strings on server. Existing list
    * will be replaced by given one.
    *
    * @param list New list of SNMP community strings
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateSnmpCommunities(final List<String> list) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_COMMUNITY_LIST);

      msg.setFieldInt32(NXCPCodes.VID_NUM_STRINGS, list.size());
      long varId = NXCPCodes.VID_STRING_LIST_BASE;
      for(int i = 0; i < list.size(); i++)
      {
         msg.setField(varId++, list.get(i));
      }

      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of well-known SNMP USM (user security model) credentials
    * configured on server.
    *
    * @return List of SNMP USM credentials
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<SnmpUsmCredential> getSnmpUsmCredentials() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_USM_CREDENTIALS);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
      ArrayList<SnmpUsmCredential> list = new ArrayList<SnmpUsmCredential>(count);
      long varId = NXCPCodes.VID_USM_CRED_LIST_BASE;
      for(int i = 0; i < count; i++, varId += 10)
      {
         list.add(new SnmpUsmCredential(response, varId));
      }

      return list;
   }

   /**
    * Update list of well-known SNMP USM credentials on server. Existing list
    * will be replaced by given one.
    *
    * @param list New list of SNMP USM credentials
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateSnmpUsmCredentials(final List<SnmpUsmCredential> list) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_USM_CREDENTIALS);

      msg.setFieldInt32(NXCPCodes.VID_NUM_RECORDS, list.size());
      long varId = NXCPCodes.VID_USM_CRED_LIST_BASE;
      for(int i = 0; i < list.size(); i++, varId += 10)
      {
         list.get(i).fillMessage(msg, varId);
      }

      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get agent's master configuration file.
    *
    * @param nodeId Node ID
    * @return Master configuration file of agent running on given node
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String getAgentConfig(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AGENT_CONFIG);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsString(NXCPCodes.VID_CONFIG_FILE);
   }

   /**
    * Update agent's master configuration file.
    *
    * @param nodeId Node ID
    * @param config New configuration file content
    * @param apply  Apply flag - if set to true, agent will restart automatically to
    *               apply changes
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateAgentConfig(long nodeId, String config, boolean apply) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_AGENT_CONFIG);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_CONFIG_FILE, config);
      msg.setFieldInt16(NXCPCodes.VID_APPLY_FLAG, apply ? 1 : 0);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of parameters supported by agent running on given node.
    *
    * @param nodeId Node ID
    * @return List of parameters supported by agent
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AgentParameter> getSupportedParameters(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PARAMETER_LIST);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setFieldInt16(NXCPCodes.VID_FLAGS, 0x01); // Indicates request for parameters list
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_PARAMETERS);
      List<AgentParameter> list = new ArrayList<AgentParameter>(count);
      long baseId = NXCPCodes.VID_PARAM_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new AgentParameter(response, baseId));
         baseId += 3;
      }
      return list;
   }

   /**
    * Get list of tables supported by agent running on given node.
    *
    * @param nodeId Node ID
    * @return List of tables supported by agent
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AgentTable> getSupportedTables(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PARAMETER_LIST);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setFieldInt16(NXCPCodes.VID_FLAGS, 0x02); // Indicates request for table list
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_TABLES);
      List<AgentTable> list = new ArrayList<AgentTable>(count);
      long baseId = NXCPCodes.VID_TABLE_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new AgentTable(response, baseId));
         baseId += response.getFieldAsInt64(baseId);
      }
      return list;
   }

   /**
    * Get all events used in data collection by given node, cluster, or template object.
    *
    * @param objectId node, cluster, or template object ID
    * @return list of used event codes
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long[] getDataCollectionEvents(long objectId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DCI_EVENTS_LIST);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      if (response.getFieldAsInt32(NXCPCodes.VID_NUM_EVENTS) == 0) return new long[0];
      return response.getFieldAsUInt32Array(NXCPCodes.VID_EVENT_LIST);
   }

   /**
    * Get names of all scripts used in data collection by given node, cluster, or template object.
    *
    * @param objectId node, cluster, or template object ID
    * @return list of used library scripts
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<Script> getDataCollectionScripts(long objectId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DCI_SCRIPT_LIST);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_SCRIPTS);
      List<Script> scripts = new ArrayList<Script>(count);
      long fieldId = NXCPCodes.VID_SCRIPT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         long id = response.getFieldAsInt64(fieldId++);
         String name = response.getFieldAsString(fieldId++);
         scripts.add(new Script(id, name, null));
      }
      return scripts;
   }

   /**
    * Export server configuration. Returns requested configuration elements
    * exported into XML.
    *
    * @param description Description of exported configuration
    * @param events      List of event codes
    * @param traps       List of trap identifiers
    * @param templates   List of template object identifiers
    * @param rules       List of event processing rule GUIDs
    * @param scripts     List of library script identifiers
    * @param objectTools List of object tool identifiers
    * @param dciSummaryTables List of DCI summary table identifiers
    * @return resulting XML document
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String exportConfiguration(String description, long[] events, long[] traps, long[] templates, UUID[] rules, 
         long[] scripts, long[] objectTools, long[] dciSummaryTables) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_EXPORT_CONFIGURATION);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setFieldInt32(NXCPCodes.VID_NUM_EVENTS, events.length);
      msg.setField(NXCPCodes.VID_EVENT_LIST, events);
      msg.setFieldInt32(NXCPCodes.VID_NUM_OBJECTS, templates.length);
      msg.setField(NXCPCodes.VID_OBJECT_LIST, templates);
      msg.setFieldInt32(NXCPCodes.VID_NUM_TRAPS, traps.length);
      msg.setField(NXCPCodes.VID_TRAP_LIST, traps);
      msg.setFieldInt32(NXCPCodes.VID_NUM_SCRIPTS, scripts.length);
      msg.setField(NXCPCodes.VID_SCRIPT_LIST, scripts);
      msg.setFieldInt32(NXCPCodes.VID_NUM_TOOLS, objectTools.length);
      msg.setField(NXCPCodes.VID_TOOL_LIST, objectTools);
      msg.setFieldInt32(NXCPCodes.VID_NUM_SUMMARY_TABLES, dciSummaryTables.length);
      msg.setField(NXCPCodes.VID_SUMMARY_TABLE_LIST, dciSummaryTables);

      msg.setFieldInt32(NXCPCodes.VID_NUM_RULES, rules.length);
      long varId = NXCPCodes.VID_RULE_LIST_BASE;
      for(int i = 0; i < rules.length; i++)
      {
         msg.setField(varId++, rules[i]);
      }

      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsString(NXCPCodes.VID_NXMP_CONTENT);
   }

   /**
    * Import server configuration (events, traps, thresholds) from XML
    *
    * @param config Configuration in XML format
    * @param flags  Import flags
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void importConfiguration(String config, int flags) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_IMPORT_CONFIGURATION);
      msg.setField(NXCPCodes.VID_NXMP_CONTENT, config);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get server stats. Returns set of named properties. The following
    * properties could be found in result set: String: VERSION Integer: UPTIME,
    * SESSION_COUNT, DCI_COUNT, OBJECT_COUNT, NODE_COUNT, PHYSICAL_MEMORY_USED,
    * VIRTUAL_MEMORY_USED, QSIZE_CONDITION_POLLER, QSIZE_CONF_POLLER,
    * QSIZE_DCI_POLLER, QSIZE_DBWRITER, QSIZE_EVENT, QSIZE_DISCOVERY,
    * QSIZE_NODE_POLLER, QSIZE_ROUTE_POLLER, QSIZE_STATUS_POLLER, 
    * QSIZE_DCI_CACHE_LOADER, ALARM_COUNT
    * long[]: ALARMS_BY_SEVERITY
    *
    * @return Server stats as set of named properties.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<String, Object> getServerStats() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SERVER_STATS);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      Map<String, Object> stats = new HashMap<String, Object>();
      stats.put("VERSION", response.getFieldAsString(NXCPCodes.VID_SERVER_VERSION));
      stats.put("UPTIME", response.getFieldAsInt32(NXCPCodes.VID_SERVER_UPTIME));
      stats.put("SESSION_COUNT", response.getFieldAsInt32(NXCPCodes.VID_NUM_SESSIONS));

      stats.put("DCI_COUNT", response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS));
      stats.put("OBJECT_COUNT", response.getFieldAsInt32(NXCPCodes.VID_NUM_OBJECTS));
      stats.put("NODE_COUNT", response.getFieldAsInt32(NXCPCodes.VID_NUM_NODES));

      stats.put("PHYSICAL_MEMORY_USED", response.getFieldAsInt32(NXCPCodes.VID_NETXMSD_PROCESS_WKSET));
      stats.put("VIRTUAL_MEMORY_USED", response.getFieldAsInt32(NXCPCodes.VID_NETXMSD_PROCESS_VMSIZE));

      stats.put("QSIZE_CONDITION_POLLER", response.getFieldAsInt32(NXCPCodes.VID_QSIZE_CONDITION_POLLER));
      stats.put("QSIZE_CONF_POLLER", response.getFieldAsInt32(NXCPCodes.VID_QSIZE_CONF_POLLER));
      stats.put("QSIZE_DCI_POLLER", response.getFieldAsInt32(NXCPCodes.VID_QSIZE_DCI_POLLER));
      stats.put("QSIZE_DCI_CACHE_LOADER", response.getFieldAsInt32(NXCPCodes.VID_QSIZE_DCI_CACHE_LOADER));
      stats.put("QSIZE_DBWRITER", response.getFieldAsInt32(NXCPCodes.VID_QSIZE_DBWRITER));
      stats.put("QSIZE_EVENT", response.getFieldAsInt32(NXCPCodes.VID_QSIZE_EVENT));
      stats.put("QSIZE_DISCOVERY", response.getFieldAsInt32(NXCPCodes.VID_QSIZE_DISCOVERY));
      stats.put("QSIZE_NODE_POLLER", response.getFieldAsInt32(NXCPCodes.VID_QSIZE_NODE_POLLER));
      stats.put("QSIZE_ROUTE_POLLER", response.getFieldAsInt32(NXCPCodes.VID_QSIZE_ROUTE_POLLER));
      stats.put("QSIZE_STATUS_POLLER", response.getFieldAsInt32(NXCPCodes.VID_QSIZE_STATUS_POLLER));

      stats.put("ALARM_COUNT", response.getFieldAsInt32(NXCPCodes.VID_NUM_ALARMS));
      stats.put("ALARMS_BY_SEVERITY", response.getFieldAsUInt32Array(NXCPCodes.VID_ALARMS_BY_SEVERITY));

      return stats;
   }

   /**
    * Get list of configured actions from server
    *
    * @return List of configured actions
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<ServerAction> getActions() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LOAD_ACTIONS);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      List<ServerAction> actions = new ArrayList<ServerAction>();
      while(true)
      {
         final NXCPMessage response = waitForMessage(NXCPCodes.CMD_ACTION_DATA, msg.getMessageId());
         if (response.getFieldAsInt64(NXCPCodes.VID_ACTION_ID) == 0) break; // End of list
         actions.add(new ServerAction(response));
      }

      return actions;
   }

   /**
    * Create new server action.
    *
    * @param name action name
    * @return ID assigned to new action
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long createAction(final String name) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_ACTION);
      msg.setField(NXCPCodes.VID_ACTION_NAME, name);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_ACTION_ID);
   }

   /**
    * Modify server action
    *
    * @param action Action object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyAction(ServerAction action) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_ACTION);
      action.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete server action
    *
    * @param actionId Action ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAction(long actionId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_ACTION);
      msg.setFieldInt32(NXCPCodes.VID_ACTION_ID, (int) actionId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Get list of certificates
    *
    * @return List of certificates
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AuthCertificate> getCertificateList() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_CERT_LIST);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_CERTIFICATES);
      final List<AuthCertificate> list = new ArrayList<AuthCertificate>(count);

      long varId = NXCPCodes.VID_CERT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new AuthCertificate(response, varId));
         varId += 10;
      }

      return list;
   }
   
   /**
    * Create new certificate
    *
    * @param data certificate file content
    * @param comments comment for certificate
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void createNewCertificate(byte[] data, String comments) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_ADD_CA_CERTIFICATE);
      msg.setField(NXCPCodes.VID_CERTIFICATE, data);
      msg.setField(NXCPCodes.VID_COMMENTS, comments);
      sendMessage(msg);

      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Delete certificate
    *
    * @param id the certificate id
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteCertificate(long id) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_CERTIFICATE);
      msg.setFieldInt32(NXCPCodes.VID_CERTIFICATE_ID, (int)id);
      sendMessage(msg);

      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Update certificate
    *
    * @param id the certificate id
    * @param comment the certificate comment
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateCertificate(long id, String comment) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_CERT_COMMENTS);
      msg.setFieldInt32(NXCPCodes.VID_CERTIFICATE_ID, (int)id);
      msg.setField(NXCPCodes.VID_COMMENTS, comment);
      sendMessage(msg);

      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of configured object tools
    *
    * @return List of object tools
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<ObjectTool> getObjectTools() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_OBJECT_TOOLS);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_TOOLS);
      final List<ObjectTool> list = new ArrayList<ObjectTool>(count);

      long varId = NXCPCodes.VID_OBJECT_TOOLS_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new ObjectTool(response, varId));
         varId += 10000;
      }

      return list;
   }

   /**
    * Get object tool details
    *
    * @param toolId Tool ID
    * @return Object tool details
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ObjectToolDetails getObjectToolDetails(long toolId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_OBJECT_TOOL_DETAILS);
      msg.setFieldInt32(NXCPCodes.VID_TOOL_ID, (int) toolId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new ObjectToolDetails(response);
   }

   /**
    * Generate unique ID for new object tool.
    *
    * @return Unique ID for object tool
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long generateObjectToolId() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GENERATE_OBJECT_TOOL_ID);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_TOOL_ID);
   }

   /**
    * Modify object tool.
    *
    * @param tool Object tool
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyObjectTool(ObjectToolDetails tool) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_OBJECT_TOOL);
      tool.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete object tool.
    *
    * @param toolId Object tool ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteObjectTool(long toolId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_OBJECT_TOOL);
      msg.setFieldInt32(NXCPCodes.VID_TOOL_ID, (int) toolId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Delete object tool.
    *
    * @param toolId Object tool ID
    * @param enable true if object tool should be enabled, false if disabled
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void changeObjecToolDisableStatuss(long toolId, boolean enable) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_OBJECT_TOOL_STATUS);
      msg.setFieldInt32(NXCPCodes.VID_TOOL_ID, (int) toolId);
      msg.setFieldInt32(NXCPCodes.VID_STATE, enable ? 1 : 0);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Execute object tool of "table" type against given node.
    *
    * @param toolId Tool ID
    * @param nodeId Node object ID
    * @return Table with tool execution results
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table executeTableTool(long toolId, long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_EXEC_TABLE_TOOL);
      msg.setFieldInt32(NXCPCodes.VID_TOOL_ID, (int) toolId);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);

      waitForRCC(msg.getMessageId());
      final NXCPMessage response = waitForMessage(NXCPCodes.CMD_TABLE_DATA, msg.getMessageId(), 300000); // wait up to 5 minutes
      return new Table(response);
   }

   /**
    * Execute server command related to given object (usually defined as object tool)
    *
    * @param objectId object ID
    * @param command  command
    * @param inputFields values for input fields (can be null)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeServerCommand(long objectId, String command, Map<String, String> inputFields) throws IOException, NXCException
   {
      executeServerCommand(objectId, command, inputFields, false, null, null);
   }
   
   /**
    * Execute server command related to given object (usually defined as object tool)
    *
    * @param objectId object ID
    * @param command  command
    * @param inputFields values for input fields (can be null)
    * @param receiveOutput true if command's output has to be read
    * @param listener listener for command's output or null
    * @param writer writer for command's output or null
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeServerCommand(long objectId, String command, Map<String, String> inputFields, boolean receiveOutput, final TextOutputListener listener, final Writer writer) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_SERVER_COMMAND);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      msg.setField(NXCPCodes.VID_COMMAND, command);
      msg.setField(NXCPCodes.VID_RECEIVE_OUTPUT, receiveOutput);
      if (inputFields != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_NUM_FIELDS, inputFields.size());
         long fieldId = NXCPCodes.VID_FIELD_LIST_BASE;
         for(Entry<String, String> e : inputFields.entrySet())
         {
            msg.setField(fieldId++, e.getKey());
            msg.setField(fieldId++, e.getValue());
         }
      }
      
      MessageHandler handler = receiveOutput ? new MessageHandler() {
         @Override
         public boolean processMessage(NXCPMessage m)
         {
            String text = m.getFieldAsString(NXCPCodes.VID_MESSAGE);
            if (text != null)
            {
               if (listener != null)
                  listener.messageReceived(text);
               if (writer != null)
               {
                  try
                  {
                     writer.write(text);
                  }
                  catch(IOException e)
                  {
                  }
               }
            }
            if (m.isEndOfSequence())
               setComplete();
            return true;
         }
      } : null;
      if (receiveOutput)
         addMessageSubscription(NXCPCodes.CMD_COMMAND_OUTPUT, msg.getMessageId(), handler);
      
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      if (receiveOutput)
      {
         synchronized(handler)
         {
            try
            {
               handler.wait();
            }
            catch(InterruptedException e)
            {
            }
         }
         if (handler.isTimeout())
            throw new NXCException(RCC.TIMEOUT);
      }      
   }

   /**
    * Get summary of SNMP trap mapping. Trap configurations returned without parameter mapping.
    *
    * @return
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<SnmpTrap> getSnmpTrapsConfigurationSummary() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_TRAP_CFG_RO);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_TRAPS);
      List<SnmpTrap> list = new ArrayList<SnmpTrap>(count);

      long varId = NXCPCodes.VID_TRAP_INFO_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new SnmpTrap(response, varId));
         varId += 10;
      }
      return list;
   }

   /**
    * Get list of configured SNMP traps
    *
    * @return List of configured SNMP traps.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<SnmpTrap> getSnmpTrapsConfiguration() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LOAD_TRAP_CFG);
      sendMessage(msg);

      waitForRCC(msg.getMessageId());
      List<SnmpTrap> traps = new ArrayList<SnmpTrap>();
      while(true)
      {
         final NXCPMessage response = waitForMessage(NXCPCodes.CMD_TRAP_CFG_RECORD, msg.getMessageId());
         if (response.getFieldAsInt64(NXCPCodes.VID_TRAP_ID) == 0) break; // end of list
         traps.add(new SnmpTrap(response));
      }
      return traps;
   }

   /**
    * Create new trap configuration record.
    *
    * @return ID of new record
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long createSnmpTrapConfiguration() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_TRAP);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_TRAP_ID);
   }

   /**
    * Delete SNMP trap configuration record from server.
    *
    * @param trapId Trap configuration record ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteSnmpTrapConfiguration(long trapId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_TRAP);
      msg.setFieldInt32(NXCPCodes.VID_TRAP_ID, (int) trapId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Modify SNMP trap configuration record.
    *
    * @param trap Modified trap configuration record
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifySnmpTrapConfiguration(SnmpTrap trap) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_TRAP);
      trap.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get timestamp of server's MIB file.
    *
    * @return Timestamp of server's MIB file
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Date getMibFileTimestamp() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_MIB_TIMESTAMP);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsDate(NXCPCodes.VID_TIMESTAMP);
   }

   /**
    * Download MIB file from server.
    *
    * @return file handle for temporary file on local file system
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public File downloadMibFile() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_MIB);
      sendMessage(msg);

      int ttw = 120;
      NXCReceivedFile file;
      do
      {
         synchronized(receivedFiles)
         {
            file = receivedFiles.get(msg.getMessageId());
            if ((file != null) && (file.getStatus() != NXCReceivedFile.OPEN))
            {
               receivedFiles.remove(file.getId());
               break;
            }

            try
            {
               receivedFiles.wait(10000);
            }
            catch(InterruptedException e)
            {
            }
         }
         ttw--;
      }
      while(ttw > 0);

      if (ttw == 0) throw new NXCException(RCC.TIMEOUT);

      if (file.getStatus() == NXCReceivedFile.FAILED) throw file.getException();

      return file.getFile();
   }

   /**
    * Get list of predefined graphs.
    *
    * @return list of predefined graphs
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<GraphSettings> getPredefinedGraphs() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_GRAPH_LIST);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_GRAPHS);
      List<GraphSettings> list = new ArrayList<GraphSettings>(count);
      long varId = NXCPCodes.VID_GRAPH_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new GraphSettings(response, varId));
         varId += 10;
      }
      return list;
   }
   
   /**
    * Checks if graph with specified name can be created/overwritten and creates/overwrites it in DB. 
    * If graph id is set to 0 it checks if graph with the same name exists, and if yes checks overwrite parameter. If it is
    * set to false, then function returns error that graph with this name already exists. 
    * If there is no graph with the same name it just creates a new one.
    * If id is set it checks that provided name is assigned only to this graph and overwrites it or throws error is the 
    * same name was already used. 
    * Also check if user have permissions to overwrite graph. 
    *   
    * If it can, then it returns 1.
    * If graph with this name already exists, but can be overwritten by current user function returns 2.
    * If graph with this name already exists, but can not be overwritten by current user function returns 0.
    *
    * @param graph predefined graph configuration
    * @param overwrite defines if existing graph should be overwritten
    * @return ID of predefined graph object
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long saveGraph(GraphSettings graph, boolean overwrite) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SAVE_GRAPH);
      msg.setFieldInt32(NXCPCodes.VID_GRAPH_ID, (int) graph.getId());
      msg.setField(NXCPCodes.VID_NAME, graph.getName());
      msg.setField(NXCPCodes.VID_GRAPH_CONFIG, graph.getConfig());
      msg.setFieldInt32(NXCPCodes.VID_ACL_SIZE, graph.getAccessList().size());
      long varId = NXCPCodes.VID_GRAPH_ACL_BASE;
      for(AccessListElement e : graph.getAccessList())
      {
         msg.setFieldInt32(varId++, (int) e.getUserId());
         msg.setFieldInt32(varId++, e.getAccessRights());
      }      
      msg.setFieldInt16(NXCPCodes.VID_FLAGS, overwrite ? 1 : 0);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_GRAPH_ID);
   }

   /**
    * Delete predefined graph.
    *
    * @param graphId predefined graph object ID
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deletePredefinedGraph(long graphId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_GRAPH);
      msg.setFieldInt32(NXCPCodes.VID_GRAPH_ID, (int) graphId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of all scripts in script library.
    * 
    * @return ID/name pairs for scripts in script library
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<Script> getScriptLibrary() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SCRIPT_LIST);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_SCRIPTS);
      List<Script> scripts = new ArrayList<Script>(count);
      long varId = NXCPCodes.VID_SCRIPT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         final long id = response.getFieldAsInt64(varId++);
         final String name = response.getFieldAsString(varId++);
         scripts.add(new Script(id, name, null));
      }
      return scripts;
   }

   /**
    * Get script from library
    * 
    * @param scriptId script ID
    * @return script source code
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Script getScript(long scriptId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_SCRIPT_ID, (int) scriptId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new Script(scriptId, response.getFieldAsString(NXCPCodes.VID_NAME),
         response.getFieldAsString(NXCPCodes.VID_SCRIPT_CODE));
   }

   /**
    * Modify script. If scriptId is 0, new script will be created in library.
    * 
    * @param scriptId script ID
    * @param name script name
    * @param source script source code
    * @return script ID (newly assigned if new script was created)
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long modifyScript(long scriptId, String name, String source) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_SCRIPT_ID, (int) scriptId);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_SCRIPT_CODE, source);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_SCRIPT_ID);
   }

   /**
    * Rename script in script library.
    * 
    * @param scriptId script ID
    * @param name new script name
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void renameScript(long scriptId, String name) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RENAME_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_SCRIPT_ID, (int) scriptId);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete script from library
    * 
    * @param scriptId script ID
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteScript(long scriptId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_SCRIPT_ID, (int) scriptId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Find connection point (either directly connected or most close known
    * interface on a switch) for given node or interface object. Will return
    * null if connection point information cannot be found.
    *
    * @param objectId Node or interface object ID
    * @return connection point information or null
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ConnectionPoint findConnectionPoint(long objectId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FIND_NODE_CONNECTION);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      if (response.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID) != 0) return new ConnectionPoint(response);
      return null;
   }

   /**
    * Find connection point (either directly connected or most close known
    * interface on a switch) for given MAC address. Will return null if
    * connection point information cannot be found.
    *
    * @param macAddr MAC address
    * @return connection point information or null
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ConnectionPoint findConnectionPoint(MacAddress macAddr) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FIND_MAC_LOCATION);
      msg.setField(NXCPCodes.VID_MAC_ADDR, macAddr.getValue());
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      if (response.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID) != 0) return new ConnectionPoint(response);
      return null;
   }

   /**
    * Find connection point (either directly connected or most close known
    * interface on a switch) for given IP address. Will return null if
    * connection point information cannot be found.
    *
    * @param zoneId zone ID
    * @param ipAddr IP address to find
    * @return connection point information or null
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ConnectionPoint findConnectionPoint(int zoneId, InetAddress ipAddr) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FIND_IP_LOCATION);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_ID, zoneId);
      msg.setField(NXCPCodes.VID_IP_ADDRESS, ipAddr);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      if (response.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID) != 0) return new ConnectionPoint(response);
      return null;
   }

   /**
    * Send KEEPALIVE message. Return true is connection is fine and false otherwise.
    * If connection is broken, session notification with code CONNECTION_BROKEN
    * will be sent to all subscribers. Note that this function will not throw exception
    * in case of error. 
    * 
    * @return true if connection is fine
    */
   public boolean checkConnection()
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_KEEPALIVE);
      try
      {
         sendMessage(msg);
         waitForRCC(msg.getMessageId());
         return true;
      }
      catch(Exception e)
      {
         sendNotification(new SessionNotification(SessionNotification.CONNECTION_BROKEN));
         return false;
      }
   }

   /**
    * @return
    * @throws IOException
    * @throws NXCException
    */
   public List<LibraryImage> getImageLibrary() throws IOException, NXCException
   {
      return getImageLibrary(null);
   }

   /**
    * @param category
    * @return
    * @throws IOException
    * @throws NXCException
    */
   public List<LibraryImage> getImageLibrary(String category) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LIST_IMAGES);
      if (category != null)
      {
         msg.setField(NXCPCodes.VID_CATEGORY, category);
      }
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      final int numOfImages = response.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);

      final List<LibraryImage> ret = new ArrayList<LibraryImage>(numOfImages);
      long varId = NXCPCodes.VID_IMAGE_LIST_BASE;
      for(int i = 0; i < numOfImages; i++)
      {
         final UUID imageGuid = response.getFieldAsUUID(varId++);
         final String imageName = response.getFieldAsString(varId++);
         final String imageCategory = response.getFieldAsString(varId++);
         final String imageMimeType = response.getFieldAsString(varId++);
         final boolean imageProtected = response.getFieldAsBoolean(varId++);
         ret.add(new LibraryImage(imageGuid, imageName, imageCategory, imageMimeType, imageProtected));
      }

      return ret;
   }

   /**
    * @param guid
    * @return
    * @throws IOException
    * @throws NXCException
    */
   public LibraryImage getImage(UUID guid) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_IMAGE);
      msg.setField(NXCPCodes.VID_GUID, guid);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      final File imageFile = waitForFile(msg.getMessageId(), 600000);
      if (imageFile == null) throw new NXCException(RCC.IO_ERROR);
      return new LibraryImage(response, imageFile);
   }

   /**
    * @param image
    * @param listener
    * @return
    * @throws IOException
    * @throws NXCException
    */
   public LibraryImage createImage(LibraryImage image, ProgressListener listener) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_IMAGE);
      image.fillMessage(msg);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      final UUID imageGuid = response.getFieldAsUUID(NXCPCodes.VID_GUID);
      image.setGuid(imageGuid);

      sendFile(msg.getMessageId(), image.getBinaryData(), listener);

      waitForRCC(msg.getMessageId());

      return image;
   }

   /**
    * @param image
    * @throws IOException
    * @throws NXCException
    */
   public void deleteImage(LibraryImage image) throws IOException, NXCException
   {
      if (image.isProtected())
      {
         throw new NXCException(RCC.INVALID_REQUEST);
      }

      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_IMAGE);
      image.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * @param image
    * @param listener
    * @throws IOException
    * @throws NXCException
    */
   public void modifyImage(LibraryImage image, ProgressListener listener) throws IOException, NXCException
   {
      if (image.isProtected())
      {
         throw new NXCException(RCC.INVALID_REQUEST);
      }

      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_IMAGE);
      image.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      sendFile(msg.getMessageId(), image.getBinaryData(), listener);

      waitForRCC(msg.getMessageId());
   }

   /**
    * Perform a forced node poll. This method will not return until poll is
    * complete, so it's advised to run it from separate thread. For each message
    * received from poller listener's method onPollerMessage will be called.
    *
    * @param nodeId   node object ID
    * @param pollType poll type
    * @param listener listener
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void pollNode(long nodeId, NodePollType pollType, final TextOutputListener listener) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_POLL_NODE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setFieldInt16(NXCPCodes.VID_POLL_TYPE, pollType.getValue());

      MessageHandler handler = new MessageHandler() {
         @Override
         public boolean processMessage(NXCPMessage m)
         {
            int rcc = m.getFieldAsInt32(NXCPCodes.VID_RCC);
            if (rcc == RCC.OPERATION_IN_PROGRESS)
            {
               if (listener != null)
                  listener.messageReceived(m.getFieldAsString(NXCPCodes.VID_POLLER_MESSAGE));
            }
            else
            {
               setComplete();
            }
            return true;
         }
      };
      handler.setMessageWaitTimeout(600000); // 10 min timeout
      addMessageSubscription(NXCPCodes.CMD_POLLING_INFO, msg.getMessageId(), handler);
      
      sendMessage(msg);
      synchronized(handler)
      {
         try
         {
            handler.wait();
         }
         catch(InterruptedException e)
         {
         }
      }
      if (handler.isTimeout())
         throw new NXCException(RCC.TIMEOUT);
   }

   /**
    * Get list of all configured situations
    *
    * @return list of situation objects
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<Situation> getSituations() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SITUATION_LIST);
      sendMessage(msg);

      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_SITUATION_COUNT);
      List<Situation> list = new ArrayList<Situation>(count);
      for(int i = 0; i < count; i++)
      {
         response = waitForMessage(NXCPCodes.CMD_SITUATION_DATA, msg.getMessageId());
         list.add(new Situation(response));
      }

      return list;
   }

   /**
    * Create new situation object.
    *
    * @param name     name for new situation object
    * @param comments comments for new situation object
    * @return id assigned to created situation object
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long createSituation(String name, String comments) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_SITUATION);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_COMMENTS, comments);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_SITUATION_ID);
   }

   /**
    * Modify situation object.
    *
    * @param id       situation id
    * @param name     new name or null to leave unchanged
    * @param comments new comments or null to leave unchanged
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifySituation(long id, String name, String comments) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_SITUATION);
      msg.setFieldInt32(NXCPCodes.VID_SITUATION_ID, (int) id);
      if (name != null) msg.setField(NXCPCodes.VID_NAME, name);
      if (comments != null) msg.setField(NXCPCodes.VID_COMMENTS, comments);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete situation object
    *
    * @param id situation id
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteSituation(long id) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_SITUATION);
      msg.setFieldInt32(NXCPCodes.VID_SITUATION_ID, (int) id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete situation instance
    *
    * @param id       situation id
    * @param instance situation instance
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteSituationInstance(long id, String instance) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DEL_SITUATION_INSTANCE);
      msg.setFieldInt32(NXCPCodes.VID_SITUATION_ID, (int) id);
      msg.setField(NXCPCodes.VID_SITUATION_INSTANCE, instance);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * List files in server's file store.
    *
    * @return list of files in server's file store
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ServerFile[] listServerFiles() throws IOException, NXCException
   {
      return listServerFiles(null);
   }
   
   /**
    * List files in server's file store.
    * 
    * @param filter array with required extension. Will be used as file filter. Give empty array or null if no filter should be
    *           applyed.
    * @return list of files in server's file store
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ServerFile[] listServerFiles(String[] filter) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LIST_SERVER_FILES);
      if(filter != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_EXTENSION_COUNT, filter.length);
         int i = 0;
         long j = NXCPCodes.VID_EXTENSION_LIST_BASE;
         for(; i < filter.length; i++, j++)
         {
            msg.setField(j, filter[i]);
         }
      }
      else
      {
         msg.setFieldInt32(NXCPCodes.VID_EXTENSION_COUNT, 0);
      }
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_INSTANCE_COUNT);
      ServerFile[] files = new ServerFile[count];
      long varId = NXCPCodes.VID_INSTANCE_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         files[i] = new ServerFile(response, varId);
         varId += 10;
      }
      return files;
   }
   
   /**
    * List files on agent file store.
    * 
    * @param file parent of new coomming list
    * @param fullPath path that will be used on an agent to get list of subfiles
    * @param objectId the ID of the node 
    * @return will return the list of sub files or the list of allowed folders if full path is set to "/"
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public AgentFile[] listAgentFiles(AgentFile file, String fullPath, long objectId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_FOLDER_CONTENT);
      msg.setField(NXCPCodes.VID_FILE_NAME, fullPath);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      msg.setFieldInt16(NXCPCodes.VID_ROOT, file == null ? 1 : 0);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_INSTANCE_COUNT);
      AgentFile[] files = new AgentFile[count];
      long varId = NXCPCodes.VID_INSTANCE_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         files[i] = new AgentFile(response, varId, file, objectId);
         varId += 10;
      }
      return files;
   }

   /**
    * Start file upload from server's file store to agent. Returns ID of upload
    * job.
    *
    * @param nodeId         node object ID
    * @param serverFileName file name in server's file store
    * @param remoteFileName fully qualified file name on target system or null to upload
    *                       file to agent's file store
    * @param jobOnHold      if true, upload job will be created in "hold" status
    * @return ID of upload job
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long uploadFileToAgent(long nodeId, String serverFileName, String remoteFileName, boolean jobOnHold)
      throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPLOAD_FILE_TO_AGENT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, serverFileName);
      if (remoteFileName != null)
         msg.setField(NXCPCodes.VID_DESTINATION_FILE_NAME, remoteFileName);
      msg.setFieldInt16(NXCPCodes.VID_CREATE_JOB_ON_HOLD, jobOnHold ? 1 : 0);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_JOB_ID);
   }

   /**
    * Upload local file to server's file store
    *
    * @param localFile      local file
    * @param serverFileName name under which file will be stored on server
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void uploadFileToServer(File localFile, String serverFileName, ProgressListener listener)
      throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPLOAD_FILE);
      if(serverFileName.equals(""))
      {
         serverFileName = localFile.getName();
      }
      msg.setField(NXCPCodes.VID_FILE_NAME, serverFileName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      sendFile(msg.getMessageId(), localFile, listener);
   }
   
   /**
    * Upload local file to server's file store
    *
    * @param localFile      local file
    * @param serverFileName name under which file will be stored on server
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void uploadLocalFileToAgent(File localFile, String agentFileName, long nodeId, ProgressListener listener)
      throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_UPLOAD);
      if(agentFileName.equals(""))
      {
         agentFileName = localFile.getName();
      }
      msg.setField(NXCPCodes.VID_FILE_NAME, agentFileName);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      sendFile(msg.getMessageId(), localFile, listener);
   }
   
   /**
    * Upload local file to server's file store
    *
    * @param localFile      local file
    * @param serverFileName name under which file will be stored on server
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void createFolderOnAgent(String folder, long nodeId)
      throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_CREATE_FOLDER);
      msg.setField(NXCPCodes.VID_FILE_NAME, folder);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Download file from remote host via agent.
    *
    * @param nodeId node object ID
    * @param remoteFileName fully qualified file name on remote system
    * @param maxFileSize maximum download size, 0 == UNLIMITED
    * @param follow if set to true, server will send file updates as they appear (like for tail -f command) 
    * @return agent file handle which contains server assigned ID and handle for local file
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public AgentFileData downloadFileFromAgent(long nodeId, String remoteFileName, long maxFileSize, boolean follow) throws IOException,
         NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AGENT_FILE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, remoteFileName);
      msg.setFieldInt32(NXCPCodes.VID_FILE_SIZE_LIMIT, (int)maxFileSize);
      msg.setFieldInt16(NXCPCodes.VID_FILE_FOLLOW, follow ? 1 : 0);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId()); // first confirmation - server job started
      final String id = response.getFieldAsString(NXCPCodes.VID_NAME);
      
      AgentFileData file =  new AgentFileData(id, waitForFile(msg.getMessageId(), 36000000));
      waitForRCC(msg.getMessageId()); // second confirmation - file transfered from agent to console
      return file;
   }
   
   /**
    * Download file from server file storage.
    *
    * @param remoteFileName fully qualified file name on remote system
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public File downloadFileFromServer( String remoteFileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SERVER_FILE);
      msg.setField(NXCPCodes.VID_FILE_NAME, remoteFileName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      return waitForFile(msg.getMessageId(), 3600000);
   }
   
   /**
    * Cancel file monitoring
    *
    * @param nodeId         node object ID
    * @param remoteFileName fully qualified file name on remote system
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void cancelFileMonitoring(long nodeId, String remoteFileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CANCEL_FILE_MONITORING);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, remoteFileName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete file from server's file store
    *
    * @param serverFileName name of server file
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteServerFile(String serverFileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_FILE);
      msg.setField(NXCPCodes.VID_FILE_NAME, serverFileName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Delete file from agent
    *
    * @param serverFileName name of server file
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAgentFile(long nodeId, String fileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_DELETE_FILE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, fileName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Rename file from agent
    *
    * @param serverFileName name of server file
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void renameAgentFile(long nodeId, String oldName, String newFileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_RENAME_FILE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, oldName);
      msg.setField(NXCPCodes.VID_NEW_FILE_NAME, newFileName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Move file from agent
    *
    * @param serverFileName name of server file
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void moveAgentFile(long nodeId, String oldName, String newFileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_MOVE_FILE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, oldName);
      msg.setField(NXCPCodes.VID_NEW_FILE_NAME, newFileName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Open server console.
    *
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void openConsole() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_OPEN_CONSOLE);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      serverConsoleConnected = true;
   }

   /**
    * Close server console.
    *
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void closeConsole() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CLOSE_CONSOLE);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      serverConsoleConnected = false;
   }

   /**
    * Process console command on server. Output of the command delivered via
    * console listener.
    *
    * @param command command to process
    * @return true if console should be closed (usually after "exit" command)
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public boolean processConsoleCommand(String command) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_ADM_REQUEST);
      msg.setField(NXCPCodes.VID_COMMAND, command);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId(), 3600000);
      return response.isEndOfSequence();
   }

   /**
    * @return the serverConsoleConnected
    */
   public boolean isServerConsoleConnected()
   {
      return serverConsoleConnected;
   }

   /**
    * Do SNMP walk. Operation will start at given root object, and callback will
    * be called one or more times as data will come from server. This method
    * will exit only when walk operation is complete.
    *
    * @param nodeId   node object ID
    * @param rootOid  root SNMP object ID (as text)
    * @param listener listener
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void snmpWalk(long nodeId, String rootOid, SnmpWalkListener listener) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_START_SNMP_WALK);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      msg.setField(NXCPCodes.VID_SNMP_OID, rootOid);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      while(true)
      {
         final NXCPMessage response = waitForMessage(NXCPCodes.CMD_SNMP_WALK_DATA, msg.getMessageId());
         final int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_VARIABLES);
         final List<SnmpValue> data = new ArrayList<SnmpValue>(count);
         long varId = NXCPCodes.VID_SNMP_WALKER_DATA_BASE;
         for(int i = 0; i < count; i++)
         {
            final String name = response.getFieldAsString(varId++);
            final int type = response.getFieldAsInt32(varId++);
            final String value = response.getFieldAsString(varId++);
            data.add(new SnmpValue(name, type, value, nodeId));
         }
         listener.onSnmpWalkData(data);
         if (response.isEndOfSequence()) break;
      }
   }

   /**
    * Get list of VLANs configured on given node
    *
    * @param nodeId node object ID
    * @return list of VLANs
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<VlanInfo> getVlans(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_VLANS);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_VLANS);
      List<VlanInfo> vlans = new ArrayList<VlanInfo>(count);
      long varId = NXCPCodes.VID_VLAN_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         vlans.add(new VlanInfo(response, varId));
         varId += 10;
      }
      return vlans;
   }

   /**
    * @return the objectsSynchronized
    */
   public boolean isObjectsSynchronized()
   {
      return objectsSynchronized;
   }

   /**
    * Get address list.
    *
    * @param list list identifier (defined in NXCSession as ADDRESS_LIST_xxx)
    * @return address list
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<IpAddressListElement> getAddressList(int listId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ADDR_LIST);
      msg.setFieldInt32(NXCPCodes.VID_ADDR_LIST_TYPE, listId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
      final List<IpAddressListElement> list = new ArrayList<IpAddressListElement>(count);
      long varId = NXCPCodes.VID_ADDR_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new IpAddressListElement(response, varId));
         varId += 10;
      }
      return list;
   }

   /**
    * Set content of address list.
    *
    * @param listId list ID
    * @param list   new list content
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setAddressList(int listId, List<IpAddressListElement> list) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_ADDR_LIST);
      msg.setFieldInt32(NXCPCodes.VID_ADDR_LIST_TYPE, listId);
      msg.setFieldInt32(NXCPCodes.VID_NUM_RECORDS, list.size());
      long varId = NXCPCodes.VID_ADDR_LIST_BASE;
      for(IpAddressListElement e : list)
      {
         msg.setFieldInt32(varId++, e.getType());
         msg.setField(varId++, e.getAddr1());
         msg.setField(varId++, e.getAddr2());
         varId += 7;
      }
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Reset server's internal component (defined by SERVER_COMPONENT_xxx)
    *
    * @param component component id
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void resetServerComponent(int component) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RESET_COMPONENT);
      msg.setFieldInt32(NXCPCodes.VID_COMPONENT_ID, component);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get IPv4 network path between two nodes. Server will return path based
    * on cached routing table information. Network path object may be incomplete
    * if server does not have enough information to build full path. In this case,
    * no exception thrown, and completness of path can be checked by calling
    * NetworkPath.isComplete().
    *
    * @param node1 source node
    * @param node2 destination node
    * @return network path object
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public NetworkPath getNetworkPath(long node1, long node2) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_NETWORK_PATH);
      msg.setFieldInt32(NXCPCodes.VID_SOURCE_OBJECT_ID, (int) node1);
      msg.setFieldInt32(NXCPCodes.VID_DESTINATION_OBJECT_ID, (int) node2);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new NetworkPath(response);
   }
   
   /**
    * Get routing table from node
    * 
    * @param nodeId node object ID
    * @return list of routing table entries
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<Route> getRoutingTable(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ROUTING_TABLE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<Route> rt = new ArrayList<Route>(count);
      long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         rt.add(new Route(response, varId));
         varId += 10;
      }
      return rt;
   }

   /**
    * Get switch forwarding database (MAC address table) from node
    * 
    * @param nodeId node object ID
    * @return list of switch forwarding database entries
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<FdbEntry> getSwitchForwardingDatabase(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SWITCH_FDB);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<FdbEntry> fdb = new ArrayList<FdbEntry>(count);
      long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         fdb.add(new FdbEntry(response, varId));
         varId += 10;
      }
      return fdb;
   }

   /**
    * Get list of wireless stations registered at given wireless controller.
    *
    * @param nodeId controller node ID
    * @return list of wireless stations
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<WirelessStation> getWirelessStations(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_WIRELESS_STATIONS);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<WirelessStation> stations = new ArrayList<WirelessStation>(count);
      long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         stations.add(new WirelessStation(response, varId));
         varId += 10;
      }
      return stations;
   }

   /**
    * Remove agent package from server
    *
    * @param packageId
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void removePackage(long packageId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_REMOVE_PACKAGE);
      msg.setFieldInt32(NXCPCodes.VID_PACKAGE_ID, (int) packageId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Install (upload) package on server
    *
    * @param info     package information
    * @param pkgFile  package file
    * @param listener progress listener (may be null)
    * @return unique ID assigned to package
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long installPackage(PackageInfo info, File pkgFile, ProgressListener listener)
      throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_INSTALL_PACKAGE);
      info.fillMessage(msg);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      final long id = response.getFieldAsInt64(NXCPCodes.VID_PACKAGE_ID);
      sendFile(msg.getMessageId(), pkgFile, listener);
      waitForRCC(msg.getMessageId());
      return id;
   }

   /**
    * Get list of installed packages
    *
    * @return
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<PackageInfo> getInstalledPackages() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PACKAGE_LIST);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      List<PackageInfo> list = new ArrayList<PackageInfo>();
      while(true)
      {
         final NXCPMessage response = waitForMessage(NXCPCodes.CMD_PACKAGE_INFO, msg.getMessageId());
         if (response.getFieldAsInt64(NXCPCodes.VID_PACKAGE_ID) == 0) break;
         list.add(new PackageInfo(response));
      }
      return list;
   }

   /**
    * Deploy agent packages onto given nodes
    *
    * @param packageId package ID
    * @param nodeList  list of nodes
    * @param listener  deployment progress listener (may be null)
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deployPackage(long packageId, Long[] nodeList, PackageDeploymentListener listener)
      throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DEPLOY_PACKAGE);
      msg.setFieldInt32(NXCPCodes.VID_PACKAGE_ID, (int) packageId);
      msg.setFieldInt32(NXCPCodes.VID_NUM_OBJECTS, nodeList.length);
      msg.setField(NXCPCodes.VID_OBJECT_LIST, nodeList);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      if (listener != null) listener.deploymentStarted();

      while(true)
      {
         final NXCPMessage response = waitForMessage(NXCPCodes.CMD_INSTALLER_INFO, msg.getMessageId(), 600000);
         final int status = response.getFieldAsInt32(NXCPCodes.VID_DEPLOYMENT_STATUS);
         if (status == PackageDeploymentListener.FINISHED) break;

         if (listener != null)
         {
            // Workaround for server bug: versions prior to 1.1.8 can send
            // irrelevant message texts for statuses other then FAILED
            if (status == PackageDeploymentListener.FAILED)
            {
               listener.statusUpdate(response.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID), status,
                  response.getFieldAsString(NXCPCodes.VID_ERROR_MESSAGE));
            }
            else
            {
               listener.statusUpdate(response.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID), status, "");
            }
         }
      }

      if (listener != null) listener.deploymentComplete();
   }

   /**
    * Lock package database
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void lockPackageDatabase() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LOCK_PACKAGE_DB);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Unlock package database
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unlockPackageDatabase() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UNLOCK_PACKAGE_DB);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Send SMS via server. User should have appropriate rights to execute this command.
    *
    * @param phoneNumber target phone number
    * @param message     message text
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void sendSMS(String phoneNumber, String message) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SEND_SMS);
      msg.setField(NXCPCodes.VID_RCPT_ADDR, phoneNumber);
      msg.setField(NXCPCodes.VID_MESSAGE, message);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Push data to server.
    *
    * @param data push data
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void pushDciData(DciPushData[] data) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_PUSH_DCI_DATA);
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, data.length);
      long varId = NXCPCodes.VID_PUSH_DCI_DATA_BASE;
      for(DciPushData d : data)
      {
         msg.setFieldInt32(varId++, (int) d.nodeId);
         if (d.nodeId == 0) msg.setField(varId++, d.nodeName);
         msg.setFieldInt32(varId++, (int) d.dciId);
         if (d.dciId == 0) msg.setField(varId++, d.dciName);
         msg.setField(varId++, d.value);
      }

      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Push value for single DCI.
    *
    * @param nodeId node ID
    * @param dciId  DCI ID
    * @param value  value to push
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void pushDciData(long nodeId, long dciId, String value) throws IOException, NXCException
   {
      pushDciData(new DciPushData[]{new DciPushData(nodeId, dciId, value)});
   }

   /**
    * Push value for single DCI.
    *
    * @param nodeName node name
    * @param dciName  DCI name
    * @param value    value to push
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void pushDciData(String nodeName, String dciName, String value) throws IOException, NXCException
   {
      pushDciData(new DciPushData[]{new DciPushData(nodeName, dciName, value)});
   }

   /**
    * @return the clientType
    */
   public int getClientType()
   {
      return clientType;
   }

   /**
    * Set client type. Can be one of the following:
    * DESKTOP_CLIENT
    * WEB_CLIENT
    * MOBILE_CLIENT
    * TABLET_CLIENT
    * APPLICATION_CLIENT
    * Must be called before connect(), otherwise will not have any effect. Ignored by servers prior to 1.2.2.
    *
    * @param clientType the clientType to set
    */
   public void setClientType(int clientType)
   {
      this.clientType = clientType;
   }

   /**
    * Get default date format provided by server
    * 
    * @return
    */
   public String getDateFormat()
   {
      return dateFormat;
   }

   /**
    * Get default time format provided by server
    * 
    * @return
    */
   public String getTimeFormat()
   {
      return timeFormat;
   }

   /**
    * Get time format for short form (usually without seconds).
    * 
    * @return
    */
   public String getShortTimeFormat()
   {
      return shortTimeFormat;
   }
   
   /**
    * Handover object cache to new session. After call to this method,
    * object cache of this session invalidated and should not be used.
    *
    * @param target target session object
    */
   public void handover(NXCSession target)
   {
      target.objectList = objectList;
      for(AbstractObject o : objectList.values())
      {
         o.setSession(target);
      }
      objectList = null;
   }

   /**
    * Get this session's ID on server.
    *
    * @return the sessionId
    */
   public int getSessionId()
   {
      return sessionId;
   }

   /**
    * Get list of all configured mapping tables.
    * 
    * @return
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<MappingTableDescriptor> listMappingTables() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LIST_MAPPING_TABLES);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      final List<MappingTableDescriptor> list = new ArrayList<MappingTableDescriptor>(count);
      long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new MappingTableDescriptor(response, varId));
         varId += 10;
      }
      return list;
   }

   /**
    * @param id
    * @return
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public MappingTable getMappingTable(int id) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_MAPPING_TABLE);
      msg.setFieldInt32(NXCPCodes.VID_MAPPING_TABLE_ID, id);
      sendMessage(msg);
      return new MappingTable(waitForRCC(msg.getMessageId()));
   }

   /**
    * Create new mapping table.
    * 
    * @param name name of new table
    * @param description description for new table
    * @param flags flags for new table
    * @return ID of new table object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int createMappingTable(String name, String description, int flags) throws IOException, NXCException
   {
      MappingTable mt = new MappingTable(name, description);
      mt.setFlags(flags);
      return updateMappingTable(mt);
   }

   /**
    * Create or update mapping table. If table ID is 0, new table will be created on server.
    * 
    * @param table mapping table
    * @return ID of new table object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int updateMappingTable(MappingTable table) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_MAPPING_TABLE);
      table.fillMessage(msg);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt32(NXCPCodes.VID_MAPPING_TABLE_ID);
   }

   /**
    * Delete mapping table
    * 
    * @param id mapping table ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteMappingTable(int id) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_MAPPING_TABLE);
      msg.setFieldInt32(NXCPCodes.VID_MAPPING_TABLE_ID, id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * @return the default DCI retention time in days
    */
   public final int getDefaultDciRetentionTime()
   {
      return defaultDciRetentionTime;
   }

   /**
    * @return the default DCI polling interval in seconds
    */
   public final int getDefaultDciPollingInterval()
   {
      return defaultDciPollingInterval;
   }
   
   /**
    * @return the minViewRefreshInterval
    */
   public int getMinViewRefreshInterval()
   {
      return minViewRefreshInterval;
   }

   /**
    * @return true if alarm status flow set to "strict" mode
    */
   public final boolean isStrictAlarmStatusFlow()
   {
      return strictAlarmStatusFlow;
   }

   /**
    * @return true if timed alarm acknowledgement is enabled
    */
   public boolean isTimedAlarmAckEnabled()
   {
      return timedAlarmAckEnabled;
   }

   /**
    * Get list of all configured DCI summary tables
    *
    * @return
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<DciSummaryTableDescriptor> listDciSummaryTables() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SUMMARY_TABLES);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      final List<DciSummaryTableDescriptor> list = new ArrayList<DciSummaryTableDescriptor>(count);
      long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new DciSummaryTableDescriptor(response, varId));
         varId += 10;
      }
      return list;
   }

   /**
    * Get DCI summary table configuration.
    *
    * @param id DCI summary table ID.
    * @return
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciSummaryTable getDciSummaryTable(int id) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SUMMARY_TABLE_DETAILS);
      msg.setFieldInt32(NXCPCodes.VID_SUMMARY_TABLE_ID, id);
      sendMessage(msg);
      return new DciSummaryTable(waitForRCC(msg.getMessageId()));
   }

   /**
    * Modify DCI summary table configuration. Will create new table object if id is 0.
    *
    * @param table DCI summary table configuration
    * @return assigned summary table ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int modifyDciSummaryTable(DciSummaryTable table) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_SUMMARY_TABLE);
      table.fillMessage(msg);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt32(NXCPCodes.VID_SUMMARY_TABLE_ID);
   }

   /**
    * Delete DCI summary table.
    *
    * @param id
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteDciSummaryTable(int id) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_SUMMARY_TABLE);
      msg.setFieldInt32(NXCPCodes.VID_SUMMARY_TABLE_ID, id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Query DCI summary table.
    *
    * @param tableId      DCI summary table ID
    * @param baseObjectId base container object ID
    * @return table with last values data for all nodes under given base container
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table queryDciSummaryTable(int tableId, long baseObjectId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_QUERY_SUMMARY_TABLE);
      msg.setFieldInt32(NXCPCodes.VID_SUMMARY_TABLE_ID, tableId);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int) baseObjectId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new Table(response);
   }

   /**
    * Query ad-hoc DCI summary table.
    *
    * @param baseObjectId base container object ID
    * @param columns columns for resulting table
    * @param function data aggregation function
    * @param periodStart start of query period
    * @param periodEnd end of query period
    * @return table with last values data for all nodes under given base container
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table queryAdHocDciSummaryTable(long baseObjectId, List<DciSummaryTableColumn> columns, AggregationFunction function, Date periodStart, Date periodEnd, boolean multiInstance) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_QUERY_ADHOC_SUMMARY_TABLE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)baseObjectId);
      msg.setFieldInt32(NXCPCodes.VID_NUM_COLUMNS, columns.size());
      msg.setFieldInt16(NXCPCodes.VID_FUNCTION, (function != null) ? function.getValue() : AggregationFunction.LAST.getValue());
      msg.setField(NXCPCodes.VID_TIME_FROM, periodStart);
      msg.setField(NXCPCodes.VID_TIME_TO, periodEnd);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, multiInstance ? 1 : 0);  // FIXME: define flags properly
      long id = NXCPCodes.VID_COLUMN_INFO_BASE;
      for(DciSummaryTableColumn c : columns)
      {
         c.fillMessage(msg, id);
         id += 10;
      }
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new Table(response);
   }

   /**
    * @return
    * @throws NXCException
    * @throws IOException
    */
   public List<UUID> listReports() throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_LIST_REPORTS);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      List<UUID> ret = new ArrayList<UUID>(count);
      long base = NXCPCodes.VID_UUID_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         ret.add(response.getFieldAsUUID(base + i));
      }
      return ret;
   }

   /**
    * @param reportId
    * @return
    * @throws NXCException
    * @throws IOException
    */
   public ReportDefinition getReportDefinition(UUID reportId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_GET_REPORT_DEFINITION);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      msg.setField(NXCPCodes.VID_LOCALE, Locale.getDefault().getLanguage());
      sendMessage(msg);
      return new ReportDefinition(reportId, waitForRCC(msg.getMessageId()));
   }

   /**
    * @param reportId
    * @param parameters
    * @return
    * @throws NXCException
    * @throws IOException
    */
   public UUID executeReport(UUID reportId, Map<String, String> parameters) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_SCHEDULE_EXECUTION);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      msg.setFieldInt32(NXCPCodes.VID_NUM_PARAMETERS, parameters.size());
      long varId = NXCPCodes.VID_PARAM_LIST_BASE;
      for(Entry<String, String> e : parameters.entrySet())
      {
         msg.setField(varId++, e.getKey());
         msg.setField(varId++, e.getValue());
      }
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsUUID(NXCPCodes.VID_JOB_ID);
   }

   /**
    * @param reportId
    * @return
    * @throws NXCException
    * @throws IOException
    */
   public List<ReportResult> listReportResults(UUID reportId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_LIST_RESULTS);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());

      List<ReportResult> results = new ArrayList<ReportResult>();
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      long base = NXCPCodes.VID_ROW_DATA_BASE;
      for(int i = 0; i < count; i++, base += 10)
      {
         ReportResult result = ReportResult.createFromMessage(response, base);
         results.add(result);
      }

      return results;
   }

   /**
    * @param reportId
    * @param jobId
    * @throws NXCException
    * @throws IOException
    */
   public void deleteReportResult(UUID reportId, UUID jobId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_DELETE_RESULT);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      msg.setField(NXCPCodes.VID_JOB_ID, jobId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * @param reportId
    * @param jobId
    * @param format
    * @return
    * @throws NXCException
    * @throws IOException
    */
   public File renderReport(UUID reportId, UUID jobId, ReportRenderFormat format) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_RENDER_RESULT);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      msg.setField(NXCPCodes.VID_JOB_ID, jobId);
      msg.setFieldInt32(NXCPCodes.VID_RENDER_FORMAT, format.getCode());
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      final File file = waitForFile(msg.getMessageId(), 600000);
      if (file == null)
      {
         throw new NXCException(RCC.IO_ERROR);
      }
      return file;
   }

	/**
	 * @param job
	 * @param parameters
	 * @throws NXCException
	 * @throws IOException
	 */
	public void scheduleReport(ReportingJob job, Map<String, String> parameters) throws NXCException, IOException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_SCHEDULE_EXECUTION);
		msg.setField(NXCPCodes.VID_REPORT_DEFINITION, job.getReportId());
		msg.setField(NXCPCodes.VID_RS_JOB_ID, job.getJobId());
		msg.setFieldInt32(NXCPCodes.VID_RS_JOB_TYPE, job.getType());
		msg.setFieldInt64(NXCPCodes.VID_TIMESTAMP, job.getStartTime().getTime());
		msg.setFieldInt32(NXCPCodes.VID_DAY_OF_WEEK, job.getDaysOfWeek());
		msg.setFieldInt32(NXCPCodes.VID_DAY_OF_MONTH, job.getDaysOfMonth());
		msg.setField(NXCPCodes.VID_COMMENTS, job.getComments());

		msg.setFieldInt32(NXCPCodes.VID_NUM_PARAMETERS, parameters.size());
		long varId = NXCPCodes.VID_PARAM_LIST_BASE;
		for(Entry<String, String> e : parameters.entrySet())
		{
			msg.setField(varId++, e.getKey());
			msg.setField(varId++, e.getValue());
		}
		sendMessage(msg);
		waitForRCC(msg.getMessageId());

		// FIXME: combine into one message
		if (job.isNotifyOnCompletion())
		{
         msg = newMessage(NXCPCodes.CMD_RS_ADD_REPORT_NOTIFY);
         msg.setField(NXCPCodes.VID_RS_JOB_ID, job.getJobId());
         msg.setFieldInt32(NXCPCodes.VID_RENDER_FORMAT, job.getRenderFormat().getCode());
         msg.setField(NXCPCodes.VID_RS_REPORT_NAME, job.getComments());  // FIXME: is this correct?
         msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, job.getEmailRecipients().size());
         varId = NXCPCodes.VID_ITEM_LIST;
         for(String s : job.getEmailRecipients())
            msg.setField(varId++, s);
         sendMessage(msg);
         waitForRCC(msg.getMessageId());
		}
	}

   /**
    * @param reportId
    * @return
    * @throws NXCException
    * @throws IOException
    */
   public List<ReportingJob> listScheduledJobs(UUID reportId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_LIST_SCHEDULES);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
		
      List<ReportingJob> result = new ArrayList<ReportingJob>();
      long varId = NXCPCodes.VID_ROW_DATA_BASE;
      int num = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      for (int i = 0; i < num; i++)
      {
         result.add(new ReportingJob(response, varId));
         varId += 20;
      }
      return result;
   }
   
   /**
    * @param reportId
    * @param jobId
    * @throws NXCException
    * @throws IOException
    */
   public void deleteReportSchedule(UUID reportId, UUID jobId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_DELETE_SCHEDULE);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      msg.setField(NXCPCodes.VID_JOB_ID, jobId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * @param clientAddress the clientAddress to set
    */
   public void setClientAddress(String clientAddress)
   {
      this.clientAddress = clientAddress;
   }

   /**
    * @param signature user's signature
    * @param challenge challenge string received from server
    * @return signed server challenge
    * @throws NXCException 
    */
   private byte[] signChallenge(Signature signature, byte[] challenge) throws NXCException
   {
      byte[] signed;

      try
      {
         signature.update(challenge);
         signed = signature.sign();
      }
      catch(SignatureException e)
      {
         throw new NXCException(RCC.ENCRYPTION_ERROR);
      }

      return signed;
   }
   
   /**
    * @return the clientLanguage
    */
   public String getClientLanguage()
   {
      return clientLanguage;
   }

   /**
    * @param clientLanguage the clientLanguage to set
    */
   public void setClientLanguage(String clientLanguage)
   {
      this.clientLanguage = clientLanguage;
   }

   /**
    * Get address map for subnet. Returned array contains one entry for each IP address
    * in a subnet. Element value could be eithet ID of the node with that IP address,
    * 0 for unused addresses, and 0xFFFFFFFF for subnet and broadcast addresses.
    * 
    * @param subnetId
    * @return
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long[] getSubnetAddressMap(long subnetId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SUBNET_ADDRESS_MAP);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)subnetId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsUInt32Array(NXCPCodes.VID_ADDRESS_MAP);
   }
   
   /**
    * Gets the list of configuration files.(Config id, name and sequence number)
    * 
    * @return the list of configuration files in correct sequence
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List <ConfigListElement> getConfigList() throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AGENT_CFG_LIST);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int size = response.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
      List <ConfigListElement> elements = new ArrayList<ConfigListElement>(size);
      long i, base;
      for(i = 0, base = NXCPCodes.VID_AGENT_CFG_LIST_BASE; i < size; i++, base += 10)
      {
         elements.add(new ConfigListElement(base, response));
      }
      Collections.sort(elements);
      return elements;
   }
   
   /**
    * Saves existing config 
    *  
    * @param id config id
    * @return content of requested by id configurations file
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ConfigContent getConfigContent(long id) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_OPEN_AGENT_CONFIG);
      msg.setFieldInt32(NXCPCodes.VID_CONFIG_ID, (int)id);
      sendMessage(msg);
      
      NXCPMessage response = waitForRCC(msg.getMessageId());      
      ConfigContent content = new ConfigContent(id, response);
      return content;
   }
   
   /**
    * Saves or creates new agent's config 
    * 
    * @param conf contents of config
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void saveAgentConfig(ConfigContent conf) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SAVE_AGENT_CONFIG);
      conf.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Delete config with given id. Does not change sequence number of following elements.
    * 
    * @param id agent configuration ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAgentConfig(long id) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_AGENT_CONFIG);
      msg.setFieldInt32(NXCPCodes.VID_CONFIG_ID, (int)id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Swaps 2 configs sequence numbers
    * 
    * @param id1
    * @param id2
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void swapAgentConfigs(long id1, long id2) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SWAP_AGENT_CONFIGS);
      msg.setFieldInt32(NXCPCodes.VID_CONFIG_ID, (int)id1);
      msg.setFieldInt32(NXCPCodes.VID_CONFIG_ID_2, (int)id2);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Get location history for given object.
    * 
    * @param objectId
    * @param from
    * @param to
    * @return
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<GeoLocation> getLocationHistory(long objectId, Date from, Date to) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_LOC_HISTORY);

      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      msg.setField(NXCPCodes.VID_TIME_FROM, from);
      msg.setField(NXCPCodes.VID_TIME_TO, to);
      sendMessage(msg);
      
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int size = response.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
      List <GeoLocation> elements = new ArrayList<GeoLocation>();
      long i, base;
      for(i = 0, base = NXCPCodes.VID_LOC_LIST_BASE; i < size; i++, base += 10)
      {
         elements.add(new GeoLocation(base, response));
      }
      Collections.sort(elements, new Comparator<GeoLocation>() {
         @Override
         public int compare(GeoLocation l1, GeoLocation l2)
         {
            return l1.getTimestamp().compareTo(l2.getTimestamp());
         }
      });
      return elements;
   }
   
   /**
    * Take screenshot from given node. Session to take screenshot from
    * can be identified either by ID or name. If ID is used name must
    * be set to null.
    * 
    * @param nodeId node object ID
    * @param sessionId session ID for session to take screenshot from
    * @param sessionName session name for session to take screenshot from
    * @return Screenshot as PNG image
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public byte[] takeScreenshot(long nodeId, String sessionName) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_TAKE_SCREENSHOT);
      msg.setFieldInt32(NXCPCodes.VID_NODE_ID, (int)nodeId);
      if (sessionName != null)
         msg.setField(NXCPCodes.VID_NAME, sessionName);
      sendMessage(msg);
      
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsBinary(NXCPCodes.VID_FILE_DATA);
   }
}
