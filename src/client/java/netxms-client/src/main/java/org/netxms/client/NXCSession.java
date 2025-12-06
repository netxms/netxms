/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
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
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.Signature;
import java.security.SignatureException;
import java.security.cert.Certificate;
import java.security.cert.CertificateEncodingException;
import java.util.ArrayList;
import java.util.Arrays;
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
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Consumer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.netxms.base.EncryptionContext;
import org.netxms.base.GeoLocation;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPDataInputStream;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.netxms.base.NXCPMsgWaitQueue;
import org.netxms.base.VersionInfo;
import org.netxms.client.agent.config.AgentConfiguration;
import org.netxms.client.agent.config.AgentConfigurationHandle;
import org.netxms.client.ai.AiAgentTask;
import org.netxms.client.ai.AiAssistantFunction;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.businessservices.BusinessServiceCheck;
import org.netxms.client.businessservices.BusinessServiceTicket;
import org.netxms.client.constants.AggregationFunction;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.client.constants.BackgroundTaskState;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.RCC;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ConditionDciInfo;
import org.netxms.client.datacollection.DCOStatusHolder;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.DciInfo;
import org.netxms.client.datacollection.DciLastValue;
import org.netxms.client.datacollection.DciPushData;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.client.datacollection.GraphFolder;
import org.netxms.client.datacollection.InterfaceTrafficDcis;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.client.datacollection.PredictionEngine;
import org.netxms.client.datacollection.RemoteChangeListener;
import org.netxms.client.datacollection.SimpleDciValue;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.datacollection.ThresholdStateChange;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.client.datacollection.TransformationTestResult;
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.client.datacollection.WinPerfObject;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.AlarmCategory;
import org.netxms.client.events.AlarmComment;
import org.netxms.client.events.BulkAlarmStateChangeData;
import org.netxms.client.events.Event;
import org.netxms.client.events.EventInfo;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventReference;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.events.SyslogRecord;
import org.netxms.client.log.Log;
import org.netxms.client.maps.MapDCIInstance;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.configs.MapDataSource;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.market.Repository;
import org.netxms.client.mt.MappingTable;
import org.netxms.client.mt.MappingTableDescriptor;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Asset;
import org.netxms.client.objects.AssetGroup;
import org.netxms.client.objects.AssetRoot;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.client.objects.BusinessServiceRoot;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Circuit;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.ClusterResource;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DashboardGroup;
import org.netxms.client.objects.DashboardRoot;
import org.netxms.client.objects.DashboardTemplate;
import org.netxms.client.objects.DependentNode;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.MutableObjectCategory;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.NetworkMapGroup;
import org.netxms.client.objects.NetworkMapRoot;
import org.netxms.client.objects.NetworkService;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.ObjectCategory;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.TemplateGroup;
import org.netxms.client.objects.TemplateRoot;
import org.netxms.client.objects.UnknownObject;
import org.netxms.client.objects.VPNConnector;
import org.netxms.client.objects.WirelessDomain;
import org.netxms.client.objects.Zone;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.client.objects.interfaces.NodeItemPair;
import org.netxms.client.objects.queries.ObjectQuery;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.client.objecttools.ObjectContextBase;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.client.objecttools.ObjectToolFolder;
import org.netxms.client.packages.PackageDeploymentJob;
import org.netxms.client.packages.PackageInfo;
import org.netxms.client.reporting.ReportDefinition;
import org.netxms.client.reporting.ReportRenderFormat;
import org.netxms.client.reporting.ReportResult;
import org.netxms.client.reporting.ReportingJob;
import org.netxms.client.reporting.ReportingJobConfiguration;
import org.netxms.client.server.AgentFile;
import org.netxms.client.server.ServerConsoleListener;
import org.netxms.client.server.ServerFile;
import org.netxms.client.server.ServerVariable;
import org.netxms.client.snmp.MibCompilationLogEntry;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.client.snmp.SnmpTrapLogRecord;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.client.snmp.SnmpWalkListener;
import org.netxms.client.topology.ArpCacheEntry;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.client.topology.FdbEntry;
import org.netxms.client.topology.NetworkPath;
import org.netxms.client.topology.OSPFInfo;
import org.netxms.client.topology.Route;
import org.netxms.client.topology.VlanInfo;
import org.netxms.client.topology.WirelessStation;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.AuthenticationToken;
import org.netxms.client.users.ResponsibleUser;
import org.netxms.client.users.TwoFactorAuthenticationMethod;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.jcraft.jzlib.Deflater;
import com.jcraft.jzlib.JZlib;

/**
 * Communication session with NetXMS server.
 */
public class NXCSession
{
   // DCI resolve flags
   public static final int DCI_RES_SEARCH_NAME = 0x01;

   // Various public constants
   public static final int DEFAULT_CONN_PORT = 4701;

   // Core notification channels
   public static final String CHANNEL_AGENT_TUNNELS = "Core.AgentTunnels";
   public static final String CHANNEL_ALARMS = "Core.Alarms";
   public static final String CHANNEL_AUDIT_LOG = "Core.Audit";
   public static final String CHANNEL_DC_THRESHOLDS = "Core.DC.Thresholds";
   public static final String CHANNEL_GEO_AREAS = "Core.GeoAreas";
   public static final String CHANNEL_EVENTS = "Core.Events";
   public static final String CHANNEL_OBJECTS = "Core.Objects";
   public static final String CHANNEL_PACKAGE_JOBS = "Core.PackageJobs";
   public static final String CHANNEL_SNMP_TRAPS = "Core.SNMP.Traps";
   public static final String CHANNEL_SYSLOG = "Core.Syslog";
   public static final String CHANNEL_USERDB = "Core.UserDB";

   // Object sync options
   public static final int OBJECT_SYNC_NOTIFY = 0x0001;
   public static final int OBJECT_SYNC_WAIT = 0x0002;
   public static final int OBJECT_SYNC_ALLOW_PARTIAL = 0x0004;

   // Configuration import options
   public static final int CFG_IMPORT_REPLACE_EVENTS                    = 0x0001;
   public static final int CFG_IMPORT_REPLACE_ACTIONS                   = 0x0002;
   public static final int CFG_IMPORT_REPLACE_TEMPLATES                 = 0x0004;
   public static final int CFG_IMPORT_REPLACE_TRAPS                     = 0x0008;
   public static final int CFG_IMPORT_REPLACE_SCRIPTS                   = 0x0010;
   public static final int CFG_IMPORT_REPLACE_SUMMARY_TABLES            = 0x0020;
   public static final int CFG_IMPORT_REPLACE_OBJECT_TOOLS              = 0x0040;
   public static final int CFG_IMPORT_REPLACE_EPP_RULES                 = 0x0080;
   public static final int CFG_IMPORT_REPLACE_TEMPLATE_NAMES_LOCATIONS  = 0x0100;
   public static final int CFG_IMPORT_DELETE_EMPTY_TEMPLATE_GROUPS      = 0x0200;
   public static final int CFG_IMPORT_REPLACE_WEB_SVCERVICE_DEFINITIONS = 0x0400;
   public static final int CFG_IMPORT_REPLACE_AM_DEFINITIONS            = 0x0800;

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
   private static final int RECEIVED_FILE_TTL = 300000; // 300 seconds
   private static final int FILE_BUFFER_SIZE = 32768; // 32KB

   // Logger
   private static Logger logger = LoggerFactory.getLogger(NXCSession.class);

   // Internal synchronization objects
   private final Semaphore syncObjects = new Semaphore(1);
   private final Semaphore syncUserDB = new Semaphore(1);

   // Connection-related attributes
   private String connAddress;
   private int connPort;
   private String connClientInfo = "nxjclient/" + VersionInfo.version();
   private int clientType = DESKTOP_CLIENT;
   private String clientAddress = null;
   private String clientLanguage = "en";
   private boolean ignoreProtocolVersion = false;
   private boolean reconnectEnabled = false;

   // Information about logged in user
   private int sessionId;
   private int userId;
   private String userName;
   private AuthenticationType authenticationMethod;
   private long userSystemRights;
   private String uiAccessRules;
   private boolean passwordExpired;
   private int graceLogins;
   private String authenticationToken = null;

   // Internal communication data
   private Socket socket = null;
   private NXCPMsgWaitQueue msgWaitQueue = null;
   private ReceiverThread recvThread = null;
   private HousekeeperThread housekeeperThread = null;
   private Thread reconnectThread = null;
   private AtomicLong requestId = new AtomicLong(1);
   private boolean connected = false;
   private boolean disconnected = false;
   private boolean serverConsoleConnected = false;
   private Integer serverConsoleConnectionCount = 0;
   private Object serverConsoleConnectionLock = new Object();
   private boolean enableCompression = true; // Compression is administratively enabled if true
   private boolean allowCompression = false; // Compression is allowed after protocol negotiation with the server
   private EncryptionContext encryptionContext = null;
   private Throwable receiverStopCause = null;

   // Communication parameters
   private int defaultRecvBufferSize = 4194304; // Default is 4MB
   private int maxRecvBufferSize = 33554432;    // Max is 32MB
   private int connectTimeout = 10000; // Default is 10 seconds  
   private int commandTimeout = 30000; // Default is 30 seconds
   private int serverCommandOutputTimeout = 60000;

   // Notification listeners and queue
   private LinkedBlockingQueue<SessionNotification> notificationQueue = new LinkedBlockingQueue<SessionNotification>(8192);
   private Set<SessionListener> listeners = new HashSet<SessionListener>(0);
   private Set<ServerConsoleListener> consoleListeners = new HashSet<ServerConsoleListener>(0);
   private Map<Long, ProgressListener> progressListeners = new HashMap<Long, ProgressListener>(0);

   // Active server channel subscriptions
   private Map<String, Integer> serverChannelSubscriptions = new HashMap<String, Integer>(0);

   // Message subscriptions
   private Map<MessageSubscription, MessageHandler> messageSubscriptions = new HashMap<MessageSubscription, MessageHandler>(0);

   // Received files
   private Map<Long, NXCReceivedFile> receivedFiles = new HashMap<Long, NXCReceivedFile>();

   // Received file updates(for file monitoring)
   private Map<UUID, String> receivedFileUpdates = new HashMap<UUID, String>();

   // Server information
   private ProtocolVersion protocolVersion;
   private String serverVersion = "(unknown)";
   private String serverBuild = "(unknown)";
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
   private Set<String> serverComponents = new HashSet<String>(0);
   private String serverName;
   private String serverColor;
   private String objectMaintenancePredefinedPeriods;
   private int networkMapDefaultWidth;
   private int networkMapDefaultHeight;

   // Configuration hints
   private Map<String, String> clientConfigurationHints = new HashMap<String, String>();

   // Objects
   private Map<Long, AbstractObject> objectList = new HashMap<Long, AbstractObject>();
   private Map<UUID, AbstractObject> objectListGUID = new HashMap<UUID, AbstractObject>();
   private Map<Long, AbstractObject> partialObjectList = new HashMap<Long, AbstractObject>();
   private Map<Integer, Zone> zoneList = new HashMap<Integer, Zone>();
   private Map<Integer, ObjectCategory> objectCategories = new HashMap<Integer, ObjectCategory>();
   private boolean objectsSynchronized = false;
   private Set<String> responsibleUserTags = new HashSet<String>();

   // Users
   private Map<Integer, AbstractUserObject> userDatabase = new HashMap<Integer, AbstractUserObject>();
   private Map<UUID, AbstractUserObject> userDatabaseGUID = new HashMap<UUID, AbstractUserObject>();
   private Set<Integer> missingUsers = new HashSet<Integer>(); // users that cannot be synchronized
   private boolean userDatabaseSynchronized = false;
   private Set<Integer> userSyncList = new HashSet<Integer>();
   private List<Runnable> callbackList = new ArrayList<Runnable>();

   // Event objects
   private Map<Integer, EventTemplate> eventTemplates = new HashMap<>();
   private boolean eventTemplatesSynchronized = false;

   // Alarm categories
   private Map<Long, AlarmCategory> alarmCategories = new HashMap<Long, AlarmCategory>();
   private boolean alarmCategoriesSynchronized = false;

   // Message of the day
   private String messageOfTheDay;
   
   // Registered license problems
   private LicenseProblem[] licenseProblems = null;

   // Cached list of prediction engines
   private List<PredictionEngine> predictionEngines = null;

   // TCP proxies
   private Map<Integer, TcpProxy> tcpProxies = new HashMap<Integer, TcpProxy>();
   private AtomicInteger tcpProxyChannelId = new AtomicInteger(1);
   private boolean clientAssignedTcpProxyId = false;

   // Set of objects whose child objects were already synchronized
   private Set<Long> synchronizedObjectSet = new HashSet<Long>();

   // OUI cache
   private OUICache ouiCache = null;

   // Asset management schema
   private Map<String, AssetAttribute> assetManagementSchema = new HashMap<String, AssetAttribute>();

   /**
    * Message subscription class
    */
   final protected static class MessageSubscription
   {
      protected int messageCode;
      protected long messageId;

      /**
       * @param messageCode The message code
       * @param messageId   The message ID
       */
      protected MessageSubscription(int messageCode, long messageId)
      {
         this.messageCode = messageCode;
         this.messageId = messageId;
      }

      /**
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

      /**
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
         super("Network Message Receiver");
         setDaemon(true);
         start();
      }

      @Override
      public void run()
      {
         logger.debug("Network receiver thread started");

         final NXCPMessageReceiver receiver = new NXCPMessageReceiver(defaultRecvBufferSize, maxRecvBufferSize);
         InputStream in;

         try
         {
            in = socket.getInputStream();
         }
         catch(IOException e)
         {
            logger.debug("Cannot get socket input stream", e);
            return; // Stop receiver thread if input stream cannot be obtained
         }

         int errorCount = 0;
         while(socket.isConnected())
         {
            try
            {
               NXCPMessage msg = receiver.receiveMessage(in, encryptionContext);
               errorCount = 0;
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
                        boolean newObject = false;
                        final AbstractObject object = createObjectFromMessage(msg);
                        if (object.isPartialObject())
                        {
                           synchronized(objectList)
                           {
                              partialObjectList.put(object.getObjectId(), object);
                           }
                        }
                        else
                        {
                           synchronized(objectList)
                           {
                              newObject = (objectList.put(object.getObjectId(), object) == null);
                              objectListGUID.put(object.getGuid(), object);
                              if (object instanceof Zone)
                                 zoneList.put(((Zone)object).getUIN(), (Zone)object);
                           }
                        }
                        if (msg.getMessageCode() == NXCPCodes.CMD_OBJECT_UPDATE)
                        {
                           // For new objects, also send update notifications for all its parents
                           if (newObject)
                           {
                              for(AbstractObject parent : object.getParentsAsArray())
                                 sendNotification(new SessionNotification(SessionNotification.OBJECT_CHANGED, parent.getObjectId(), parent));
                           }
                           sendNotification(new SessionNotification(SessionNotification.OBJECT_CHANGED, object.getObjectId(), object));
                        }
                     }
                     else
                     {
                        long objectId = msg.getFieldAsInt32(NXCPCodes.VID_OBJECT_ID);
                        synchronized(objectList)
                        {
                           AbstractObject object = objectList.get(objectId);
                           if (object != null)
                           {
                              objectListGUID.remove(object.getGuid());
                              objectList.remove(objectId);
                              if (object instanceof Zone)
                                 zoneList.remove(((Zone)object).getUIN());
                           }
                        }
                        sendNotification(new SessionNotification(SessionNotification.OBJECT_DELETED, objectId));
                     }
                     break;
                  case NXCPCodes.CMD_OBJECT_LIST_END:
                     completeSync(syncObjects);
                     break;
                  case NXCPCodes.CMD_OBJECT_CATEGORY_UPDATE:
                     processObjectCategoryUpdate(msg);
                     break;
                  case NXCPCodes.CMD_GEO_AREA_UPDATE:
                     processGeoAreaUpdate(msg);
                     break;
                  case NXCPCodes.CMD_USER_DATA:
                     final User user = new User(msg);
                     synchronized(userDatabase)
                     {
                        if (user.isDeleted())
                        {
                           AbstractUserObject o = userDatabase.remove(user.getId());
                           if (o != null)
                              userDatabaseGUID.remove(o.getGuid());
                           missingUsers.add(user.getId());
                        }
                        else
                        {
                           userDatabase.put(user.getId(), user);
                           userDatabaseGUID.put(user.getGuid(), user);
                           missingUsers.remove(user.getId());
                        }
                     }
                     break;
                  case NXCPCodes.CMD_GROUP_DATA:
                     final UserGroup group = new UserGroup(msg);
                     synchronized(userDatabase)
                     {
                        if (group.isDeleted())
                        {
                           AbstractUserObject o = userDatabase.remove(group.getId());
                           if (o != null)
                              userDatabaseGUID.remove(o.getGuid());
                           missingUsers.add(group.getId());
                        }
                        else
                        {
                           userDatabase.put(group.getId(), group);
                           userDatabaseGUID.put(group.getGuid(), group);
                           missingUsers.add(group.getId());
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
                           msg.getFieldAsInt32(NXCPCodes.VID_NOTIFICATION_CODE) + SessionNotification.NOTIFY_BASE, new Alarm(msg)));
                     break;
                  case NXCPCodes.CMD_BULK_ALARM_STATE_CHANGE:
                     processBulkAlarmStateChange(msg);
                     break;
                  case NXCPCodes.CMD_FILE_DATA:
                     processFileData(msg);
                     break;
                  case NXCPCodes.CMD_FILE_MONITORING:
                     processFileUpdate(msg);
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
                  case NXCPCodes.CMD_ADM_MESSAGE:
                     processConsoleOutput(msg);
                     break;
                  case NXCPCodes.CMD_IMAGE_LIBRARY_UPDATE:
                     processImageLibraryUpdate(msg);
                     break;
                  case NXCPCodes.CMD_GRAPH_UPDATE:
                     GraphDefinition graph = GraphDefinition.createGraphSettings(msg, NXCPCodes.VID_GRAPH_LIST_BASE);
                     sendNotification(new SessionNotification(SessionNotification.PREDEFINED_GRAPHS_CHANGED, graph.getId(), graph));
                     break;
                  case NXCPCodes.CMD_ALARM_CATEGORY_UPDATE:
                     processAlarmCategoryConfigChange(msg);
                     break;
                  case NXCPCodes.CMD_THRESHOLD_UPDATE:
                     processThresholdChange(msg);
                     break;
                  case NXCPCodes.CMD_TCP_PROXY_DATA:
                     processTcpProxyData((int)msg.getMessageId(), msg.getBinaryData());
                     break;
                  case NXCPCodes.CMD_CLOSE_TCP_PROXY:
                     processTcpProxyClosure(msg.getFieldAsInt32(NXCPCodes.VID_CHANNEL_ID), msg.getFieldAsInt32(NXCPCodes.VID_RCC));
                     break;
                  case NXCPCodes.CMD_MODIFY_NODE_DCI:
                     DataCollectionObject dco;
                     int type = msg.getFieldAsInt32(NXCPCodes.VID_DCOBJECT_TYPE);
                     switch(type)
                     {
                        case DataCollectionObject.DCO_TYPE_ITEM:
                           dco = new DataCollectionItem(null, msg);
                           break;
                        case DataCollectionObject.DCO_TYPE_TABLE:
                           dco = new DataCollectionTable(null, msg);
                           break;
                        default:
                           dco = null;
                           break;
                     }
                     sendNotification(
                           new SessionNotification(SessionNotification.DCI_UPDATE, msg.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID),
                                 dco));
                     break;
                  case NXCPCodes.CMD_DELETE_NODE_DCI:
                     sendNotification(
                           new SessionNotification(SessionNotification.DCI_DELETE, msg.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID),
                                 (Long)msg.getFieldAsInt64(NXCPCodes.VID_DCI_ID)));
                     break;
                  case NXCPCodes.CMD_SET_DCI_STATUS:
                     int itemCount = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
                     final long[] itemList = new long[itemCount];
                     int pos = 0;
                     for(int i = 0; i < itemCount; i++)
                     {
                        itemList[pos++] = msg.getFieldAsInt32(NXCPCodes.VID_ITEM_LIST + i);
                     }
                     sendNotification(new SessionNotification(SessionNotification.DCI_STATE_CHANGE,
                           msg.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID),
                           new DCOStatusHolder(itemList, DataCollectionObjectStatus.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_DCI_STATUS)))));
                     break;
                  case NXCPCodes.CMD_UPDATE_AGENT_POLICY:
                     sendNotification(new SessionNotification(SessionNotification.POLICY_MODIFIED,
                           msg.getFieldAsInt64(NXCPCodes.VID_TEMPLATE_ID), new AgentPolicy(msg)));
                     break;
                  case NXCPCodes.CMD_DELETE_AGENT_POLICY:
                     sendNotification(new SessionNotification(SessionNotification.POLICY_DELETED,
                           msg.getFieldAsInt64(NXCPCodes.VID_TEMPLATE_ID), msg.getFieldAsUUID(NXCPCodes.VID_GUID)));
                     break;
                  case NXCPCodes.CMD_UPDATE_SYSTEM_ACCESS_RIGHTS:
                     userSystemRights = msg.getFieldAsInt64(NXCPCodes.VID_USER_SYS_RIGHTS);
                     sendNotification(new SessionNotification(SessionNotification.SYSTEM_ACCESS_CHANGED, userSystemRights));
                     break;
                  case NXCPCodes.CMD_UPDATE_BIZSVC_CHECK:
                     sendNotification(
                           new SessionNotification(SessionNotification.BIZSVC_CHECK_MODIFIED, msg.getFieldAsInt64(NXCPCodes.VID_CHECK_LIST_BASE),
                                 new BusinessServiceCheck(msg, NXCPCodes.VID_CHECK_LIST_BASE)));
                     break;
                  case NXCPCodes.CMD_DELETE_BIZSVC_CHECK:
                     sendNotification(new SessionNotification(SessionNotification.BIZSVC_CHECK_DELETED, msg.getFieldAsInt64(NXCPCodes.VID_CHECK_ID)));
                     break;
                  case NXCPCodes.CMD_AGENT_TUNNEL_UPDATE:
                     sendNotification(
                           new SessionNotification(msg.getFieldAsInt32(NXCPCodes.VID_NOTIFICATION_CODE) + SessionNotification.NOTIFY_BASE, new AgentTunnel(msg, NXCPCodes.VID_ELEMENT_LIST_BASE)));
                     break;
                  case NXCPCodes.CMD_UPDATE_ASSET_ATTRIBUTE:
                     AssetAttribute attr = new AssetAttribute(msg, NXCPCodes.VID_AM_ATTRIBUTES_BASE);
                     sendNotification(new SessionNotification(SessionNotification.AM_ATTRIBUTE_UPDATED, 0, attr));
                     synchronized(assetManagementSchema)
                     {
                        assetManagementSchema.put(attr.getName(), attr);
                     }
                     break;
                  case NXCPCodes.CMD_DELETE_ASSET_ATTRIBUTE:
                     String attrName = msg.getFieldAsString(NXCPCodes.VID_NAME);
                     sendNotification(new SessionNotification(SessionNotification.AM_ATTRIBUTE_DELETED, 0, attrName));
                     synchronized(assetManagementSchema)
                     {
                        assetManagementSchema.remove(attrName);
                     }
                     break;
                  case NXCPCodes.CMD_PACKAGE_DEPLOYMENT_JOB_UPDATE:
                     sendNotification(new SessionNotification(SessionNotification.PACKAGE_DEPLOYMENT_JOB_CHANGED, msg.getFieldAsInt64(NXCPCodes.VID_ELEMENT_LIST_BASE),
                           new PackageDeploymentJob(msg, NXCPCodes.VID_ELEMENT_LIST_BASE)));
                     break;
                  default:
                     // Check subscriptions
                     synchronized(messageSubscriptions)
                     {
                        MessageSubscription s = new MessageSubscription(msg.getMessageCode(), msg.getMessageId());
                        MessageHandler handler = messageSubscriptions.get(s);
                        if (handler != null)
                        {
                           try
                           {
                              if (handler.processMessage(msg))
                                 msg = null;
                           }
                           catch(Exception e)
                           {
                              logger.error("Exception in message handler", e);
                           }

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
               if (!disconnected)
               {
                  logger.debug("Receiver error", e);
                  if (reconnectEnabled)
                  {
                     backgroundReconnect();
                     logger.info("Network receiver thread stopped");
                     return; // Stop this thread without normal cleanup
                  }
                  receiverStopCause = e;
               }
               break; // Stop on read errors
            }
            catch(NXCPException e)
            {
               logger.debug("Receiver error", e);
               if (e.getErrorCode() == NXCPException.SESSION_CLOSED)
               {
                  receiverStopCause = e;
                  break;
               }
               errorCount++;
            }
            catch(NXCException e)
            {
               logger.debug("Receiver error", e);
               if (e.getErrorCode() == RCC.ENCRYPTION_ERROR)
               {
                  receiverStopCause = e;
                  break;
               }
               errorCount++;
            }
            if (errorCount > 100)
            {
               receiverStopCause = new NXCPException(NXCPException.FATAL_PROTOCOL_ERROR);
               break;
            }
         }

         synchronized(tcpProxies)
         {
            Throwable cause = (receiverStopCause != null) ? receiverStopCause : new NXCPException(NXCPException.SESSION_CLOSED);
            for(TcpProxy p : tcpProxies.values())
               p.abort(cause);
         }

         if (!disconnected)
            backgroundDisconnect(SessionNotification.CONNECTION_BROKEN);

         logger.info("Network receiver thread stopped");
         msgWaitQueue.shutdown();
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
       * Process CMD_BULK_ALARM_STATE_CHANGE notification message
       *
       * @param msg NXCP message
       */
      private void processBulkAlarmStateChange(final NXCPMessage msg)
      {
         int code = msg.getFieldAsInt32(NXCPCodes.VID_NOTIFICATION_CODE) + SessionNotification.NOTIFY_BASE;
         sendNotification(new SessionNotification(code, new BulkAlarmStateChangeData(msg)));
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
               if (eventTemplatesSynchronized)
               {
                  resyncEventTemplates();
               }
               break;
            case SessionNotification.SESSION_KILLED:
            case SessionNotification.SERVER_SHUTDOWN:
            case SessionNotification.CONNECTION_BROKEN:
               backgroundDisconnect(code);
               return;  // backgroundDisconnect will send disconnect notification
            case SessionNotification.OBJECT_CATEGORY_DELETED:
               synchronized(objectCategories)
               {
                  objectCategories.remove((int)data);
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
      private void processFileUpdate(final NXCPMessage msg)
      {
         UUID monitorId = msg.getFieldAsUUID(NXCPCodes.VID_MONITOR_ID);
         String newData = msg.getFieldAsString(NXCPCodes.VID_FILE_DATA);
         synchronized(receivedFileUpdates)
         {
            String existingData = receivedFileUpdates.get(monitorId);
            receivedFileUpdates.put(monitorId, (existingData != null) ? existingData + newData : newData);
            receivedFileUpdates.notifyAll();
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
         int bytes = file.writeData(msg.getBinaryData(), msg.isCompressedStream());
         notifyProgressListener(id, bytes);
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
            file.abortTransfer(msg.getFieldAsBoolean(NXCPCodes.VID_JOB_CANCELLED));
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
         final int id = msg.getFieldAsInt32(NXCPCodes.VID_USER_ID);

         AbstractUserObject object = null;
         switch(code)
         {
            case SessionNotification.USER_DB_OBJECT_CREATED:
            case SessionNotification.USER_DB_OBJECT_MODIFIED:
               object = ((id & 0x40000000) != 0) ? new UserGroup(msg) : new User(msg);
               synchronized(userDatabase)
               {
                  userDatabase.put(id, object);
                  userDatabaseGUID.put(object.getGuid(), object);
                  missingUsers.remove(id);
               }
               break;
            case SessionNotification.USER_DB_OBJECT_DELETED:
               synchronized(userDatabase)
               {
                  object = userDatabase.get(id);
                  if (object != null)
                  {
                     userDatabase.remove(id);
                     userDatabaseGUID.remove(object.getGuid());
                  }
                  missingUsers.add(id);
               }
               break;
         }

         // Send notification if changed object was found in local database copy
         // or added to it and notification code was known
         if (object != null)
            sendNotification(new SessionNotification(SessionNotification.USER_DB_CHANGED, code, object));
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
         int eventCode = msg.getFieldAsInt32(NXCPCodes.VID_EVENT_CODE);
         EventTemplate tmpl = (code != SessionNotification.EVENT_TEMPLATE_DELETED) ?
               new EventTemplate(msg, NXCPCodes.VID_ELEMENT_LIST_BASE) :
               null;
         if (eventTemplatesSynchronized)
         {
            synchronized(eventTemplates)
            {
               if (code == SessionNotification.EVENT_TEMPLATE_DELETED)
               {
                  eventTemplates.remove(eventCode);
               }
               else
               {
                  eventTemplates.put(tmpl.getCode(), tmpl);
               }
            }
         }
         sendNotification(new SessionNotification(code, tmpl == null ? eventCode : tmpl.getCode(), tmpl));
      }

      /**
       * Process server notification on image library change
       *
       * @param msg notification message
       */
      private void processImageLibraryUpdate(NXCPMessage msg)
      {
         final UUID imageGuid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
         final int flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
         sendNotification(new SessionNotification(SessionNotification.IMAGE_LIBRARY_CHANGED,
               flags == 0 ? SessionNotification.IMAGE_UPDATED : SessionNotification.IMAGE_DELETED, imageGuid));
      }

      /**
       * Process object category update
       *
       * @param msg notification message
       */
      private void processObjectCategoryUpdate(NXCPMessage msg)
      {
         ObjectCategory category = new ObjectCategory(msg, NXCPCodes.VID_ELEMENT_LIST_BASE);
         synchronized(objectCategories)
         {
            objectCategories.put(category.getId(), category);
         }
         sendNotification(new SessionNotification(SessionNotification.OBJECT_CATEGORY_UPDATED, category.getId(), category));
      }

      /**
       * Process geographical area update
       *
       * @param msg notification message
       */
      private void processGeoAreaUpdate(NXCPMessage msg)
      {
         GeoArea area = new GeoArea(msg, NXCPCodes.VID_ELEMENT_LIST_BASE);
         sendNotification(new SessionNotification(SessionNotification.GEO_AREA_UPDATED, area.getId(), area));
      }

      /**
       * Process server notification on threshold change
       *
       * @param msg notification message
       */
      public void processThresholdChange(NXCPMessage msg)
      {
         sendNotification(new SessionNotification(SessionNotification.THRESHOLD_STATE_CHANGED,
               msg.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID), new ThresholdStateChange(msg)));
      }

      /**
       * Process server notification on alarm category configuration change
       *
       * @param msg notification message
       */
      private void processAlarmCategoryConfigChange(final NXCPMessage msg)
      {
         int code = msg.getFieldAsInt32(NXCPCodes.VID_NOTIFICATION_CODE) + SessionNotification.NOTIFY_BASE;
         long categoryId = msg.getFieldAsInt64(NXCPCodes.VID_ELEMENT_LIST_BASE);
         AlarmCategory ac = (code != SessionNotification.ALARM_CATEGORY_DELETED) ?
               new AlarmCategory(msg, NXCPCodes.VID_ELEMENT_LIST_BASE) :
               null;
         if (alarmCategoriesSynchronized)
         {
            synchronized(alarmCategories)
            {
               if (code == SessionNotification.ALARM_CATEGORY_DELETED)
               {
                  alarmCategories.remove(categoryId);
               }
               else
               {
                  alarmCategories.put(categoryId, ac);
               }
            }
         }
         sendNotification(new SessionNotification(code, categoryId, ac));
      }

      /**
       * Process TCP proxy data
       *
       * @param channelId proxy channel ID
       * @param data      received data block
       */
      private void processTcpProxyData(int channelId, byte[] data)
      {
         TcpProxy proxy;
         synchronized(tcpProxies)
         {
            proxy = tcpProxies.get(channelId);
         }
         if (proxy != null)
         {
            proxy.processRemoteData(data);
         }
         else
         {
            logger.debug("Received TCP proxy data for unknown channel ID " + channelId);
         }
      }

      /**
       * Process TCP proxy closure.
       *
       * @param channelId proxy channel ID
       */
      private void processTcpProxyClosure(int channelId, int rcc)
      {
         TcpProxy proxy;
         synchronized(tcpProxies)
         {
            proxy = tcpProxies.remove(channelId);
         }
         if (proxy != null)
         {
            if (rcc == RCC.SUCCESS)
               proxy.localClose();
            else
               proxy.abort(new NXCException(rcc));
         }
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

      /**
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
                     h.setExpired();
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

      /**
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
                  logger.error("Unhandled exception in notification handler", e);
               }
            }
         }
         cachedListenerList = null;
         logger.debug("Client session notification processor stopped");
      }
   }
   
   /**
    * User synchronization thread
    */
   class BackgroundUserSync extends Thread
   {
      public BackgroundUserSync()
      {
         setName("BackgroundUserSync");
         setDaemon(true);
         start();
      }

      /**
       * @see java.lang.Thread#run()
       */
      @Override
      public void run()
      {
         while(!disconnected)
         {
            synchronized(userSyncList)
            {
               while(userSyncList.isEmpty())
               {
                  try
                  {
                     userSyncList.wait();
                  }
                  catch(InterruptedException e)
                  {
                  }
                  if (disconnected)
                     return;
               }
            }

            // Delay actual sync in case more synchronization requests will come
            try
            {
               Thread.sleep(200);
            }
            catch(InterruptedException e)
            {
            }

            Set<Integer> userSyncListCopy;
            List<Runnable> callbackListCopy;
            synchronized(userSyncList)
            {
               userSyncListCopy = userSyncList;
               userSyncList = new HashSet<Integer>();
               callbackListCopy = callbackList;
               callbackList = new ArrayList<Runnable>();
            }

            try
            {
               syncMissingUsers(userSyncListCopy);
               for(Runnable cb : callbackListCopy)
                  cb.run();
            }
            catch(Exception e)
            {
               logger.error("Exception while synchronizing user database objects", e);
            }
         }         
      }
   }

   /**
    * Create session object that will connect to given address on default port (4701) without encryption.
    * 
    * @param connAddress server host name or IP address
    */
   public NXCSession(String connAddress)
   {
      this(connAddress, DEFAULT_CONN_PORT, true);
   }

   /**
    * Create session object that will connect to given address on given port without encryption.
    * 
    * @param connAddress server host name or IP address
    * @param connPort TCP port
    */
   public NXCSession(String connAddress, int connPort)
   {
      this(connAddress, connPort, true);
   }

   /**
    * Create session object that will connect to given address on given port.
    * 
    * @param connAddress server host name or IP address
    * @param connPort TCP port
    * @param enableCompression enable message compression
    */
   public NXCSession(String connAddress, int connPort, boolean enableCompression)
   {
      this.connAddress = connAddress;
      this.connPort = connPort;
      this.enableCompression = enableCompression;
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
         case AbstractObject.OBJECT_ASSET:
            object = new Asset(msg, this);
            break;
         case AbstractObject.OBJECT_ASSETGROUP:
            object = new AssetGroup(msg, this);
            break;
         case AbstractObject.OBJECT_ASSETROOT:
            object = new AssetRoot(msg, this);
            break;
         case AbstractObject.OBJECT_BUSINESSSERVICE:
            object = new BusinessService(msg, this);
            break;
         case AbstractObject.OBJECT_BUSINESSSERVICEPROTOTYPE:
            object = new BusinessServicePrototype(msg, this);
            break;
         case AbstractObject.OBJECT_BUSINESSSERVICEROOT:
            object = new BusinessServiceRoot(msg, this);
            break;
         case AbstractObject.OBJECT_CHASSIS:
            object = new Chassis(msg, this);
            break;
         case AbstractObject.OBJECT_CIRCUIT:
            object = new Circuit(msg, this);
            break;
         case AbstractObject.OBJECT_CLUSTER:
            object = new Cluster(msg, this);
            break;
         case AbstractObject.OBJECT_COLLECTOR:
            object = new Collector(msg, this);
            break;
         case AbstractObject.OBJECT_CONDITION:
            object = new Condition(msg, this);
            break;
         case AbstractObject.OBJECT_CONTAINER:
            object = new Container(msg, this);
            break;
         case AbstractObject.OBJECT_DASHBOARDGROUP:
            object = new DashboardGroup(msg, this);
            break;
         case AbstractObject.OBJECT_DASHBOARD:
            object = new Dashboard(msg, this);
            break;
         case AbstractObject.OBJECT_DASHBOARDROOT:
            object = new DashboardRoot(msg, this);
            break;
         case AbstractObject.OBJECT_DASHBOARDTEMPLATE:
            object = new DashboardTemplate(msg, this);
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
         case AbstractObject.OBJECT_RACK:
            object = new Rack(msg, this);
            break;
         case AbstractObject.OBJECT_SENSOR:
            object = new Sensor(msg, this);
            break;
         case AbstractObject.OBJECT_SERVICEROOT:
            object = new ServiceRoot(msg, this);
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
         case AbstractObject.OBJECT_WIRELESSDOMAIN:
            object = new WirelessDomain(msg, this);
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
         response.setField(NXCPCodes.VID_SESSION_KEY, encryptionContext.getEncryptedSessionKey());
         response.setField(NXCPCodes.VID_SESSION_IV, encryptionContext.getEncryptedIv());
         response.setFieldInt16(NXCPCodes.VID_CIPHER, encryptionContext.getCipher());
         response.setFieldInt16(NXCPCodes.VID_KEY_LENGTH, encryptionContext.getKeyLength());
         response.setFieldInt16(NXCPCodes.VID_IV_LENGTH, encryptionContext.getIvLength());
         response.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
         logger.debug("Cipher selected: " + EncryptionContext.getCipherName(encryptionContext.getCipher()));
         logger.debug("Server key fingerprint: " + encryptionContext.getServerKeyFingerprint());
      }
      catch(Exception e)
      {
         logger.error("Failed to create encryption context", e);
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
               if (syncObject.tryAcquire(actualTimeout, TimeUnit.MILLISECONDS))
               {
                  success = true;
                  syncObject.release();
                  break;
               }
            }
            catch(InterruptedException e)
            {
            }
            actualTimeout -= System.currentTimeMillis() - startTime;
         }
         if (!success)
            throw new NXCException(RCC.TIMEOUT);
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
    * @param listener The ServerConsoleListener to add
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
    * @param listener The ServerConsoleListener to remove
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
    * @param messageCode The message code
    * @param messageId   The message ID
    * @param handler     The message handler
    */
   public void addMessageSubscription(int messageCode, long messageId, MessageHandler handler)
   {
      synchronized(messageSubscriptions)
      {
         messageSubscriptions.put(new MessageSubscription(messageCode, messageId), handler);
      }
   }

   /**
    * Remove message subscription
    *
    * @param messageCode The message code
    * @param messageId   The message ID
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
         logger.warn("Notification processing queue is full");
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
      if (socket == null)
      {
         throw new IllegalStateException("Session is not connected");
      }
      final OutputStream outputStream = socket.getOutputStream();
      byte[] message;
      if ((encryptionContext != null) && !msg.isEncryptionDisabled())
      {
         try
         {
            message = encryptionContext.encryptMessage(msg, allowCompression);
         }
         catch(GeneralSecurityException e)
         {
            throw new NXCException(RCC.ENCRYPTION_ERROR);
         }
      }
      else
      {
         message = msg.createNXCPMessage(allowCompression);
      }
      outputStream.write(message);
   }

   /**
    * Send "abort file transfer" message
    *
    * @param requestId request ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if operation was timed out
    */
   private void abortFileTransfer(long requestId) throws IOException, NXCException
   {
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_ABORT_FILE_TRANSFER, requestId);
      msg.setBinaryMessage(true);
      msg.setBinaryData(new byte[0]);
      sendMessage(msg);
   }

   /**
    * Send file over NXCP
    *
    * @param requestId              request ID
    * @param file                   source file to be sent
    * @param listener               progress listener
    * @param allowStreamCompression true if data stream compression is allowed
    * @param offset offset to start data send or 0 if from the start
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   protected void sendFile(final long requestId, final File file, ProgressListener listener, boolean allowStreamCompression, long offset)
         throws IOException, NXCException
   {
      if (listener != null)
         listener.setTotalWorkAmount(file.length());
      
      InputStream inputStream;
      try
      {
         inputStream = new BufferedInputStream(new FileInputStream(file));
      }
      catch(IOException e)
      {
         abortFileTransfer(requestId);
         throw e;
      }
      sendFileStream(requestId, inputStream, listener, allowStreamCompression && (file.length() > 1024), offset);
      inputStream.close();
   }

   /**
    * Send block of data as binary message
    *
    * @param requestId              request ID
    * @param data                   file data
    * @param listener               progress listener
    * @param allowStreamCompression true if data stream compression is allowed
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   protected void sendFile(final long requestId, final byte[] data, ProgressListener listener, boolean allowStreamCompression)
         throws IOException, NXCException
   {
      if (listener != null)
         listener.setTotalWorkAmount(data.length);
      final InputStream inputStream = new ByteArrayInputStream(data);
      sendFileStream(requestId, inputStream, listener, allowStreamCompression, 0);
      inputStream.close();
   }

   /**
    * Send binary message, data loaded from provided input stream and splitted
    * into chunks of {@value FILE_BUFFER_SIZE} bytes
    *
    * @param requestId              request ID
    * @param inputStream            data input stream
    * @param listener               progress listener
    * @param allowStreamCompression true if data stream compression is allowed
    * @param offset offset to start data send or 0 if from the start
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private void sendFileStream(final long requestId, final InputStream inputStream, ProgressListener listener,
         boolean allowStreamCompression, long offset) throws IOException, NXCException
   {
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_FILE_DATA, requestId);
      msg.setBinaryMessage(true);
      inputStream.skip(offset);

      Deflater compressor = allowStreamCompression ? new Deflater(9) : null;
      msg.setStream(true, allowStreamCompression);

      final byte[] buffer = new byte[FILE_BUFFER_SIZE];
      try
      {
         long bytesSent = 0;
         while(true)
         {
            final int bytesRead = inputStream.read(buffer);
            if (bytesRead < FILE_BUFFER_SIZE)
            {
               msg.setEndOfFile(true);
            }

            if ((compressor != null) && (bytesRead > 0))
            {
               byte[] compressedData = new byte[compressor.deflateBound(bytesRead) + 4];
               compressor.setInput(buffer, 0, bytesRead, false);
               compressor.setOutput(compressedData, 4, compressedData.length - 4);
               if (compressor.deflate(JZlib.Z_SYNC_FLUSH) != JZlib.Z_OK)
                  throw new NXCException(RCC.IO_ERROR);
               int length = compressedData.length - compressor.getAvailOut();
               byte[] payload = Arrays.copyOf(compressedData, length);
               payload[0] = 2;   // DEFLATE method
               payload[1] = 0;   // reserved
               payload[2] = (byte)((bytesRead >> 8) & 0xFF);   // uncompressed length, high bits
               payload[3] = (byte)(bytesRead & 0xFF);   // uncompressed length, low bits
               msg.setBinaryData(payload);
            }
            else
            {
               msg.setBinaryData((bytesRead > 0) ? Arrays.copyOf(buffer, bytesRead) : new byte[0]);
            }
            sendMessage(msg);

            bytesSent += (bytesRead == -1) ? 0 : bytesRead;
            if (listener != null)
               listener.markProgress(bytesSent);

            if (bytesRead < FILE_BUFFER_SIZE)
               break;
         }
         if (compressor != null)
            compressor.deflateEnd();
      }
      catch(Exception e)
      {
         abortFileTransfer(requestId);
         throw e;
      }
   }

   /**
    * Wait for message with specific code and id.
    *
    * @param code    Message code
    * @param id      Message id
    * @param timeout Wait timeout in milliseconds
    * @return Message object
    * @throws NXCException if message was not arrived within timeout interval
    */
   public NXCPMessage waitForMessage(final int code, final long id, final int timeout) throws NXCException
   {
      final NXCPMessage msg = msgWaitQueue.waitForMessage(code, id, timeout);
      if (msg == null)
      {
         throw (receiverStopCause != null) ? new NXCException(RCC.COMM_FAILURE, receiverStopCause) : new NXCException(RCC.TIMEOUT);
      }
      return msg;
   }

   /**
    * Wait for message with specific code and id.
    *
    * @param code Message code
    * @param id   Message id
    * @return Message object
    * @throws NXCException if message was not arrived within timeout interval
    */
   public NXCPMessage waitForMessage(final int code, final long id) throws NXCException
   {
      return waitForMessage(code, id, msgWaitQueue.getDefaultTimeout());
   }

   /**
    * Wait for CMD_REQUEST_COMPLETED message with given id using default timeout
    *
    * @param id Message id
    * @return received message
    * @throws NXCException if message was not arrived within timeout interval or contains RCC other than RCC.SUCCESS
    */
   public NXCPMessage waitForRCC(final long id) throws NXCException
   {
      return waitForRCC(id, msgWaitQueue.getDefaultTimeout());
   }

   /**
    * Wait for CMD_REQUEST_COMPLETED message with given id
    *
    * @param id      Message id
    * @param timeout Timeout in milliseconds
    * @return received message
    * @throws NXCException if message was not arrived within timeout interval or contains RCC other than RCC.SUCCESS
    */
   public NXCPMessage waitForRCC(final long id, final int timeout) throws NXCException
   {
      final NXCPMessage msg = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, id, timeout);
      final int rcc = msg.getFieldAsInt32(NXCPCodes.VID_RCC);
      if (rcc != RCC.SUCCESS && rcc != RCC.FILE_APPEND_POSSIBLE)
      {
         long[] relatedObjects = msg.getFieldAsUInt32Array(NXCPCodes.VID_OBJECT_LIST);
         String description;
         if ((rcc == RCC.COMPONENT_LOCKED) && (msg.findField(NXCPCodes.VID_LOCKED_BY) != null))
         {
            description = msg.getFieldAsString(NXCPCodes.VID_LOCKED_BY);
         }
         else if (msg.findField(NXCPCodes.VID_ERROR_TEXT) != null)
         {
            description = msg.getFieldAsString(NXCPCodes.VID_ERROR_TEXT);
         }
         else if (msg.findField(NXCPCodes.VID_VALUE) != null)
         {
            description = msg.getFieldAsString(NXCPCodes.VID_VALUE);
         }
         else
         {
            description = null;
         }
         throw new NXCException(rcc, description, relatedObjects);
      }
      return msg;
   }

   /**
    * Create new NXCP message with unique id
    *
    * @param code Message code
    * @return New message object
    */
   public final NXCPMessage newMessage(int code)
   {
      return new NXCPMessage(code, requestId.getAndIncrement());
   }

   /**
    * Wait for specific file to arrive
    *
    * @param id Message ID
    * @param timeout Timeout (since arrival of last received file part) in milliseconds to consider file transfer as timed out
    * @return Received file or null in case of failure
    */
   public ReceivedFile waitForFile(final long id, final int timeout)
   {
      File file = null;
      int status = ReceivedFile.FAILED;
      long timestamp = System.currentTimeMillis();

      while(true)
      {
         synchronized(receivedFiles)
         {
            NXCReceivedFile rf = receivedFiles.get(id);
            if (rf != null)
            {
               if (rf.getStatus() != NXCReceivedFile.OPEN)
               {
                  if (rf.getStatus() == NXCReceivedFile.RECEIVED)
                  {
                     file = rf.getFile();
                     status = ReceivedFile.SUCCESS;
                  }
                  break;
               }
               timestamp = rf.getTimestamp();
            }

            if (System.currentTimeMillis() - timestamp > timeout)
            {
               status = ReceivedFile.TIMEOUT;
               break;
            }

            try
            {
               receivedFiles.wait(timeout);
            }
            catch(InterruptedException e)
            {
            }
         }
      }
      return new ReceivedFile(file, status);
   }

   /**
    * Wait for update from specific file monitor.
    *
    * @param monitorId file monitor ID (previously returned by <code>downloadFileFromAgent</code>
    * @param timeout Wait timeout in milliseconds
    * @return Received tail string or null in case of failure
    */
   public String waitForFileUpdate(UUID monitorId, final int timeout)
   {
      int timeRemaining = timeout;
      String data = null;

      while(timeRemaining > 0)
      {
         synchronized(receivedFileUpdates)
         {
            data = receivedFileUpdates.get(monitorId);
            if (data != null)
            {
               receivedFileUpdates.remove(monitorId);
               break;
            }

            long startTime = System.currentTimeMillis();
            try
            {
               receivedFileUpdates.wait(timeRemaining);
            }
            catch(InterruptedException e)
            {
            }
            timeRemaining -= System.currentTimeMillis() - startTime;
         }
      }
      return data;
   }

   /**
    * Execute simple commands (without arguments and only returning RCC)
    *
    * @param command Command code
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
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
    * @throws IOException           if socket I/O error occurs
    * @throws UnknownHostException  if the host is unknown
    * @throws NXCException          if NetXMS server returns an error or operation was timed out
    * @throws IllegalStateException if the state is illegal
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
    * @param componentVersions The versions of the components
    * @throws IOException           if socket I/O error occurs
    * @throws UnknownHostException  if the host is unknown
    * @throws NXCException          if NetXMS server returns an error or operation was timed out
    * @throws IllegalStateException if the state is illegal
    */
   public void connect(int[] componentVersions) throws IOException, UnknownHostException, NXCException, IllegalStateException
   {
      if (connected)
         throw new IllegalStateException("Session already connected");

      if (disconnected)
         throw new IllegalStateException("Session already disconnected and cannot be reused");

      encryptionContext = null;
      logger.info("Connecting to " + connAddress + ":" + connPort);
      try
      {
         socket = new Socket();
         socket.connect(new InetSocketAddress(connAddress, connPort), connectTimeout);
         msgWaitQueue = new NXCPMsgWaitQueue(commandTimeout);
         recvThread = new ReceiverThread();
         housekeeperThread = new HousekeeperThread();
         new NotificationProcessor();
         new BackgroundUserSync();

         // get server information
         logger.debug("Connection established, retrieving server info");
         NXCPMessage request = newMessage(NXCPCodes.CMD_GET_SERVER_INFO);
         sendMessage(request);
         NXCPMessage response = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());

         protocolVersion = new ProtocolVersion(response);
         if (!ignoreProtocolVersion)
         {
            if (!protocolVersion.isCorrectVersion(ProtocolVersion.INDEX_BASE) || ((componentVersions != null)
                  && !validateProtocolVersions(componentVersions)))
            {
               logger.warn("Connection failed (" + protocolVersion.toString() + ")");
               throw new NXCException(RCC.BAD_PROTOCOL, protocolVersion.toString());
            }
         }
         else
         {
            logger.info("Protocol version ignored");
         }

         serverVersion = response.getFieldAsString(NXCPCodes.VID_SERVER_VERSION);
         serverBuild = response.getFieldAsString(NXCPCodes.VID_SERVER_BUILD);
         serverId = response.getFieldAsInt64(NXCPCodes.VID_SERVER_ID);
         serverTimeZone = response.getFieldAsString(NXCPCodes.VID_TIMEZONE);
         serverTime = response.getFieldAsInt64(NXCPCodes.VID_TIMESTAMP) * 1000;
         serverTimeRecvTime = System.currentTimeMillis();
         serverChallenge = response.getFieldAsBinary(NXCPCodes.VID_CHALLENGE);

         tileServerURL = response.getFieldAsString(NXCPCodes.VID_TILE_SERVER_URL);
         if (tileServerURL == null)
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

         int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_COMPONENTS);
         long fieldId = NXCPCodes.VID_COMPONENT_LIST_BASE;
         for(int i = 0; i < count; i++)
            serverComponents.add(response.getFieldAsString(fieldId++));

         // Setup encryption
         request = newMessage(NXCPCodes.CMD_REQUEST_ENCRYPTION);
         request.setFieldInt16(NXCPCodes.VID_USE_X509_KEY_FORMAT, 1);
         sendMessage(request);
         waitForRCC(request.getMessageId());

         logger.info("Connected to server version " + serverVersion + " (build " + serverBuild + ")");
         logger.info("Server time zone is " + serverTimeZone);
         connected = true;
      }
      catch(UnknownHostException e)
      {
         throw new NXCException(RCC.CANNOT_RESOLVE_HOSTNAME, e.getMessage(), null, e);
      }
      finally
      {
         if (!connected)
            disconnect(SessionNotification.USER_DISCONNECT);
      }
   }

   /**
    * Login to server using login name and password.
    *
    * @param login    login name
    * @param password password
    * @throws IOException           if socket I/O error occurs
    * @throws NXCException          if NetXMS server returns an error or operation was timed out
    * @throws IllegalStateException if the state is illegal
    */
   public void login(String login, String password) throws NXCException, IOException, IllegalStateException
   {
      login(AuthenticationType.PASSWORD, login, password, null, null, null);
   }

   /**
    * Login to server using authentication token.
    *
    * @param token authentication token
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    * @throws IllegalStateException if the state is illegal
    */
   public void login(String token) throws NXCException, IOException, IllegalStateException
   {
      login(AuthenticationType.TOKEN, token, null, null, null, null);
   }

   /**
    * Login to server using certificate.
    *
    * @param login login name
    * @param certificate user's certificate
    * @param signature user's digital signature
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    * @throws IllegalStateException if the state is illegal
    */
   public void login(String login, Certificate certificate, Signature signature) throws NXCException, IOException, IllegalStateException
   {
      login(AuthenticationType.CERTIFICATE, login, null, certificate, signature, null);
   }

   /**
    * Login to server.
    *
    * @param authType authentication type
    * @param login login name
    * @param password password
    * @param certificate user's certificate
    * @param signature user's digital signature
    * @param twoFactorAuthenticationCallback callback for handling two-factor authentication if requested by server
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    * @throws IllegalStateException if the state is illegal
    */
   public void login(AuthenticationType authType, String login, String password, Certificate certificate,
		 Signature signature, TwoFactorAuthenticationCallback twoFactorAuthenticationCallback)
		 throws NXCException, IOException, IllegalStateException
   {
      if (!connected)
         throw new IllegalStateException("Session not connected");

      if (disconnected)
         throw new IllegalStateException("Session already disconnected and cannot be reused");

      authenticationMethod = authType;
      userName = login;

      NXCPMessage request = newMessage(NXCPCodes.CMD_LOGIN);
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

      if (twoFactorAuthenticationCallback != null)
      {
         byte[] token = twoFactorAuthenticationCallback.getTrustedDeviceToken(serverId, login);
         if ((token != null) && (token.length != 0))
            request.setField(NXCPCodes.VID_TRUSTED_DEVICE_TOKEN, token);
      }

      request.setField(NXCPCodes.VID_LIBNXCL_VERSION, VersionInfo.version());
      request.setField(NXCPCodes.VID_CLIENT_INFO, connClientInfo);
      request.setField(NXCPCodes.VID_OS_INFO, System.getProperty("os.name") + " " + System.getProperty("os.version"));
      request.setFieldInt16(NXCPCodes.VID_CLIENT_TYPE, clientType);
      if (clientAddress != null)
      {
         logger.debug("Client address provided: " + clientAddress);
         request.setField(NXCPCodes.VID_CLIENT_ADDRESS, clientAddress);
      }
      if (clientLanguage != null)
         request.setField(NXCPCodes.VID_LANGUAGE, clientLanguage);
      request.setField(NXCPCodes.VID_ENABLE_COMPRESSION, enableCompression);
      sendMessage(request);

      NXCPMessage response = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());
      int rcc = response.getFieldAsInt32(NXCPCodes.VID_RCC);
      logger.debug("CMD_LOGIN_RESP received, RCC=" + rcc);

      if ((rcc == RCC.NEED_2FA) && (twoFactorAuthenticationCallback != null))
      {
         logger.info("Two factor authentication requested by server");

         List<String> methods = response.getStringListFromFields(NXCPCodes.VID_2FA_METHOD_LIST_BASE, NXCPCodes.VID_2FA_METHOD_COUNT);
         boolean trustedDevicesAllowed = response.getFieldAsBoolean(NXCPCodes.VID_TRUSTED_DEVICES_ALLOWED);
         int selectedMethod = twoFactorAuthenticationCallback.selectMethod(methods);
         if (selectedMethod < 0)
         {
            logger.debug("2FA method selection cancelled by user");
            throw new NXCException(RCC.OPERATION_CANCELLED);
         }
         logger.debug("Selected 2FA method " + selectedMethod);

         if ((selectedMethod >= 0) && (selectedMethod < methods.size()))
         {
            request = newMessage(NXCPCodes.CMD_2FA_PREPARE_CHALLENGE);
            request.setField(NXCPCodes.VID_2FA_METHOD, methods.get(selectedMethod));
            sendMessage(request);

            response = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());
            rcc = response.getFieldAsInt32(NXCPCodes.VID_RCC);
            logger.debug("Two factor challenge preparation completed, RCC=" + rcc);

            if (rcc == RCC.SUCCESS)
            {
               String challenge = response.getFieldAsString(NXCPCodes.VID_CHALLENGE);
               String qrLabel = response.getFieldAsString(NXCPCodes.VID_QR_LABEL);
               String userResponse = twoFactorAuthenticationCallback.getUserResponse(challenge, qrLabel, trustedDevicesAllowed);
               if (userResponse == null)
               {
                  logger.debug("2FA response read cancelled by user");
                  throw new NXCException(RCC.OPERATION_CANCELLED);
               }

               request = newMessage(NXCPCodes.CMD_2FA_VALIDATE_RESPONSE);
               request.setField(NXCPCodes.VID_2FA_RESPONSE, userResponse);
               sendMessage(request);

               response = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());
               rcc = response.getFieldAsInt32(NXCPCodes.VID_RCC);
               logger.debug("Two factor validation response received, RCC=" + rcc);

               if ((rcc == RCC.SUCCESS) && trustedDevicesAllowed && response.isFieldPresent(NXCPCodes.VID_TRUSTED_DEVICE_TOKEN))
               {
                  byte[] token = response.getFieldAsBinary(NXCPCodes.VID_TRUSTED_DEVICE_TOKEN);
                  if ((token != null) && (token.length > 0))
                     twoFactorAuthenticationCallback.saveTrustedDeviceToken(serverId, login, token);
               }
            }
         }
      }
      else if (rcc == RCC.VERSION_MISMATCH)
      {
         String minVersion = response.getFieldAsString(NXCPCodes.VID_VALUE);
         logger.error("Login failed, server requires minimal client version {} (this client version is {})", minVersion, VersionInfo.version());
         throw new NXCException(rcc, minVersion);
      }
      
      if (rcc != RCC.SUCCESS)
      {
         logger.error("Login failed, RCC=" + rcc);
         throw new NXCException(rcc);
      }

      userId = response.getFieldAsInt32(NXCPCodes.VID_USER_ID);
      sessionId = response.getFieldAsInt32(NXCPCodes.VID_SESSION_ID);
      authenticationToken = response.getFieldAsString(NXCPCodes.VID_AUTH_TOKEN);
      userSystemRights = response.getFieldAsInt64(NXCPCodes.VID_USER_SYS_RIGHTS);
      passwordExpired = response.getFieldAsBoolean(NXCPCodes.VID_CHANGE_PASSWD_FLAG);
      graceLogins = response.getFieldAsInt32(NXCPCodes.VID_GRACE_LOGINS);
      zoningEnabled = response.getFieldAsBoolean(NXCPCodes.VID_ZONING_ENABLED);
      helpdeskLinkActive = response.getFieldAsBoolean(NXCPCodes.VID_HELPDESK_LINK_ACTIVE);
      clientAssignedTcpProxyId = response.getFieldAsBoolean(NXCPCodes.VID_TCP_PROXY_CLIENT_CID);
      uiAccessRules = response.getFieldAsString(NXCPCodes.VID_UI_ACCESS_RULES);

      // Server may send updated user name
      if (response.isFieldPresent(NXCPCodes.VID_USER_NAME))
         userName = response.getFieldAsString(NXCPCodes.VID_USER_NAME);

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

      serverCommandOutputTimeout = response.getFieldAsInt32(NXCPCodes.VID_SERVER_COMMAND_TIMEOUT) * 1000;
      serverColor = response.getFieldAsString(NXCPCodes.VID_SERVER_COLOR);
      serverName = response.getFieldAsString(NXCPCodes.VID_SERVER_NAME);
      if ((serverName == null) || serverName.isEmpty())
         serverName = connAddress;

      objectMaintenancePredefinedPeriods = response.getFieldAsString(NXCPCodes.VID_OBJ_MAINT_PREDEF_TIMES);
      networkMapDefaultWidth = response.getFieldAsInt32(NXCPCodes.VID_NETMAP_DEFAULT_WIDTH);
      networkMapDefaultHeight = response.getFieldAsInt32(NXCPCodes.VID_NETWMAP_DEFAULT_HEIGHT);

      // compatibility with 2.2.x before 2.2.6
      int count = response.getFieldAsInt32(NXCPCodes.VID_ALARM_LIST_DISP_LIMIT);
      clientConfigurationHints.put("AlarmList.DisplayLimit", Integer.toString(count));

      count = response.getFieldAsInt32(NXCPCodes.VID_CONFIG_HINT_COUNT);
      if (count > 0)
      {
         long fieldId = NXCPCodes.VID_CONFIG_HINT_LIST_BASE;
         for(int i = 0; i < count; i++)
         {
            String key = response.getFieldAsString(fieldId++);
            String value = response.getFieldAsString(fieldId++);
            clientConfigurationHints.put(key, value);
         }
      }
      clientConfigurationHints.forEach((key, value) -> logger.info("Configuration hint: {} = {}", key, value));

      messageOfTheDay = response.getFieldAsString(NXCPCodes.VID_MESSAGE_OF_THE_DAY);

      String tags = response.getFieldAsString(NXCPCodes.VID_RESPONSIBLE_USER_TAGS);
      if (tags != null)
      {
         String[] elements = tags.split(",");
         for(String e : elements)
         {
            responsibleUserTags.add(e.trim());
         }
      }

      count = response.getFieldAsInt32(NXCPCodes.VID_LICENSE_PROBLEM_COUNT);
      if (count > 0)
      {
         licenseProblems = new LicenseProblem[count];
         long fieldId = NXCPCodes.VID_LICENSE_PROBLEM_BASE;
         for(int i = 0; i < count; i++)
         {
            licenseProblems[i] = new LicenseProblem(response, fieldId);
            fieldId++;
            logger.warn("License problem reported by server: " + licenseProblems[i].getDescription());
         }
      }

      allowCompression = response.getFieldAsBoolean(NXCPCodes.VID_ENABLE_COMPRESSION);

      ouiCache = new OUICache(this);

      logger.info("Succesfully logged in, userId=" + userId);
   }

   /**
    * Request new ephemetral authentication token for this session from server.
    *
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void requestAuthenticationToken() throws IOException, NXCException
   {
      authenticationToken = requestAuthenticationToken(false, 600, null, 0).getValue();
   }

   /**
    * Request new authentication token from server.
    *
    * @param persistent true to request persistent token
    * @param validFor token validity time in seconds
    * @param description optional token description (can be null)
    * @param userId user ID to issue token for (0 to indicate currently logged in user)
    * @return token ID for persistent tokens or 0 for ephemeral
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public AuthenticationToken requestAuthenticationToken(boolean persistent, int validFor, String description, int userId) throws IOException, NXCException
   {
      NXCPMessage request = newMessage(NXCPCodes.CMD_REQUEST_AUTH_TOKEN);
      request.setField(NXCPCodes.VID_PERSISTENT, persistent);
      request.setFieldInt32(NXCPCodes.VID_VALIDITY_TIME, validFor);
      request.setFieldInt32(NXCPCodes.VID_USER_ID, userId);
      request.setField(NXCPCodes.VID_DESCRIPTION, description);
      sendMessage(request);
      NXCPMessage response = waitForRCC(request.getMessageId());
      return new AuthenticationToken(response, NXCPCodes.VID_ELEMENT_LIST_BASE);
   }

   /**
    * Revoke persistent auithentication token.
    *
    * @param tokenId token ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void revokeAuthenticationToken(int tokenId) throws IOException, NXCException
   {
      NXCPMessage request = newMessage(NXCPCodes.CMD_REVOKE_AUTH_TOKEN);
      request.setFieldInt32(NXCPCodes.VID_TOKEN_ID, tokenId);
      sendMessage(request);
      waitForRCC(request.getMessageId());
   }

   /**
    * Get list of active authentication tokens for given user.
    * 
    * @param userId user ID to get token list for (0 to indicate currently logged in user)
    * @return list of tokens
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AuthenticationToken> getAuthenticationTokens(int userId) throws IOException, NXCException
   {
      NXCPMessage request = newMessage(NXCPCodes.CMD_GET_AUTH_TOKENS);
      request.setFieldInt32(NXCPCodes.VID_USER_ID, userId);
      sendMessage(request);
      NXCPMessage response = waitForRCC(request.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      List<AuthenticationToken> tokens = new ArrayList<>(count);
      for(int i = 0; i < count; i++)
         tokens.add(new AuthenticationToken(response, fieldId));
      return tokens;
   }

   /**
    * Disconnect session in background
    *
    * @param reason disconnect reason (appropriate session notification code)
    */
   private void backgroundDisconnect(final int reason)
   {
      Thread t = new Thread(() -> disconnect(reason), "NXCSession disconnect");
      t.setDaemon(true);
      t.start();
   }

   /**
    * Disconnect from server.
    *
    * @param reason disconnect reason (appropriate session notification code)
    */
   private synchronized void disconnect(int reason)
   {
      if (disconnected)
         return;

      logger.debug("Session disconnect requested (reason=" + reason + ")");

      disconnected = true;
      if (socket != null)
      {
         logger.debug("Closing TCP socket");
         try
         {
            socket.shutdownInput();
            socket.shutdownOutput();
         }
         catch(IOException e)
         {
         }

         try
         {
            socket.close();
         }
         catch(IOException e)
         {
         }
      }

      // cause notification processing thread to stop
      notificationQueue.clear();
      if (reason != SessionNotification.USER_DISCONNECT)
         notificationQueue.offer(new SessionNotification(reason));
      notificationQueue.offer(new SessionNotification(SessionNotification.STOP_PROCESSING_THREAD));

      // cause user sync thrad to stop
      synchronized(userSyncList)
      {
         userSyncList.notifyAll();
      }

      if (recvThread != null)
      {
         logger.debug("Waiting for receiver thread shutdown");
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
         logger.debug("Waiting for housekeepeer thread shutdown");
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

      if (msgWaitQueue != null)
      {
         logger.debug("Shutdown message wait queue");
         msgWaitQueue.shutdown();
         msgWaitQueue = null;
      }

      connected = false;
      socket = null;

      listeners.clear();
      consoleListeners.clear();
      messageSubscriptions.clear();
      receivedFiles.clear();
      receivedFileUpdates.clear();
      objectList.clear();
      objectListGUID.clear();
      partialObjectList.clear();
      zoneList.clear();
      eventTemplates.clear();
      userDatabase.clear();
      userDatabaseGUID.clear();
      alarmCategories.clear();
      tcpProxies.clear();

      if (ouiCache != null)
         ouiCache.dispose();

      logger.debug("Session disconnect completed");
   }

   /**
    * Disconnect from server.
    */
   public void disconnect()
   {
      disconnect(SessionNotification.USER_DISCONNECT);
   }

   /**
    * Disconnect from server because of user inactivity. This method differs from <code>disconnect</code> in that it will send
    * appropriate notification to session listeners.
    */
   public void disconnectInactiveSession()
   {
      disconnect(SessionNotification.INACTIVITY_TIMEOUT);
   }

   /**
    * Reconnect session in background
    */
   private synchronized void backgroundReconnect()
   {
      if (reconnectThread == null)
      {
         logger.info("Attempting to reconnect after communication failure");
         reconnectThread = new Thread(() -> reconnect(), "NXCSession reconnect");
         reconnectThread.setDaemon(true);
         reconnectThread.start();
      }
   }

   /**
    * Do reconnect
    */
   private void reconnect()
   {
      sendNotification(new SessionNotification(SessionNotification.RECONNECT_STARTED));
      int retries = 10;
      while(retries-- > 0)
      {
         try
         {
            if (socket != null)
               socket.close();

            if (recvThread != null)
            {
               recvThread.join();
               recvThread = null;
            }

            encryptionContext = null;
            allowCompression = false;

            logger.debug("Connecting to " + connAddress + ":" + connPort);
            socket = new Socket();
            socket.connect(new InetSocketAddress(connAddress, connPort), connectTimeout);

            recvThread = new ReceiverThread();

            // Setup encryption
            NXCPMessage request = newMessage(NXCPCodes.CMD_REQUEST_ENCRYPTION);
            request.setFieldInt16(NXCPCodes.VID_USE_X509_KEY_FORMAT, 1);
            sendMessage(request);
            waitForRCC(request.getMessageId());

            logger.debug("Using token " + authenticationToken);
            login(authenticationToken);
            logger.debug("Reconnect completed");

            // Restore subscriptions
            for(Entry<String, Integer> e : serverChannelSubscriptions.entrySet())
            {
               NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_CHANGE_SUBSCRIPTION);
               msg.setField(NXCPCodes.VID_NAME, e.getKey());
               msg.setFieldInt16(NXCPCodes.VID_OPERATION, 1);
               for(int i = 0; i < e.getValue(); i++)
               {
                  msg.setMessageId(requestId.getAndIncrement());
                  sendMessage(msg);
                  waitForRCC(msg.getMessageId());
               }
            }

            synchronized(this)
            {
               reconnectThread = null;
               notifyAll();
               sendNotification(new SessionNotification(SessionNotification.RECONNECT_COMPLETED));
            }
            return;
         }
         catch(Exception e)
         {
            logger.debug("Reconnect failed", e);
            String message;
            if (e instanceof UnknownHostException)
               message = RCC.getText(RCC.CANNOT_RESOLVE_HOSTNAME, Locale.getDefault().getLanguage(), e.getMessage());
            else
               message = e.getLocalizedMessage();
            sendNotification(new SessionNotification(SessionNotification.RECONNECT_ATTEMPT_FAILED, message));
            if (socket != null)
            {
               try
               {
                  socket.close();
               }
               catch(IOException eio)
               {
               }
               socket = null;
            }
         }

         try
         {
            Thread.sleep(15000);
         }
         catch(InterruptedException e)
         {
         }
      }

      synchronized(this)
      {
         notifyAll();
      }
      backgroundDisconnect(SessionNotification.CONNECTION_BROKEN);
   }

   /**
    * Check if background reconnect is enabled.
    *
    * @return true if background reconnect is enabled
    */
   public boolean isReconnectEnabled()
   {
      return reconnectEnabled;
   }

   /**
    * Enable or disable background reconnect.
    *
    * @param enable true to enable background session reconnect
    */
   public void enableReconnect(boolean enable)
   {
      this.reconnectEnabled = enable;
   }

   /**
    * Get connection state
    *
    * @return connection state
    */
   public boolean isConnected()
   {
      return connected && !disconnected;
   }

   /**
    * Get the state of protocol version check
    *
    * @return true if protocol version should not be checked
    */
   public boolean isIgnoreProtocolVersion()
   {
      return ignoreProtocolVersion;
   }

   /**
    * If set to true, protocol version is not checked at connect.
    *
    * @param ignoreProtocolVersion true if protocol version should not be checked
    */
   public void setIgnoreProtocolVersion(boolean ignoreProtocolVersion)
   {
      this.ignoreProtocolVersion = ignoreProtocolVersion;
   }

   /**
    * Validate protocol versions
    *
    * @param versions the protocol versions
    * @return true if protocol versions are valid
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
    * @param maxBufferSize     max size of receiver buffer in bytes.
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
    * Get NetXMS server build information.
    *
    * @return Server version
    */
   public String getServerBuild()
   {
      return serverBuild;
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
    * Get server name
    *
    * @return the serverName
    */
   public String getServerName()
   {
      return serverName;
   }

   /**
    * Get server identification colour
    *
    * @return the serverColor
    */
   public String getServerColor()
   {
      return serverColor;
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
    * @return the objectMaintenancePredefinedPeriods
    */
   public String getObjectMaintenancePredefinedPeriods()
   {
      return objectMaintenancePredefinedPeriods;
   }

   /**
    * @return the serverChallenge
    */
   public byte[] getServerChallenge()
   {
      return serverChallenge;
   }

   /**
    * Check if server component with given id is registered
    *
    * @param componentId The component ID
    * @return true if server component is registered
    */
   public boolean isServerComponentRegistered(String componentId)
   {
      return serverComponents.contains(componentId);
   }

   /**
    * Get list of registered server components
    *
    * @return Array of registered server components
    */
   public String[] getRegisteredServerComponents()
   {
      return serverComponents.toArray(new String[serverComponents.size()]);
   }

   /**
    * Get server URL
    *
    * @return the tileServerURL
    */
   public String getTileServerURL()
   {
      return tileServerURL;
   }

   /**
    * Get the state of zoning
    *
    * @return true if zoning is enabled
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
    * @return The client information
    */
   public String getClientInfo()
   {
      return connClientInfo;
   }

   /**
    * Set client information string
    *
    * @param connClientInfo The client info to set
    */
   public void setClientInfo(final String connClientInfo)
   {
      this.connClientInfo = connClientInfo;
   }

   /**
    * Set command execution timeout.
    *
    * @param commandTimeout New command timeout
    */
   public void setCommandTimeout(final int commandTimeout)
   {
      this.commandTimeout = commandTimeout;
   }

   /**
    * Set connect call timeout (must be set before connect call)
    *
    * @param connectTimeout connect timeout in milliseconds
    */
   public void setConnectTimeout(int connectTimeout)
   {
      this.connectTimeout = connectTimeout;
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
    * Get the current user name
    *
    * @return the userName
    */
   public String getUserName()
   {
      return userName;
   }

   /**
    * Get the current authentication method
    *
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
    * Get user interface access rules.
    *
    * @return user interface access rules
    */
   public String getUIAccessRules()
   {
      return uiAccessRules;
   }

   /**
    * Get message of the day if server config is set
    *
    * @return Message of the day
    */
   public String getMessageOfTheDay()
   {
      return messageOfTheDay;
   }
   
   /**
    * Get list of license problems reported by server. This method will always return null for community edition server.
    *
    * @return list of license problems reported by server or null if there are none
    */
   public LicenseProblem[] getLicenseProblems()
   {
      return licenseProblems;
   }

   /**
    * Check if server has any license problems.
    *
    * @return true if server has any license problems
    */
   public boolean hasLicenseProblems()
   {
      return (licenseProblems != null) && (licenseProblems.length > 0);
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
    * Get number of remaining grace logins
    *
    * @return number of remaining grace logins
    */
   public int getGraceLogins()
   {
      return graceLogins;
   }

   /**
    * Get maximum number of records allowed to be displayed in alarm list
    *
    * @return The limit of alarms displayed
    */
   public int getAlarmListDisplayLimit()
   {
      return getClientConfigurationHintAsInt("AlarmList.DisplayLimit", 4096);
   }

   /**
    * Get client configuration hint as string
    *
    * @param name         hint name
    * @param defaultValue default value (returned if given hint was not provided by server)
    * @return hint value as provided by server or default value
    */
   public String getClientConfigurationHint(String name, String defaultValue)
   {
      String v = clientConfigurationHints.get(name);
      return (v != null) ? v : defaultValue;
   }

   /**
    * Get client configuration hint as string
    *
    * @param name hint name
    * @return hint value as provided by server or null
    */
   public String getClientConfigurationHint(String name)
   {
      return getClientConfigurationHint(name, null);
   }

   /**
    * Get client configuration hint as integer
    *
    * @param name         hint name
    * @param defaultValue default value (returned if given hint was not provided by server or is not valid integer)
    * @return hint value as provided by server or default value
    */
   public int getClientConfigurationHintAsInt(String name, int defaultValue)
   {
      String v = clientConfigurationHints.get(name);
      if (v == null)
         return defaultValue;
      try
      {
         return Integer.parseInt(v);
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }

   /**
    * Get client configuration hint as boolean
    *
    * @param name         hint name
    * @param defaultValue default value (returned if given hint was not provided by server or is not valid boolean)
    * @return hint value as provided by server or default value
    */
   public boolean getClientConfigurationHintAsBoolean(String name, boolean defaultValue)
   {
      String v = clientConfigurationHints.get(name);
      if (v == null)
         return defaultValue;
      if (v.equalsIgnoreCase("true"))
         return true;
      if (v.equalsIgnoreCase("false"))
         return false;
      try
      {
         return Integer.parseInt(v) != 0;
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }

   /**
    * Get allowed tags for responsible users.
    *
    * @return allowed tags for responsible users
    */
   public Set<String> getResponsibleUserTags()
   {
      return new HashSet<String>(responsibleUserTags);
   }

   /**
    * Synchronize object categories
    *
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncObjectCategories() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_OBJECT_CATEGORIES);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      synchronized(objectCategories)
      {
         objectCategories.clear();
         int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
         long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
         for(int i = 0; i < count; i++, fieldId += 10)
         {
            ObjectCategory category = new ObjectCategory(response, fieldId);
            objectCategories.put(category.getId(), category);
         }
      }
   }

   /**
    * Get object category by ID
    *
    * @param categoryId object category ID
    * @return object category or null
    */
   public ObjectCategory getObjectCategory(int categoryId)
   {
      synchronized(objectCategories)
      {
         return objectCategories.get(categoryId);
      }
   }

   /**
    * Get all object categories.
    *
    * @return all object categories
    */
   public List<ObjectCategory> getObjectCategories()
   {
      synchronized(objectCategories)
      {
         return new ArrayList<ObjectCategory>(objectCategories.values());
      }
   }

   /**
    * Modify object category.
    *
    * @param category updated category object
    * @return ID of category object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int modifyObjectCategory(MutableObjectCategory category) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_OBJECT_CATEGORY);
      category.fillMessage(msg);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt32(NXCPCodes.VID_CATEGORY_ID);
   }

   /**
    * Delete object category. This method will fail if category is in use (set to at least one object) unless <b>forceDelete</b>
    * parameter set to <b>true</b>.
    *
    * @param categoryId category ID
    * @param forceDelete force deletion flag - if set to true category will be deleted even if in use
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteObjectCategory(int categoryId, boolean forceDelete) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_OBJECT_CATEGORY);
      msg.setFieldInt32(NXCPCodes.VID_CATEGORY_ID, categoryId);
      msg.setField(NXCPCodes.VID_FORCE_DELETE, forceDelete);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Synchronizes NetXMS objects between server and client. After successful
    * sync, subscribe client to object change notifications.
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncObjects() throws IOException, NXCException
   {
      syncObjects(true);
   }

   /**
    * Synchronizes NetXMS objects between server and client. After successful
    * sync, subscribe client to object change notifications.
    *
    * @param syncNodeComponents defines if node components should be synced
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncObjects(boolean syncNodeComponents) throws IOException, NXCException
   {
      syncObjectCategories();

      syncObjects.acquireUninterruptibly();

      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_OBJECTS);
      msg.setField(NXCPCodes.VID_SYNC_NODE_COMPONENTS, syncNodeComponents);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      waitForSync(syncObjects, commandTimeout * 10);
      objectsSynchronized = objectsSynchronized || syncNodeComponents;
      sendNotification(new SessionNotification(SessionNotification.OBJECT_SYNC_COMPLETED));
      subscribe(CHANNEL_OBJECTS);
   }

   /**
    * Synchronizes selected object set with the server.
    *
    * @param objects      identifiers of objects need to be synchronized
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncObjectSet(long[] objects) throws IOException, NXCException
   {
      syncObjectSet(objects, 0, 0);
   }

   /**
    * Synchronizes selected object set with the server. The following options are accepted:<br>
    * <code>OBJECT_SYNC_NOTIFY</code> - send object update notification for each received object<br>
    * <code>OBJECT_SYNC_WAIT</code> - wait until all requested objects received<br>
    *
    * @param objects identifiers of objects need to be synchronized
    * @param options sync options (see above)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncObjectSet(long[] objects, int options) throws IOException, NXCException
   {
      syncObjectSet(objects, 0, options);
   }

   /**
    * Synchronizes selected object set with the server. The following options are accepted:<br>
    * <code>OBJECT_SYNC_NOTIFY</code> - send object update notification for each received object<br>
    * <code>OBJECT_SYNC_WAIT</code> - wait until all requested objects received<br>
    * <code>OBJECT_SYNC_ALLOW_PARTIAL</code> - allow reading partial object information (map ID should be set to get partial access to objects)
    *
    * @param objects identifiers of objects need to be synchronized
    * @param mapId ID of map object to be used as reference if OBJECT_SYNC_ALLOW_PARTIAL is set
    * @param options sync options (see above)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncObjectSet(long[] objects, long mapId, int options) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SELECTED_OBJECTS);
      msg.setFieldInt16(NXCPCodes.VID_FLAGS, options);
      msg.setFieldInt32(NXCPCodes.VID_MAP_ID, (int)mapId);
      msg.setFieldInt32(NXCPCodes.VID_NUM_OBJECTS, ((long[])objects).length);
      msg.setField(NXCPCodes.VID_OBJECT_LIST, ((long[])objects));
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      if ((options & OBJECT_SYNC_WAIT) != 0)
         waitForRCC(msg.getMessageId());
   }

   /**
    * Synchronizes selected object set with the server. The following options are accepted:<br>
    * <code>OBJECT_SYNC_NOTIFY</code> - send object update notification for each received object<br>
    * <code>OBJECT_SYNC_WAIT</code> - wait until all requested objects received<br>
    * <code>OBJECT_SYNC_ALLOW_PARTIAL</code> - allow reading partial object information (map ID should be set to get partial access to objects)
    *
    * @param objects identifiers of objects need to be synchronized
    * @param mapId ID of map object to be used as reference if OBJECT_SYNC_ALLOW_PARTIAL is set
    * @param options sync options (see above)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncObjectSet(Collection<Long> objects, long mapId, int options) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SELECTED_OBJECTS);
      msg.setFieldInt16(NXCPCodes.VID_FLAGS, options);
      msg.setFieldInt32(NXCPCodes.VID_MAP_ID, (int)mapId);
      msg.setFieldInt32(NXCPCodes.VID_NUM_OBJECTS, ((Collection<Long>)objects).size());
      msg.setField(NXCPCodes.VID_OBJECT_LIST, ((Collection<Long>)objects));       
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      if ((options & OBJECT_SYNC_WAIT) != 0)
         waitForRCC(msg.getMessageId());
   }

   /**
    * Synchronize only those objects from given set which are not synchronized yet.
    *
    * @param objects identifiers of objects need to be synchronized
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncMissingObjects(long[] objects) throws IOException, NXCException
   {
      syncMissingObjects(objects, 0, 0);
   }

   /**
    * Synchronize only those objects from given set which are not synchronized yet.
    * Accepts all options which are valid for syncObjectSet.
    *
    * @param objects      identifiers of objects need to be synchronized
    * @param options      sync options (see comments for syncObjectSet)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncMissingObjects(long[] objects, int options) throws IOException, NXCException
   {
      syncMissingObjects(objects, 0, options);
   }

   /**
    * Synchronize only those objects from given set which are not synchronized yet. Accepts all options which are valid for
    * syncObjectSet.
    *
    * @param objects identifiers of objects need to be synchronized
    * @param mapId ID of map object to be used as reference if OBJECT_SYNC_ALLOW_PARTIAL is set
    * @param options sync options (see comments for syncObjectSet)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncMissingObjects(long[] objects, long mapId, int options) throws IOException, NXCException
   {
      final long[] syncList = Arrays.copyOf(objects, objects.length);
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
         syncObjectSet(syncList, mapId, options);
      }
   }

   /**
    * Synchronize only those objects from given set which are not synchronized yet. Accepts all options which are valid for
    * syncObjectSet.
    *
    * @param objects identifiers of objects need to be synchronized
    * @param options sync options (see comments for syncObjectSet)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncMissingObjects(Collection<Long> objects, int options) throws IOException, NXCException
   {
      syncMissingObjects(objects, 0, options);
   }

   /**
    * Synchronize only those objects from given set which are not synchronized yet. Accepts all options which are valid for
    * syncObjectSet.
    *
    * @param objects identifiers of objects need to be synchronized
    * @param mapId ID of map object to be used as reference if OBJECT_SYNC_ALLOW_PARTIAL is set
    * @param options sync options (see comments for syncObjectSet)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncMissingObjects(Collection<Long> objects, long mapId, int options) throws IOException, NXCException
   {
      long[] syncList = new long[objects.size()];
      int count = 0;
      synchronized(objectList)
      {
         for(Long id : objects)
         {
            if (!objectList.containsKey(id))
               syncList[count++] = id;
         }
      }

      if (count > 0)
      {
         syncObjectSet((count < syncList.length) ? Arrays.copyOf(syncList, count) : syncList, mapId, options);
      }
   }

   /**
    * Sync children of given object.
    * 
    * @param object parent object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncChildren(AbstractObject object) throws NXCException, IOException
   {
      if (objectsSynchronized)
         return;

      boolean syncRequired;
      synchronized(synchronizedObjectSet)
      {
         syncRequired = !synchronizedObjectSet.contains(object.getObjectId());
      }

      if (syncRequired)
      {
         syncMissingObjects(object.getChildIdList(), NXCSession.OBJECT_SYNC_WAIT);
         synchronized(synchronizedObjectSet)
         {
            synchronizedObjectSet.add(object.getObjectId());
         }
      }
   }

   /**
    * Returns true if children are already synchronized for given object.
    * 
    * @param id object id
    * @return true if children are synchronized
    */
   public boolean areChildrenSynchronized(long id)
   {
      synchronized(synchronizedObjectSet)
      {
         return objectsSynchronized || synchronizedObjectSet.contains(id);
      }
   }
   
   /**
    * Find object by regex
    * 
    * @param regex for object name
    * @return list of matching objects
    */
   public List<AbstractObject> findObjectByRegex(String regex)
   {
      List<AbstractObject> objects = new ArrayList<AbstractObject>();
      Matcher matcher = Pattern.compile(regex).matcher("");

      synchronized(objectList)
      {
         for(AbstractObject o : objectList.values())
         {
            matcher.reset(o.getObjectName());
            if (matcher.matches())
               objects.add(o);
         }
      }

      return objects;
   }   

   /**
    * Find NetXMS object by it's identifier.
    *
    * @param id Object identifier
    * @return Object with given ID or null if object cannot be found
    */
   public AbstractObject findObjectById(final long id)
   {
      synchronized(objectList)
      {
         return objectList.get(id);
      }
   }

   /**
    * Find NetXMS object by it's identifier as full object or as partial object
    *
    * @param id Object identifier
    * @param allowPartial true to allow returning objects with partial data
    * @return Object with given ID or null if object cannot be found
    */
   public AbstractObject findObjectById(final long id, boolean allowPartial)
   {
      synchronized(objectList)
      {
         AbstractObject object = objectList.get(id);
         return ((object != null) || !allowPartial) ? object : partialObjectList.get(id);
      }
   }

   /**
    * Find NetXMS object by it's identifier or return wait an object to wait on
    *
    * @param id Object identifier
    * @return Object to wait on if object not found. If object is found it will be already set inside
    */
   public FutureObject findFutureObjectById(final long id)
   {
      final FutureObject object;
      synchronized(objectList)
      {
         AbstractObject result = objectList.get(id);
         if (result == null)
         {
            object = new FutureObject();   
            addListener(new SessionListener() {               
               @Override
               public void notificationHandler(SessionNotification n)
               {
                  if (n.code == SessionNotification.OBJECT_CHANGED && n.subCode == id)
                  {
                     synchronized(object)
                     {
                        object.setObject((AbstractObject)n.object);
                        object.notifyAll();
                     }
                     removeListener(this);
                  }
               }
            });
         }
         else
         {
            object = new FutureObject(result);
         }
      }
      return object;
   }

   /**
    * Find NetXMS object by it's identifier and execute provided callback once found
    *
    * @param id Object identifier
    * @param callback callback to be called when object is found
    */
   public void findObjectAsync(final long id, ObjectCreationListener callback)
   {
      if (callback == null)
         return;

      synchronized(objectList)
      {
         AbstractObject object = objectList.get(id);
         if (object == null)
         { 
            addListener(new SessionListener() {
               @Override
               public void notificationHandler(SessionNotification n)
               {
                  if (n.code == SessionNotification.OBJECT_CHANGED && n.subCode == id)
                  {
                     callback.objectCreated(findObjectById(id));
                     removeListener(this);
                  }
               }
            });
         }
         else
         {
            callback.objectCreated((AbstractObject)object);
         }
      }
   }

   /**
    * Find NetXMS object by it's identifier with additional class checking.
    *
    * @param id            object identifier
    * @param requiredClass required object class
    * @param <T>           Object
    * @return Object with given ID or null if object cannot be found or is not an instance of required class
    */
   @SuppressWarnings("unchecked")
   public <T> T findObjectById(final long id, final Class<T> requiredClass)
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
    * @param idList collection of object identifiers
    * @param returnUnknown if true, this method will return UnknownObject placeholders for unknown object identifiers
    * @return array of found objects
    */
   public List<AbstractObject> findMultipleObjects(final Collection<Long> idList, boolean returnUnknown)
   {
      return findMultipleObjects(idList, null, returnUnknown);
   }

   /**
    * Find multiple NetXMS objects by identifiers
    *
    * @param idList collection of object identifiers
    * @param classFilter class filter for objects, or null to disable filtering
    * @param returnUnknown if true, this method will return UnknownObject placeholders for unknown object identifiers
    * @return array of found objects
    */
   public List<AbstractObject> findMultipleObjects(final Collection<Long> idList, Class<? extends AbstractObject> classFilter, boolean returnUnknown)
   {
      List<AbstractObject> result = new ArrayList<AbstractObject>(idList.size());

      synchronized(objectList)
      {
         for(Long id : idList)
         {
            final AbstractObject object = objectList.get(id);
            if ((object != null) && ((classFilter == null) || classFilter.isInstance(object)))
            {
               result.add(object);
            }
            else if (returnUnknown)
            {
               result.add(new UnknownObject(id, this));
            }
         }
      }

      return result;
   }

   /**
    * Find NetXMS object by it's GUID.
    *
    * @param guid Object GUID
    * @return Object with given ID or null if object cannot be found
    */
   public AbstractObject findObjectByGUID(final UUID guid)
   {
      synchronized(objectList)
      {
         return objectListGUID.get(guid);
      }
   }

   /**
    * Find NetXMS object by it's GUID with additional class checking.
    *
    * @param guid          object GUID
    * @param requiredClass required object class
    * @param <T>           Object
    * @return Object with given ID or null if object cannot be found or is not an instance of required class
    */
   @SuppressWarnings("unchecked")
   public <T extends AbstractObject> T findObjectByGUID(final UUID guid, final Class<T> requiredClass)
   {
      AbstractObject object = findObjectByGUID(guid);
      return requiredClass.isInstance(object) ? (T)object : null;
   }

   /**
    * Find zone object by zone UIN (unique identification number).
    *
    * @param zoneUIN zone UIN to find
    * @return zone object or null
    */
   public Zone findZone(int zoneUIN)
   {
      synchronized(objectList)
      {
         return zoneList.get(zoneUIN);
      }
   }

   /**
    * Get all accessible zone objects
    *
    * @return list of all accessible zone objects
    */
   public List<Zone> getAllZones()
   {
      synchronized(objectList)
      {
         return new ArrayList<Zone>(zoneList.values());
      }
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
    * Find object by name with object filter. Name comparison
    * is case-insensitive.
    *
    * @param name object name to find
    * @param filter TODO
    * @return object with matching name or null
    */
   public AbstractObject findObjectByName(final String name, ObjectFilter filter)
   {
      AbstractObject result = null;
      synchronized(objectList)
      {
         for(AbstractObject object : objectList.values())
         {
            if (object.getObjectName().equalsIgnoreCase(name) && filter.accept(object))
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
    * Generic object find using filter. WIll return first object matching given filter.
    *
    * @param filter ObjectFilter to filter the result
    * @return first matching object or null
    */
   public AbstractObject findObject(ObjectFilter filter)
   {
      AbstractObject result = null;
      synchronized(objectList)
      {
         for(AbstractObject object : objectList.values())
         {
            if (filter.accept(object))
            {
               result = object;
               break;
            }
         }
      }
      return result;
   }

   /**
    * Find all objects matching given filter.
    *
    * @param filter ObjectFilter to filter the result
    * @return list of matching objects (empty list if nothing found)
    */
   public List<AbstractObject> filterObjects(ObjectFilter filter)
   {
      List<AbstractObject> result = new ArrayList<AbstractObject>();
      synchronized(objectList)
      {
         for(AbstractObject object : objectList.values())
         {
            if (filter.accept(object))
            {
               result.add(object);
            }
         }
      }
      return result;
   }

   /**
    * Get list of top-level objects matching given object filter. Filter may be null.
    *
    * @param objectFilter filter for objects
    * @return List of all matching top level objects (either without parents or with inaccessible parents)
    */
   public AbstractObject[] getTopLevelObjects(ObjectFilter objectFilter)
   {
      HashSet<AbstractObject> list = new HashSet<AbstractObject>();
      synchronized(objectList)
      {
         for(AbstractObject object : objectList.values())
         {
            if ((objectFilter != null) && !objectFilter.accept(object))
               continue;

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
                  if (objectFilter != null)
                  {
                     AbstractObject p = objectList.get(parent);
                     if ((p != null) && objectFilter.accept(p))
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
               if (!hasParents)
                  list.add(object);
            }
         }
      }
      return list.toArray(AbstractObject[]::new);
   }

   /**
    * Get list of top-level objects matching given class filter. Class filter may be null to ignore object class.
    *
    * @param classFilter To filter the classes
    * @return List of all top matching level objects (either without parents or with inaccessible parents)
    */
   public AbstractObject[] getTopLevelObjects(Set<Integer> classFilter)
   {
      if (classFilter == null)
         return getTopLevelObjects((ObjectFilter)null);
      return getTopLevelObjects((AbstractObject o) -> classFilter.contains(o.getObjectClass()));
   }

   /**
    * Get list of top-level objects.
    *
    * @return List of all top level objects (either without parents or with inaccessible parents)
    */
   public AbstractObject[] getTopLevelObjects()
   {
      return getTopLevelObjects((ObjectFilter)null);
   }

   /**
    * Get list of all objects
    *
    * @return List of all objects
    */
   public List<AbstractObject> getAllObjects()
   {
      synchronized(objectList)
      {
         return new ArrayList<AbstractObject>(objectList.values());
      }
   }

   /**
    * Get object name by ID.
    *
    * @param objectId object ID
    * @return object name if object is known, or string in form [&lt;object_id&gt;] for unknown objects
    */
   public String getObjectName(long objectId)
   {
      AbstractObject object = findObjectById(objectId, true);
      return (object != null) ? object.getObjectName() : ("[" + Long.toString(objectId) + "]");
   }

   /**
    * Get object name with alias by ID.
    *
    * @param objectId object ID
    * @return object name with alias if object is known, or string in form [&lt;object_id&gt;] for unknown objects
    */
   public String getObjectNameWithAlias(long objectId)
   {
      AbstractObject object = findObjectById(objectId, true);
      return (object != null) ? object.getNameWithAlias() : ("[" + Long.toString(objectId) + "]");
   }

   /**
    * Get zone name by UIN
    *
    * @param zoneUIN zone UIN
    * @return zone name
    */
   public String getZoneName(int zoneUIN)
   {
      Zone zone = findZone(zoneUIN);
      return (zone != null) ? zone.getObjectName() : ("[" + Long.toString(zoneUIN) + "]");
   }

   /**
    * Query objects on server side, optionally only those located below given root object. If progress callback is not null, it will
    * be called periodically during query with updated completion percentage.
    *
    * @param query query to execute
    * @param rootObjectId root object ID or 0 to query all objects
    * @param progressCallback optional progress callback
    * @return list of matching objects
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AbstractObject> queryObjects(String query, long rootObjectId, final Consumer<Integer> progressCallback) throws IOException, NXCException
   {
      NXCPMessage request = newMessage(NXCPCodes.CMD_QUERY_OBJECTS);
      request.setField(NXCPCodes.VID_QUERY, query);
      request.setFieldUInt32(NXCPCodes.VID_ROOT, rootObjectId);
      if (progressCallback != null)
      {
         request.setField(NXCPCodes.VID_REPORT_PROGRESS, true);
         addMessageSubscription(NXCPCodes.CMD_PROGRESS_REPORT, request.getMessageId(), new MessageHandler() {
            @Override
            public boolean processMessage(NXCPMessage msg)
            {
               progressCallback.accept(msg.getFieldAsInt32(NXCPCodes.VID_PROGRESS));
               return true;
            }
         });
      }
      try
      {
         sendMessage(request);
         NXCPMessage response = waitForRCC(request.getMessageId());
         return findMultipleObjects(response.getFieldAsUInt32Array(NXCPCodes.VID_OBJECT_LIST), false);
      }
      finally
      {
         removeMessageSubscription(NXCPCodes.CMD_PROGRESS_REPORT, request.getMessageId());
      }
   }

   /**
    * Query objects on server side. If progress callback is not null, it will be called periodically during query with updated
    * completion percentage.
    *
    * @param query query to execute
    * @param progressCallback optional progress callback
    * @return list of matching objects
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AbstractObject> queryObjects(String query, Consumer<Integer> progressCallback) throws IOException, NXCException
   {
      return queryObjects(query, 0L, progressCallback);
   }

   /**
    * Query objects on server side and read certain object properties. Available properties are the same as in corresponding NXSL
    * objects or computed properties set using "with" statement in query. If <code>readAllComputedProperties</code> is set to
    * <code>true</code> then all computed properties will be retrieved in addition to those explicitly listed. Set of queried
    * objects can be limited to child objects of certain object. If progress callback is not null, it will be called periodically
    * during query with updated completion percentage.
    *
    * @param query query to execute
    * @param rootObjectId ID of root object or 0 to query all objects
    * @param properties object properties to read
    * @param orderBy list of properties for ordering result set (can be null)
    * @param inputFields set of input fields provided by user (can be null)
    * @param contextObjectId ID of context object (will be available in query via global variable $context), 0 if none
    * @param readAllComputedProperties if set to <code>true</code>, query will return all computed properties in addition to
    *           properties explicitly listed in <code>properties</code> parameter
    * @param limit limit number of records (0 for unlimited)
    * @param progressCallback optional progress callback
    * @param metadata optional map to receive query metadata (can be null)
    * @return list of matching objects
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<ObjectQueryResult> queryObjectDetails(String query, long rootObjectId, List<String> properties, List<String> orderBy, Map<String, String> inputFields, long contextObjectId,
         boolean readAllComputedProperties, int limit, Consumer<Integer> progressCallback, Map<String, String> metadata) throws IOException, NXCException
   {
      NXCPMessage request = newMessage(NXCPCodes.CMD_QUERY_OBJECT_DETAILS);
      request.setField(NXCPCodes.VID_QUERY, query);
      request.setFieldUInt32(NXCPCodes.VID_ROOT, rootObjectId);
      if (properties != null)
         request.setFieldsFromStringCollection(properties, NXCPCodes.VID_FIELD_LIST_BASE, NXCPCodes.VID_FIELDS);
      if (orderBy != null)
         request.setFieldsFromStringCollection(orderBy, NXCPCodes.VID_ORDER_FIELD_LIST_BASE, NXCPCodes.VID_ORDER_FIELDS);
      if (inputFields != null)
         request.setFieldsFromStringMap(inputFields, NXCPCodes.VID_INPUT_FIELD_BASE, NXCPCodes.VID_INPUT_FIELD_COUNT);
      request.setFieldUInt32(NXCPCodes.VID_CONTEXT_OBJECT_ID, contextObjectId);
      request.setFieldInt32(NXCPCodes.VID_RECORD_LIMIT, limit);
      request.setField(NXCPCodes.VID_READ_ALL_FIELDS, readAllComputedProperties);
      if (progressCallback != null)
      {
         request.setField(NXCPCodes.VID_REPORT_PROGRESS, true);
         addMessageSubscription(NXCPCodes.CMD_PROGRESS_REPORT, request.getMessageId(), new MessageHandler() {
            @Override
            public boolean processMessage(NXCPMessage msg)
            {
               progressCallback.accept(msg.getFieldAsInt32(NXCPCodes.VID_PROGRESS));
               return true;
            }
         });
      }

      try
      {
         sendMessage(request);

         NXCPMessage response = waitForRCC(request.getMessageId());
         long[] objects = response.getFieldAsUInt32Array(NXCPCodes.VID_OBJECT_LIST);
         syncMissingObjects(objects, NXCSession.OBJECT_SYNC_WAIT);
         List<ObjectQueryResult> results = new ArrayList<ObjectQueryResult>(objects.length);
         long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
         for(int i = 0; i < objects.length; i++)
         {
            Map<String, String> values = response.getStringMapFromFields(fieldId + 1, fieldId);
            AbstractObject object = findObjectById(objects[i]);
            if (object != null)
            {
               results.add(new ObjectQueryResult(object, values));
            }
            fieldId += values.size() * 2 + 1;
         }
         if (metadata != null)
         {
            metadata.clear();
            metadata.putAll(response.getStringMapFromFields(NXCPCodes.VID_METADATA_BASE, NXCPCodes.VID_METADATA_SIZE));
         }
         return results;
      }
      finally
      {
         removeMessageSubscription(NXCPCodes.CMD_PROGRESS_REPORT, request.getMessageId());
      }
   }

   /**
    * Get list of configured object queries.
    *
    * @return list of configured object queries
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<ObjectQuery> getObjectQueries() throws IOException, NXCException
   {
      NXCPMessage request = newMessage(NXCPCodes.CMD_GET_OBJECT_QUERIES);
      sendMessage(request);

      NXCPMessage response = waitForRCC(request.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<ObjectQuery> queries = new ArrayList<ObjectQuery>(count);
      long[] fieldId = new long[] { NXCPCodes.VID_ELEMENT_LIST_BASE };
      for(int i = 0; i < count; i++)
         queries.add(new ObjectQuery(response, fieldId));
      return queries;
   }

   /**
    * Modify object query. If query ID is 0 new query object will be created and assigned ID will be returned.
    *
    * @param query query to create or modify
    * @return assigned query ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int modifyObjectQuery(ObjectQuery query) throws IOException, NXCException
   {
      NXCPMessage request = newMessage(NXCPCodes.CMD_MODIFY_OBJECT_QUERY);
      query.fillMessage(request);
      sendMessage(request);
      NXCPMessage response = waitForRCC(request.getMessageId());
      return response.getFieldAsInt32(NXCPCodes.VID_QUERY_ID);
   }

   /**
    * Delete predefined object query.
    *
    * @param queryId query ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteObjectQuery(int queryId) throws IOException, NXCException
   {
      NXCPMessage request = newMessage(NXCPCodes.CMD_DELETE_OBJECT_QUERY);
      request.setFieldInt32(NXCPCodes.VID_QUERY_ID, queryId);
      sendMessage(request);
      waitForRCC(request.getMessageId());
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
      NXCPMessage request = newMessage(NXCPCodes.CMD_GET_ALL_ALARMS);
      final long rqId = request.getMessageId();
      sendMessage(request);

      final HashMap<Long, Alarm> alarmList = new HashMap<Long, Alarm>(0);
      while(true)
      {
         request = waitForMessage(NXCPCodes.CMD_ALARM_DATA, rqId);
         long alarmId = request.getFieldAsInt32(NXCPCodes.VID_ALARM_ID);
         if (alarmId == 0)
            break; // ALARM_ID == 0 indicates end of list
         alarmList.put(alarmId, new Alarm(request));
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
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
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
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
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
    * @param time    timeout for sticky acknowledge in seconds (0 for infinite)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void acknowledgeAlarm(final long alarmId, boolean sticky, int time) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_ACK_ALARM);
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
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
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Resolve alarm by helpdesk reference.
    *
    * @param helpdeskReference Identifier of alarm to be resolved.
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
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Terminate alarm by helpdesk reference.
    *
    * @param helpdeskReference Identifier of alarm to be resolved.
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

   private Map<Long, Integer> bulkAlarmOperation(int cmd, List<Long> alarmIds) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(cmd);
      msg.setField(NXCPCodes.VID_ALARM_ID_LIST, alarmIds.toArray(new Long[alarmIds.size()]));
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      Map<Long, Integer> operationFails = new HashMap<Long, Integer>();

      // Returned alarm ID`s if there were any failed operations
      if (response.findField(NXCPCodes.VID_ALARM_ID_LIST) != null)
      {
         for(int i = 0; i < response.getFieldAsUInt32ArrayEx(NXCPCodes.VID_ALARM_ID_LIST).length; i++)
         {
            operationFails.put(response.getFieldAsUInt32ArrayEx(NXCPCodes.VID_ALARM_ID_LIST)[i],
                  (Integer)response.getFieldAsUInt32ArrayEx(NXCPCodes.VID_FAIL_CODE_LIST)[i].intValue());
         }
      }

      return operationFails;
   }

   /**
    * Bulk terminate alarms.
    *
    * @param alarmIds Identifiers of alarms to be terminated.
    * @return true if all alarms were terminated, false if some, or all, were not terminated
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<Long, Integer> bulkResolveAlarms(List<Long> alarmIds) throws IOException, NXCException
   {
      return bulkAlarmOperation(NXCPCodes.CMD_BULK_RESOLVE_ALARMS, alarmIds);
   }

   /**
    * Bulk terminate alarms.
    *
    * @param alarmIds Identifiers of alarms to be terminated.
    * @return true if all alarms were terminated, false if some, or all, were not terminated
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<Long, Integer> bulkTerminateAlarms(List<Long> alarmIds) throws IOException, NXCException
   {
      return bulkAlarmOperation(NXCPCodes.CMD_BULK_TERMINATE_ALARMS, alarmIds);
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
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
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
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
      sendMessage(msg);
      return waitForRCC(msg.getMessageId()).getFieldAsString(NXCPCodes.VID_HELPDESK_REF);
   }

   /**
    * Get URL for helpdesk issue associated with given alarm
    *
    * @param alarmId The ID of alarm
    * @return URL of helpdesk issue
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String getHelpdeskIssueUrl(long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_HELPDESK_URL);
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
      sendMessage(msg);
      return waitForRCC(msg.getMessageId()).getFieldAsString(NXCPCodes.VID_URL);
   }

   /**
    * Unlink helpdesk issue from alarm. User must have OBJECT_ACCESS_UPDATE_ALARMS access right
    * on alarm's source object and SYSTEM_ACCESS_UNLINK_ISSUES system wide access right.
    *
    * @param helpdeskReference The helpdesk reference
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
    * @param alarmId alarm id
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unlinkHelpdeskIssue(long alarmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UNLINK_HELPDESK_ISSUE);
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
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
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
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
    * @param alarmId   alarm ID
    * @param commentId comment ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAlarmComment(long alarmId, long commentId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_ALARM_COMMENT);
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
      msg.setFieldUInt32(NXCPCodes.VID_COMMENT_ID, commentId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Create alarm comment.
    *
    * @param alarmId The alarm ID
    * @param text    The comment text
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void createAlarmComment(long alarmId, String text) throws IOException, NXCException
   {
      updateAlarmComment(alarmId, 0, text);
   }

   /**
    * Create alarm comment by helpdesk reference.
    *
    * @param helpdeskReference The helpdesk reference
    * @param text              The reference text
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
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
    * @param alarmId   alarm ID
    * @param commentId comment ID or 0 for creating new comment
    * @param text      message text
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateAlarmComment(long alarmId, long commentId, String text) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_ALARM_COMMENT);
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
      msg.setFieldUInt32(NXCPCodes.VID_COMMENT_ID, commentId);
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
    * @return The server variables
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<String, ServerVariable> getServerVariables() throws IOException, NXCException
   {
      NXCPMessage request = newMessage(NXCPCodes.CMD_GET_CONFIG_VARLIST);
      sendMessage(request);

      final NXCPMessage response = waitForRCC(request.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_VARIABLES);
      final HashMap<String, ServerVariable> varList = new HashMap<String, ServerVariable>(count);

      long id = NXCPCodes.VID_VARLIST_BASE;
      for(int i = 0; i < count; i++, id += 10)
      {
         ServerVariable v = new ServerVariable(response, id);
         varList.put(v.getName(), v);
      }

      count = response.getFieldAsInt32(NXCPCodes.VID_NUM_VALUES);
      for(int i = 0; i < count; i++)
      {
         ServerVariable var = varList.get(response.getFieldAsString(id++));
         if (var != null)
            var.addPossibleValue(response, id);
         id += 2;
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
    * Get server public configuration variable as a int
    *
    * @param name configuration variable name
    * @return value of requested configuration variable
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int getPublicServerVariableAsInt(String name) throws IOException, NXCException
   {
      return Integer.parseInt(getPublicServerVariable(name));
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
    * @param name  The name of the variable
    * @param value The value of the variable
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
    * @param name The name of the variable
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
    * Set server configuration variables to default
    *
    * @param varList The list of variables
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setDefaultServerValues(List<ServerVariable> varList) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_CONFIG_TO_DEFAULT);
      long base = NXCPCodes.VID_VARLIST_BASE;
      for(ServerVariable v : varList)
         msg.setField(base++, v.getName());
      msg.setFieldInt32(NXCPCodes.VID_NUM_VARIABLES, varList.size());
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get server config CLOB
    *
    * @param name The name of the config
    * @return The config CLOB
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
    * @param name  The name to set
    * @param value The value to set
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
    * Subscribe to notification channel. Each subscribe call should be matched by unsubscribe call.
    * Calling subscribe on already subscribed channel will increase internal counter, and subscription
    * will be cancelled when this counter returns back to 0.
    *
    * @param channel Notification channel to subscribe to.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void subscribe(String channel) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_SUBSCRIPTION);
      msg.setField(NXCPCodes.VID_NAME, channel);
      msg.setFieldInt16(NXCPCodes.VID_OPERATION, 1);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      synchronized(serverChannelSubscriptions)
      {
         Integer count = serverChannelSubscriptions.get(channel);
         if (count == null)
         {
            count = Integer.valueOf(0);
         }
         serverChannelSubscriptions.put(channel, count + 1);
      }
   }

   /**
    * Unsubscribe from notification channel.
    *
    * @param channel Notification channel to unsubscribe from.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unsubscribe(String channel) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_SUBSCRIPTION);
      msg.setField(NXCPCodes.VID_NAME, channel);
      msg.setFieldInt16(NXCPCodes.VID_OPERATION, 0);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      synchronized(serverChannelSubscriptions)
      {
         Integer count = serverChannelSubscriptions.get(channel);
         if (count != null)
         {
            if (count > 1)
               serverChannelSubscriptions.put(channel, count - 1);
            else
               serverChannelSubscriptions.remove(channel);
         }
      }
   }

   /**
    * Synchronize user database and subscribe to user change notifications
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncUserDatabase() throws IOException, NXCException
   {
      syncUserDB.acquireUninterruptibly();
      NXCPMessage msg = newMessage(NXCPCodes.CMD_LOAD_USER_DB);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      waitForSync(syncUserDB, commandTimeout * 10);
      userDatabaseSynchronized = true;
      subscribe(CHANNEL_USERDB);
   }

   /**
    * Subscribe to user change notifications
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void subscribeToUserDBUpdates() throws IOException, NXCException
   {
      subscribe(CHANNEL_USERDB);
   }

   /**
    * Synchronize users by id if does not exist
    *
    * @param users list of user id's to synchronize
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncUserSet(Collection<Integer> users) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SELECTED_USERS);
      msg.setFieldInt32(NXCPCodes.VID_NUM_OBJECTS, users.size());
      msg.setField(NXCPCodes.VID_OBJECT_LIST, users.toArray(Integer[]::new));
      sendMessage(msg);

      // First request completion message will indicate successful sync start
      waitForRCC(msg.getMessageId());

      // Server will send second completion message when all user database objects were sent back
      waitForRCC(msg.getMessageId());

      // Check that each user from set was synchronized and add update missing list
      synchronized(userDatabase)
      {
         for(Integer id : users)
         {
            if (userDatabase.containsKey(id))
               missingUsers.remove(id);
            else
               missingUsers.add(id);
         }
      }
   }

   /**
    * Synchronize only objects that were not yet synchronized
    * 
    * @param users set of user IDs to sync
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    * @return true if synchronization was completed and false if synchronization is not needed
    */
   public boolean syncMissingUsers(Collection<Integer> users) throws IOException, NXCException
   {
      if (userDatabaseSynchronized)
         return false;

      final Set<Integer> syncSet = new HashSet<Integer>();
      synchronized(userDatabase)
      {
         for(Integer id : users)
         {
            if (!userDatabase.containsKey(id))
               syncSet.add(id);
         }
      }

      if (!syncSet.isEmpty())
         syncUserSet(syncSet);
      return !syncSet.isEmpty();
   }

   /**
    * Check if user database is synchronized with client
    *
    * @return true if user database is synchronized with client
    */
   public boolean isUserDatabaseSynchronized()
   {
      return userDatabaseSynchronized;
   }

   /**
    * Find multiple users by list of IDs
    *
    * @param ids of user DBObjects to find
    * @return list of found users
    */
   public List<AbstractUserObject> findUserDBObjectsByIds(final Collection<Integer> ids)
   {
      List<AbstractUserObject> users = new ArrayList<AbstractUserObject>();
      synchronized(userDatabase)
      {
         for(Integer l : ids)
         {
            AbstractUserObject user = userDatabase.get(l);
            if (user != null)
               users.add(user);
         }
      }
      return users;
   }

   /**
    * Find user or group by ID. If full user database synchronization was not done this method may return null for existing user
    * that is not yet synchronized with the client. In such case client library will initiate background synchronization of that
    * user object. If provided callback is not null it will be executed when synchronization is complete.
    *
    * @param id The user database object ID
    * @param callback synchronization completion callback (may be null)
    * @return User object with given ID or null if such user does not exist or not synchronized with client.
    */
   public AbstractUserObject findUserDBObjectById(final int id, Runnable callback)
   {
      AbstractUserObject object = null;
      boolean needSync = false;
      synchronized(userDatabase)
      {
         object = userDatabase.get(id);
         if ((object == null) && !userDatabaseSynchronized && !missingUsers.contains(id))
            needSync = true;
      }
      if (needSync)
      {
         synchronized(userSyncList)
         {
            userSyncList.add(id);
            if (callback != null)
               callbackList.add(callback);
            userSyncList.notifyAll();
         }
      }
      return object;      
   }

   /**
    * Find user or group by GUID
    *
    * @param guid The user DBObject GUID
    * @return User object with given GUID or null if such user does not exist
    */
   public AbstractUserObject findUserDBObjectByGUID(final UUID guid)
   {
      synchronized(userDatabase)
      {
         return userDatabaseGUID.get(guid);
      }
   }

   /**
    * Find user or group by name.
    *
    * @param name user database object name
    * @param classFilter optional class filter. If not null, only objects of given class will be matched.
    * @return User object with given name or null if such object does not exist
    */
   public AbstractUserObject findUserDBObjectByName(String name, Class<? extends AbstractUserObject> classFilter)
   {
      synchronized(userDatabase)
      {
         for(AbstractUserObject o : userDatabase.values())
         {
            if (o.getName().equals(name) && ((classFilter == null) || classFilter.isInstance(o)))
               return o;
         }
      }
      return null;
   }

   /**
    * Get list of all user database objects
    *
    * @return List of all user database objects
    */
   public AbstractUserObject[] getUserDatabaseObjects()
   {
      synchronized(userDatabase)
      {
         return userDatabase.values().toArray(AbstractUserObject[]::new);
      }
   }

   /**
    * Create user or group on server
    *
    * @param name Login name for new user
    * @return ID assigned to newly created user
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private int createUserDBObject(final String name, final boolean isGroup) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_USER);
      msg.setField(NXCPCodes.VID_USER_NAME, name);
      msg.setField(NXCPCodes.VID_IS_GROUP, isGroup);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt32(NXCPCodes.VID_USER_ID);
   }

   /**
    * Create user on server
    *
    * @param name Login name for new user
    * @return ID assigned to newly created user
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int createUser(final String name) throws IOException, NXCException
   {
      return createUserDBObject(name, false);
   }

   /**
    * Create user group on server
    *
    * @param name Name for new user group
    * @return ID assigned to newly created user group
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int createUserGroup(final String name) throws IOException, NXCException
   {
      return createUserDBObject(name, true);
   }

   /**
    * Delete user or group on server
    *
    * @param id User or group ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteUserDBObject(int id) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_USER);
      msg.setFieldInt32(NXCPCodes.VID_USER_ID, id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Set password for user
    *
    * @param id          User ID
    * @param newPassword New password
    * @param oldPassword Old password
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setUserPassword(int id, final String newPassword, final String oldPassword) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_PASSWORD);
      msg.setFieldInt32(NXCPCodes.VID_USER_ID, id);
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
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
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
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
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
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyUserDBObject(final AbstractUserObject object) throws IOException, NXCException
   {
      modifyUserDBObject(object, 0x7FFFFFFF);
   }

   /**
    * Detach user from LDAP
    *
    * @param userId user ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void detachUserFromLdap(int userId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DETACH_LDAP_USER);
      msg.setFieldInt32(NXCPCodes.VID_USER_ID, userId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Set custom attribute for currently logged in user. Server will allow to change
    * only attributes whose name starts with dot.
    *
    * @param name  Attribute's name
    * @param value New attribute's value
    * @throws IOException  if socket I/O error occurs
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
    * @throws IOException  if socket I/O error occurs
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
    * Find all DCIs matching given creteria.
    * 
    * @param objectId if specific object needed (set to 0 if match by object name is needed)
    * @param objectName regular expression to match object name (not used if object ID is not 0)
    * @param dciName regular expression to match DCI name
    * @param flags find flags
    * @return list of all found DCIs and their last values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<DciValue> findMatchingDCI(long objectId, String objectName, String dciName, int flags) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_MATCHING_DCI);
      
      if (objectId != 0)
         msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      else
         msg.setField(NXCPCodes.VID_OBJECT_NAME, objectName);
      msg.setField(NXCPCodes.VID_DCI_NAME, dciName);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
      sendMessage(msg);
      
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      List<DciValue> result = new ArrayList<DciValue>(count);
      
      Long base = NXCPCodes.VID_DCI_VALUES_BASE;
      for(int i = 0; i < count; i++, base += 50)
      {
         result.add(new SimpleDciValue(response, base));
      }
      
      return result;
   }

   /**
    * Get DCI summary for given node
    *
    * @param nodeId                ID of the node to get DCI values for
    * @param objectTooltipOnly     if set to true, only DCIs with DCF_SHOW_ON_OBJECT_TOOLTIP flag set are returned
    * @param overviewOnly          if set to true, only DCIs with DCF_SHOW_IN_OBJECT_OVERVIEW flag set are returned
    * @param includeNoValueObjects if set to true, objects with no value (like instance discovery DCIs) will be returned as well
    * @return List of DCI values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciValue[] getDataCollectionSummary(final long nodeId, boolean objectTooltipOnly, boolean overviewOnly,
         boolean includeNoValueObjects) throws IOException, NXCException
   {
      return getDataCollectionSummary(nodeId, 0, objectTooltipOnly, overviewOnly, includeNoValueObjects);
   }

   /**
    * Get DCI summary for given node and map
    *
    * @param nodeId                ID of the node to get DCI values for
    * @param objectTooltipOnly     if set to true, only DCIs with DCF_SHOW_ON_OBJECT_TOOLTIP flag set are returned
    * @param overviewOnly          if set to true, only DCIs with DCF_SHOW_IN_OBJECT_OVERVIEW flag set are returned
    * @param includeNoValueObjects if set to true, objects with no value (like instance discovery DCIs) will be returned as well
    * @return List of DCI values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciValue[] getDataCollectionSummary(final long nodeId, final long mapId, boolean objectTooltipOnly, boolean overviewOnly,
         boolean includeNoValueObjects) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DATA_COLLECTION_SUMMARY);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_OBJECT_TOOLTIP_ONLY, objectTooltipOnly);
      msg.setField(NXCPCodes.VID_OVERVIEW_ONLY, overviewOnly);
      msg.setField(NXCPCodes.VID_INCLUDE_NOVALUE_OBJECTS, includeNoValueObjects);
      msg.setFieldUInt32(NXCPCodes.VID_MAP_ID, mapId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      DciValue[] list = new DciValue[count];
      long base = NXCPCodes.VID_DCI_VALUES_BASE;
      for(int i = 0; i < count; i++, base += 50)
      {
         list[i] = DciValue.createFromMessage(response, base);
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
      return getDataCollectionSummary(nodeId, 0, false, false, false);
   }
   
   /**
    * Get tooltip last values for all objects 
    * 
    * @param nodeList list of node IDs
    * @return map with node id to DCI last values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<Long, DciValue[]> getTooltipLastValues(Set<Long> nodeList) throws IOException, NXCException
   {
      Map<Long, DciValue[]> cachedDciValues = new HashMap<Long, DciValue[]>();
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_TOOLTIP_LAST_VALUES);
      msg.setField(NXCPCodes.VID_OBJECT_LIST, nodeList.toArray(new Long[nodeList.size()]));
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      long base = NXCPCodes.VID_DCI_VALUES_BASE;
      List<DciValue> values = null;
      Long nodeId = 0L;
      for (int i = 0; i < count; i++, base += 50)
      {
         DciValue v = (DciValue)new SimpleDciValue(response, base);
         if(nodeId != v.getNodeId())
         {
            if (nodeId != 0)
            {
               cachedDciValues.put(nodeId, values.toArray(new DciValue[values.size()]));
            }
            values = new ArrayList<DciValue>();
            nodeId = v.getNodeId();
         }
         values.add(v);
         
      }
      if (nodeId != 0)
      {
         cachedDciValues.put(nodeId, values.toArray(new DciValue[values.size()]));
      }
      return cachedDciValues;
   }

   /**
    * Get last DCI values for given Map DCI Instance list
    *
    * @param mapDcis List with Map DCI Instances
    * @return List of DCI values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciValue[] getLastValues(Set<MapDCIInstance> mapDcis) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DCI_VALUES);
      long base = NXCPCodes.VID_DCI_VALUES_BASE;
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, mapDcis.size());
      for(MapDCIInstance item : mapDcis)
      {
         item.fillMessage(msg, base);
         base += 10;
      }
      return doLastValuesRequest(msg);
   }

   /**
    * Get last DCI values for given Single Dci Config list
    *
    * @param dciConfig List with Single Dci Configs
    * @return List of DCI values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciValue[] getLastValues(List<? extends MapDataSource> dciConfig) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DCI_VALUES);
      long base = NXCPCodes.VID_DCI_VALUES_BASE;
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, dciConfig.size());
      for(MapDataSource c : dciConfig)
      {
         c.fillMessage(msg, base);
         base += 10;
      }
      return doLastValuesRequest(msg);
   }

   /**
    * Send request for last values using prepared message
    *
    * @param msg NXCP message to send
    * @return The DCI values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private DciValue[] doLastValuesRequest(NXCPMessage msg) throws IOException, NXCException
   {
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      DciValue[] list = new DciValue[count];
      long fieldId = NXCPCodes.VID_DCI_VALUES_BASE;
      for(int i = 0; i < count; i++, fieldId += 50)
      {
         list[i] = (DciValue)new SimpleDciValue(response, fieldId);
      }

      return list;
   }

   /**
    * Get active thresholds
    *
    * @param dciConfig Dci config
    * @return list of active thresholds
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<Threshold> getActiveThresholds(List<MapDataSource> dciConfig) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ACTIVE_THRESHOLDS);
      long base = NXCPCodes.VID_DCI_VALUES_BASE;
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, dciConfig.size());
      for(MapDataSource c : dciConfig)
      {
         c.fillMessage(msg, base);
         base += 10;
      }

      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_THRESHOLDS);
      List<Threshold> list = new ArrayList<Threshold>(count);
      base = NXCPCodes.VID_DCI_THRESHOLD_BASE;
      for(int i = 0; i < count; i++, base += 20)
      {
         list.add(new Threshold(response, base));
      }

      return list;
   }

   /**
    * Get last value for given table DCI on given node
    *
    * @param nodeId ID of the node to get DCI values for
    * @param dciId DCI ID
    * @return Table object with last values for table DCI
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table getTableLastValues(final long nodeId, final long dciId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_TABLE_LAST_VALUE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setFieldUInt32(NXCPCodes.VID_DCI_ID, dciId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new Table(response);
   }

   /**
    * Get last value for given table or single valued DCI on given node
    *
    * @param nodeId ID of the node to get DCI values for
    * @param dciId DCI ID
    * @return Last value object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciLastValue getDciLastValue(final long nodeId, final long dciId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DCI_LAST_VALUE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setFieldUInt32(NXCPCodes.VID_DCI_ID, dciId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new DciLastValue(response);
   }

   /**
    * Get list of DCIs configured to be shown on performance tab in console for given node.
    *
    * @param nodeId Node object ID
    * @return List of performance tab DCIs
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<PerfTabDci> getPerfTabItems(final long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PERFTAB_DCI_LIST);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      List<PerfTabDci> list = new ArrayList<PerfTabDci>(count);
      long baseId = NXCPCodes.VID_SYSDCI_LIST_BASE;
      for(int i = 0; i < count; i++, baseId += 50)
         list.add(new PerfTabDci(response, baseId, nodeId));

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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      List<ThresholdViolationSummary> list = new ArrayList<ThresholdViolationSummary>();
      long fieldId = NXCPCodes.VID_THRESHOLD_BASE;
      while(response.getFieldAsInt64(fieldId) != 0)
      {
         final ThresholdViolationSummary t = new ThresholdViolationSummary(response, fieldId);
         list.add(t);
         fieldId += 50 * t.getDciList().size() + 2;
      }

      return list;
   }

   /**
    * Parse data from raw message CMD_DCI_DATA.
    * This method is intended for calling by client internal methods only. It made public to
    * allow access from client extensions.
    *
    * @param input Raw data
    * @param data  Data object to add rows to
    * @return number of received data rows
    */
   public int parseDataRows(final byte[] input, DataSeries data)
   {
      final NXCPDataInputStream inputStream = new NXCPDataInputStream(input);
      int rows = 0;
      DciDataRow row = null;

      try
      {
         rows = inputStream.readInt();
         final DataType dataType = DataType.getByValue(inputStream.readShort());
         data.setDataType(dataType);
         short flags = inputStream.readShort();

         for(int i = 0; i < rows; i++)
         {
            long timestamp = inputStream.readLong();
            Object value;
            switch(dataType)
            {
               case INT32:
                  value = Long.valueOf(inputStream.readInt());
                  break;
               case UINT32:
               case COUNTER32:
                  value = Long.valueOf(inputStream.readUnsignedInt());
                  break;
               case INT64:
               case UINT64:
               case COUNTER64:
                  value = Long.valueOf(inputStream.readLong());
                  break;
               case FLOAT:
                  value = Double.valueOf(inputStream.readDouble());
                  break;
               case STRING:
                  value = inputStream.readUTF();
                  break;
               default:
                  value = null;
                  break;
            }
            row = new DciDataRow(new Date(timestamp), value);
            data.addDataRow(row);
            if ((flags & 0x01) != 0) // raw value present
            {
               row.setRawValue(inputStream.readUTF());
            }
         }
      }
      catch(IOException e)
      {
         logger.debug("Internal error in parseDataRows", e);
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
    * @param valueType  TODO
    * @param delegateReadObject delegate object read access should be provided thought 
    * @return DCI data set
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private DataSeries getCollectedDataInternal(long nodeId, long dciId, String instance, String dataColumn, Date from, Date to,
         int maxRows, HistoricalDataType valueType, long delegateReadObject) throws IOException, NXCException
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
         msg = newMessage((valueType == HistoricalDataType.FULL_TABLE) ? NXCPCodes.CMD_GET_TABLE_DCI_DATA : NXCPCodes.CMD_GET_DCI_DATA);
      }
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setFieldUInt32(NXCPCodes.VID_DCI_ID, dciId);
      msg.setFieldInt16(NXCPCodes.VID_HISTORICAL_DATA_TYPE, valueType.getValue());
      msg.setFieldUInt32(NXCPCodes.VID_DELEGATE_OBJECT_ID, delegateReadObject);

      DataSeries data = new DataSeries(nodeId, dciId);

      long timeFrom = (from != null) ? from.getTime() : 0;
      long timeTo = (to != null) ? to.getTime() : 0;

      // If full table values are requested, each value will be sent in separate message
      if (valueType == HistoricalDataType.FULL_TABLE)
      {
         msg.setFieldInt32(NXCPCodes.VID_MAX_ROWS, maxRows);
         msg.setFieldInt64(NXCPCodes.VID_TIME_FROM, timeFrom);
         msg.setFieldInt64(NXCPCodes.VID_TIME_TO, timeTo);
         sendMessage(msg);

         NXCPMessage response = waitForRCC(msg.getMessageId());
         data.updateFromMessage(response);

         while(true)
         {
            response = waitForMessage(NXCPCodes.CMD_DCI_DATA, msg.getMessageId());
            long timestamp = response.getFieldAsInt64(NXCPCodes.VID_TIMESTAMP_MS);
            if (timestamp == 0)
               break; // End of value list indicator

            data.addDataRow(new DciDataRow(new Date(timestamp), new Table(response)));
         }
      }
      else
      {
         int rowsReceived, rowsRemaining = maxRows;
         do
         {
            msg.setMessageId(requestId.getAndIncrement());
            msg.setFieldInt32(NXCPCodes.VID_MAX_ROWS, maxRows);
            msg.setFieldInt64(NXCPCodes.VID_TIME_FROM, timeFrom);
            msg.setFieldInt64(NXCPCodes.VID_TIME_TO, timeTo);
            sendMessage(msg);

            NXCPMessage response = waitForRCC(msg.getMessageId());
            data.updateFromMessage(response);

            response = waitForMessage(NXCPCodes.CMD_DCI_DATA, msg.getMessageId());
            if (!response.isBinaryMessage())
               throw new NXCException(RCC.INTERNAL_ERROR);

            rowsReceived = parseDataRows(response.getBinaryData(), data);
            if (((rowsRemaining == 0) || (rowsRemaining > MAX_DCI_DATA_ROWS)) && (rowsReceived == MAX_DCI_DATA_ROWS))
            {
               // adjust boundaries for next request
               if (rowsRemaining > 0)
                  rowsRemaining -= rowsReceived;

               // Rows goes in newest to oldest order, so if we need to
               // retrieve additional data, we should update timeTo limit
               if (to != null)
               {
                  DciDataRow row = data.getLastValue();
                  if (row != null)
                  {
                     // There should be only one value per millisecond, so we set
                     // last row's timestamp - 1 millisecond as new boundary
                     timeTo = row.getTimestamp().getTime() - 1;
                  }
               }
            }
         } while(rowsReceived == MAX_DCI_DATA_ROWS);
      }
      return data;
   }

   /**
    * Get collected DCI data from server. Please note that you should specify
    * either row count limit or time from/to limit.
    *
    * @param nodeId    Node ID
    * @param dciId     DCI ID
    * @param from      Start of time range or null for no limit
    * @param to        End of time range or null for no limit
    * @param maxRows   Maximum number of rows to retrieve or 0 for no limit
    * @param valueType TODO
    * @param delegateReadObject delegate object read access should be provided thought 
    * @return DCI data set
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DataSeries getCollectedData(long nodeId, long dciId, Date from, Date to, int maxRows, HistoricalDataType valueType, long delegateReadObject)
         throws IOException, NXCException
   {
      return getCollectedDataInternal(nodeId, dciId, null, null, from, to, maxRows, valueType, delegateReadObject);
   }

   /**
    * Get collected DCI data from server. Please note that you should specify
    * either row count limit or time from/to limit.
    *
    * @param nodeId    Node ID
    * @param dciId     DCI ID
    * @param from      Start of time range or null for no limit
    * @param to        End of time range or null for no limit
    * @param maxRows   Maximum number of rows to retrieve or 0 for no limit
    * @param valueType TODO
    * @return DCI data set
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DataSeries getCollectedData(long nodeId, long dciId, Date from, Date to, int maxRows, HistoricalDataType valueType)
         throws IOException, NXCException
   {
      return getCollectedDataInternal(nodeId, dciId, null, null, from, to, maxRows, valueType, 0);
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
    * @param delegateReadObject delegate object read access should be provided thought 
    * @return DCI data set
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DataSeries getCollectedTableData(long nodeId, long dciId, String instance, String dataColumn, Date from, Date to,
         int maxRows, long delegateReadObject) throws IOException, NXCException
   {
      if (instance == null || dataColumn == null)
         throw new NXCException(RCC.INVALID_ARGUMENT);
      return getCollectedDataInternal(nodeId, dciId, instance, dataColumn, from, to, maxRows, HistoricalDataType.PROCESSED, delegateReadObject);
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
   public DataSeries getCollectedTableData(long nodeId, long dciId, String instance, String dataColumn, Date from, Date to,
         int maxRows) throws IOException, NXCException
   {
      if (instance == null || dataColumn == null)
         throw new NXCException(RCC.INVALID_ARGUMENT);
      return getCollectedDataInternal(nodeId, dciId, instance, dataColumn, from, to, maxRows, HistoricalDataType.PROCESSED, 0);
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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setFieldUInt32(NXCPCodes.VID_DCI_ID, dciId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete collected data entry for given DCI
    *
    * @param nodeId    Node object ID
    * @param dciId     DCI ID
    * @param timestamp timestamp of entry
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteDciEntry(long nodeId, long dciId, long timestamp) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_DCI_ENTRY);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setFieldUInt32(NXCPCodes.VID_DCI_ID, dciId);
      msg.setFieldUInt32(NXCPCodes.VID_TIMESTAMP_MS, timestamp);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Start background recalculation of DCI values using preserved raw values.
    *
    * @param objectId object ID
    * @param dciId DCI ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void recalculateDCIValues(long objectId, long dciId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RECALCULATE_DCI_VALUES);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setFieldUInt32(NXCPCodes.VID_DCI_ID, dciId);
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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setFieldUInt32(NXCPCodes.VID_DCI_ID, dciId);
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
      return getThresholds(nodeId, dciId, 0);
   }

   /**
    * Get list of thresholds configured for given DCI
    *
    * @param nodeId Node object ID
    * @param dciId  DCI ID
    * @param delegateReadObject delegate object read access should be provided thought 
    * @return List of configured thresholds
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Threshold[] getThresholds(final long nodeId, final long dciId, long delegateReadObject) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DCI_THRESHOLDS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setFieldUInt32(NXCPCodes.VID_DCI_ID, dciId);
      msg.setFieldUInt32(NXCPCodes.VID_DELEGATE_OBJECT_ID, delegateReadObject);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_THRESHOLDS);
      final Threshold[] list = new Threshold[count];

      long fieldId = NXCPCodes.VID_DCI_THRESHOLD_BASE;
      for(int i = 0; i < count; i++)
      {
         list[i] = new Threshold(response, fieldId);
         fieldId += 20;
      }

      return list;
   }

   /**
    * Get names for given DCI list
    *
    * @param nodeIds node identifiers
    * @param dciIds  DCI identifiers (length must match length of node identifiers list)
    * @return array of resolved DCI names
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<Long, DciInfo> dciIdsToNames(List<Long> nodeIds, List<Long> dciIds) throws IOException, NXCException
   {
      if (nodeIds.isEmpty())
         return new HashMap<Long, DciInfo>();

      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RESOLVE_DCI_NAMES);
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, nodeIds.size());
      msg.setField(NXCPCodes.VID_NODE_LIST, nodeIds);
      msg.setField(NXCPCodes.VID_DCI_LIST, dciIds);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      Map<Long, DciInfo> result = new HashMap<Long, DciInfo>();
      int size = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      long fieldId = NXCPCodes.VID_DCI_LIST_BASE;
      for(int i = 0; i < size; i++)
      {
         result.put(response.getFieldAsInt64(fieldId), new DciInfo(response.getFieldAsString(fieldId + 1), response.getFieldAsString(fieldId + 2), response.getFieldAsString(fieldId + 3)));
         fieldId += 4;
      }
      return result;
   }

   /**
    * Get names for given DCI list
    *
    * @param itemList list of items to resolved
    * @return map of DCI id to DCI description
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<Long, DciInfo> dciIdsToNames(Collection<? extends NodeItemPair> itemList) throws IOException, NXCException
   {
      if (itemList.isEmpty())
         return new HashMap<Long, DciInfo>();

      List<Long> nodeIds = new ArrayList<Long>();
      List<Long> dciIds = new ArrayList<Long>();
      for(NodeItemPair nodeItem : itemList)
      {
         if (nodeItem.getNodeId() != 0 && nodeItem.getDciId() != 0)
         {
            nodeIds.add(nodeItem.getNodeId());
            dciIds.add(nodeItem.getDciId());
         }
      }
      return dciIdsToNames(nodeIds, dciIds);
   }

   /**
    * Get DCI ID for given DCI name
    *
    * @param nodeId  node object identifier
    * @param dciName DCI name
    * @return id of DCI with given name
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long dciNameToId(long nodeId, String dciName) throws IOException, NXCException
   {
      if (nodeId == 0 || dciName == null || dciName.isEmpty())
         return 0;

      DciValue[] list = getLastValues(nodeId);
      for(DciValue dciValue : list)
      {
         if (dciValue.getName().equals(dciName))
            return dciValue.getId();
      }
      return 0;
   }

   /**
    * Query metric immediately. This call will cause server to do actual call to managed node and will return current value for
    * given metric. Result is not cached.
    *
    * @param nodeId node object ID
    * @param origin parameter's origin (NetXMS agent, SNMP, etc.)
    * @param name parameter's name
    * @return current parameter's value
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String queryMetric(long nodeId, DataOrigin origin, String name) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_QUERY_PARAMETER);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setFieldInt16(NXCPCodes.VID_DCI_SOURCE_TYPE, origin.getValue());
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsString(NXCPCodes.VID_VALUE);
   }

   /**
    * Query table immediately. This call will cause server to do actual call to managed node and will return current value for given
    * table. Result is not cached.
    *
    * @param nodeId node object ID
    * @param origin parameter's origin (NetXMS agent, SNMP, etc.)
    * @param name table's name
    * @return current table's value
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table queryTable(long nodeId, DataOrigin origin, String name) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_QUERY_TABLE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setFieldInt16(NXCPCodes.VID_DCI_SOURCE_TYPE, origin.getValue());
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new Table(response);
   }

   /**
    * Query agent's table immediately. This call will cause server to do actual call to managed node and will return current value
    * for given table. Result is not cached.
    *
    * @param nodeId node object ID
    * @param name table's name
    * @return current table's value
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table queryAgentTable(long nodeId, String name) throws IOException, NXCException
   {
      return queryTable(nodeId, DataOrigin.AGENT, name);
   }

   /**
    * Hook method to allow adding of custom object creation data to NXCP message. Default implementation does nothing.
    *
    * @param data object creation data passed to createObject
    * @param userData user-defined data for object creation passed to createObject
    * @param msg NXCP message that will be sent to server
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
      msg.setFieldUInt32(NXCPCodes.VID_PARENT_ID, data.getParentId());
      msg.setFieldInt16(NXCPCodes.VID_OBJECT_CLASS, data.getObjectClass());
      msg.setField(NXCPCodes.VID_OBJECT_NAME, data.getName());
      if ((data.getObjectAlias() != null) && !data.getObjectAlias().isEmpty())
         msg.setField(NXCPCodes.VID_ALIAS, data.getObjectAlias());
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, data.getZoneUIN());
      msg.setFieldUInt32(NXCPCodes.VID_ASSET_ID, data.getAssetId());
      if (data.getComments() != null)
         msg.setField(NXCPCodes.VID_COMMENTS, data.getComments());

      // Class-specific attributes
      switch(data.getObjectClass())
      {
         case AbstractObject.OBJECT_ASSET:
            if (data.getAssetProperties() != null)
               msg.setFieldsFromStringMap(data.getAssetProperties(), NXCPCodes.VID_ASSET_PROPERTIES_BASE, NXCPCodes.VID_NUM_ASSET_PROPERTIES);
            break;
         case AbstractObject.OBJECT_CHASSIS:
            msg.setFieldUInt32(NXCPCodes.VID_CONTROLLER_ID, data.getControllerId());
            break;
         case AbstractObject.OBJECT_INTERFACE:
            msg.setField(NXCPCodes.VID_MAC_ADDR, data.getMacAddress().getValue());
            msg.setField(NXCPCodes.VID_IP_ADDRESS, data.getIpAddress());
            msg.setFieldInt32(NXCPCodes.VID_IF_TYPE, data.getIfType());
            msg.setFieldInt32(NXCPCodes.VID_IF_INDEX, data.getIfIndex());
            msg.setFieldInt32(NXCPCodes.VID_PHY_CHASSIS, data.getChassis());
            msg.setFieldInt32(NXCPCodes.VID_PHY_MODULE, data.getModule());
            msg.setFieldInt32(NXCPCodes.VID_PHY_PIC, data.getPIC());
            msg.setFieldInt32(NXCPCodes.VID_PHY_PORT, data.getPort());
            msg.setFieldInt16(NXCPCodes.VID_IS_PHYS_PORT, data.isPhysicalPort() ? 1 : 0);
            break;
         case AbstractObject.OBJECT_MOBILEDEVICE:
            msg.setField(NXCPCodes.VID_DEVICE_ID, data.getDeviceId());
            break;
         case AbstractObject.OBJECT_NODE:
            if (data.getPrimaryName() != null)
               msg.setField(NXCPCodes.VID_PRIMARY_NAME, data.getPrimaryName());
            msg.setField(NXCPCodes.VID_IP_ADDRESS, data.getIpAddress());
            msg.setFieldInt16(NXCPCodes.VID_AGENT_PORT, data.getAgentPort());
            msg.setFieldInt16(NXCPCodes.VID_SNMP_PORT, data.getSnmpPort());
            msg.setFieldInt16(NXCPCodes.VID_ETHERNET_IP_PORT, data.getEtherNetIpPort());
            msg.setFieldInt16(NXCPCodes.VID_MODBUS_TCP_PORT, data.getModbusTcpPort());
            msg.setFieldInt16(NXCPCodes.VID_MODBUS_UNIT_ID, data.getModbusUnitId());
            msg.setFieldInt32(NXCPCodes.VID_CREATION_FLAGS, data.getCreationFlags());
            msg.setFieldUInt32(NXCPCodes.VID_AGENT_PROXY, data.getAgentProxyId());
            msg.setFieldUInt32(NXCPCodes.VID_SNMP_PROXY, data.getSnmpProxyId());
            msg.setFieldUInt32(NXCPCodes.VID_MQTT_PROXY, data.getMqttProxyId());
            msg.setFieldUInt32(NXCPCodes.VID_ETHERNET_IP_PROXY, data.getEtherNetIpProxyId());
            msg.setFieldUInt32(NXCPCodes.VID_MODBUS_PROXY, data.getModbusProxyId());
            msg.setFieldUInt32(NXCPCodes.VID_ICMP_PROXY, data.getIcmpProxyId());
            msg.setFieldUInt32(NXCPCodes.VID_SSH_PROXY, data.getSshProxyId());
            msg.setFieldInt16(NXCPCodes.VID_SSH_PORT, data.getSshPort());
            msg.setField(NXCPCodes.VID_SSH_LOGIN, data.getSshLogin());
            msg.setField(NXCPCodes.VID_SSH_PASSWORD, data.getSshPassword());
            msg.setFieldUInt32(NXCPCodes.VID_VNC_PROXY, data.getVncProxyId());
            msg.setFieldInt16(NXCPCodes.VID_VNC_PORT, data.getVncPort());
            msg.setField(NXCPCodes.VID_VNC_PASSWORD, data.getVncPassword());
            msg.setFieldUInt32(NXCPCodes.VID_WEB_SERVICE_PROXY, data.getWebServiceProxyId());
            break;
         case AbstractObject.OBJECT_NETWORKMAP:
            msg.setFieldInt16(NXCPCodes.VID_MAP_TYPE, data.getMapType().getValue());
            msg.setField(NXCPCodes.VID_SEED_OBJECTS, data.getSeedObjectIds());
            msg.setFieldUInt32(NXCPCodes.VID_FLAGS, data.getFlags());
            break;
         case AbstractObject.OBJECT_NETWORKSERVICE:
            msg.setFieldInt16(NXCPCodes.VID_SERVICE_TYPE, data.getServiceType());
            msg.setFieldInt16(NXCPCodes.VID_IP_PROTO, data.getIpProtocol());
            msg.setFieldInt16(NXCPCodes.VID_IP_PORT, data.getIpPort());
            msg.setField(NXCPCodes.VID_SERVICE_REQUEST, data.getRequest());
            msg.setField(NXCPCodes.VID_SERVICE_RESPONSE, data.getResponse());
            msg.setFieldInt16(NXCPCodes.VID_CREATE_STATUS_DCI, data.isCreateStatusDci() ? 1 : 0);
            break;
         case AbstractObject.OBJECT_RACK:
            msg.setFieldInt16(NXCPCodes.VID_HEIGHT, data.getHeight());
            break;
         case AbstractObject.OBJECT_SENSOR:
            msg.setFieldInt32(NXCPCodes.VID_SENSOR_FLAGS, data.getFlags());
            msg.setField(NXCPCodes.VID_MAC_ADDR, data.getMacAddress());
            msg.setFieldInt16(NXCPCodes.VID_DEVICE_CLASS, data.getDeviceClass().getValue());
            msg.setField(NXCPCodes.VID_VENDOR, data.getVendor());
            msg.setField(NXCPCodes.VID_MODEL, data.getModel());
            msg.setField(NXCPCodes.VID_SERIAL_NUMBER, data.getSerialNumber());
            msg.setField(NXCPCodes.VID_DEVICE_ADDRESS, data.getDeviceAddress());
            msg.setFieldUInt32(NXCPCodes.VID_GATEWAY_NODE, data.getGatewayNodeId());
            msg.setFieldInt32(NXCPCodes.VID_MODBUS_UNIT_ID, data.getModbusUnitId());
            break;
         case AbstractObject.OBJECT_SUBNET:
            msg.setField(NXCPCodes.VID_IP_ADDRESS, data.getIpAddress());
            break;
         case AbstractObject.OBJECT_BUSINESSSERVICEPROTOTYPE:
            msg.setFieldInt16(NXCPCodes.VID_INSTD_METHOD, data.getInstanceDiscoveryMethod());
            break;
      }

      if (userData != null)
         createCustomObject(data, userData, msg);

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
    * Create new NetXMS object in synchronous mode. 
    *
    * @param data Object creation data
    * @return newly created AbstractObject
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public AbstractObject createObjectSync(final NXCObjectCreationData data) throws IOException, NXCException
   {
      long id = createObject(data, null);
      FutureObject object = findFutureObjectById(id);
      synchronized (object) 
      {
         while (!object.hasObject())
         {
            try
            {
               object.wait();
            }
            catch(InterruptedException e)
            {
            }
         }
      }

      return object.getObject();
   }

   /**
    * Create new NetXMS object and run callback once it's created.
    * 
    * @param data Object creation data
    * @param callback callback to run on object creation
    * @return ID of new object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long createObjectAsync(final NXCObjectCreationData data, ObjectCreationListener callback) throws IOException, NXCException
   {
      long id = createObject(data, null);
      findObjectAsync(id, callback); 
      return id;
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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
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
            objectListGUID.remove(object.getGuid());
            if (object instanceof Zone)
               zoneList.remove(((Zone)object).getUIN());
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
            objectListGUID.remove(object.getGuid());
            if (object instanceof Zone)
               zoneList.remove(((Zone)object).getUIN());
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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, data.getObjectId());

      // Object name
      if (data.getName() != null)
      {
         msg.setField(NXCPCodes.VID_OBJECT_NAME, data.getName());
      }

      // Object alias
      if (data.getAlias() != null)
      {
         msg.setField(NXCPCodes.VID_ALIAS, data.getAlias());
      }

      // Object name on network map
      if (data.getNameOnMap() != null)
      {
         msg.setField(NXCPCodes.VID_NAME_ON_MAP, data.getNameOnMap());
      }

      // Primary IP
      if (data.getPrimaryIpAddress() != null)
      {
         msg.setField(NXCPCodes.VID_IP_ADDRESS, data.getPrimaryIpAddress());
      }

      // Access control list
      if (data.getACL() != null)
      {
         if (data.isInheritAccessRights() == null)
            throw new IllegalArgumentException("Access control list should be set together with inherited access rights flag");

         final Collection<AccessListElement> acl = data.getACL();
         msg.setFieldInt32(NXCPCodes.VID_ACL_SIZE, acl.size());
         msg.setFieldInt16(NXCPCodes.VID_INHERIT_RIGHTS, data.isInheritAccessRights() ? 1 : 0);

         long id1 = NXCPCodes.VID_ACL_USER_BASE;
         long id2 = NXCPCodes.VID_ACL_RIGHTS_BASE;
         for(AccessListElement e : acl)
         {
            msg.setFieldInt32(id1++, e.getUserId());
            msg.setFieldInt32(id2++, e.getAccessRights());
         }
      }

      if (data.getCustomAttributes() != null)
      {
         Map<String, CustomAttribute> attrList = data.getCustomAttributes();
         Iterator<String> it = attrList.keySet().iterator();
         long id = NXCPCodes.VID_CUSTOM_ATTRIBUTES_BASE;
         int count = 0;
         while(it.hasNext())
         {
            String key = it.next();
            CustomAttribute attr = attrList.get(key);
            if (attr.getSourceObject() != 0 && !attr.isRedefined())
               continue;
            msg.setField(id++, key);
            msg.setField(id++, attr.getValue());
            msg.setFieldUInt32(id++, attr.getFlags());
            count++;
         }
         msg.setFieldInt32(NXCPCodes.VID_NUM_CUSTOM_ATTRIBUTES, count);
      }

      if (data.getAutoBindFilter() != null)
      {
         msg.setField(NXCPCodes.VID_AUTOBIND_FILTER, data.getAutoBindFilter());
      }

      if (data.getAutoBindFilter2() != null)
      {
         msg.setField(NXCPCodes.VID_AUTOBIND_FILTER_2, data.getAutoBindFilter2());
      }

      if (data.getAutoBindFlags() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_AUTOBIND_FLAGS, data.getAutoBindFlags());
      }

      if (data.getFilter() != null)
      {
         msg.setField(NXCPCodes.VID_FILTER, data.getFilter());
      }

      if (data.getLinkStylingScript() != null)
      {
         msg.setField(NXCPCodes.VID_LINK_STYLING_SCRIPT, data.getLinkStylingScript());
      }

      if (data.getVersion() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_VERSION, data.getVersion());
      }

      if (data.getAgentPort() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_AGENT_PORT, data.getAgentPort());
      }

      if (data.getAgentProxy() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_AGENT_PROXY, data.getAgentProxy().intValue());
      }

      if (data.getAgentSecret() != null)
      {
         msg.setField(NXCPCodes.VID_SHARED_SECRET, data.getAgentSecret());
      }

      if (data.getTrustedObjects() != null)
      {
         msg.setField(NXCPCodes.VID_TRUSTED_OBJECTS, data.getTrustedObjects());
      }

      if (data.getSnmpVersion() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_SNMP_VERSION, data.getSnmpVersion().getValue());
      }

      if (data.getSnmpAuthName() != null || data.getSnmpAuthPassword() != null || 
            data.getSnmpPrivPassword() != null || data.getSnmpAuthMethod() != null ||
            data.getSnmpPrivMethod() != null)         
      {
         if (data.getSnmpAuthName() == null || data.getSnmpAuthPassword() == null || 
               data.getSnmpPrivPassword() == null || data.getSnmpAuthMethod() == null ||
               data.getSnmpPrivMethod() == null) 
            throw new IllegalArgumentException("All SNMP fields should be set: auth name, auth password, priv password, auth method, priv method");

         msg.setField(NXCPCodes.VID_SNMP_AUTH_OBJECT, data.getSnmpAuthName());
         msg.setField(NXCPCodes.VID_SNMP_AUTH_PASSWORD, data.getSnmpAuthPassword());
         msg.setField(NXCPCodes.VID_SNMP_PRIV_PASSWORD, data.getSnmpPrivPassword());
         int methods = data.getSnmpAuthMethod() | (data.getSnmpPrivMethod() << 8);
         msg.setFieldInt16(NXCPCodes.VID_SNMP_USM_METHODS, methods);
      }

      if (data.getSnmpProxy() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_SNMP_PROXY, data.getSnmpProxy().intValue());
      }

      if (data.getSnmpPort() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_SNMP_PORT, data.getSnmpPort());
      }

      if (data.getMqttProxy() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_MQTT_PROXY, data.getMqttProxy().intValue());
      }

      if (data.getIcmpProxy() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_ICMP_PROXY, data.getIcmpProxy().intValue());
      }

      if (data.getGeolocation() != null)
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

      if (data.getMapLayout() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_LAYOUT, data.getMapLayout().getValue());
      }
      
      if (data.getMapWidth() != null || data.getMapHeight() != null)
      {
         if (data.getMapWidth() == null || data.getMapHeight() == null)
            throw new IllegalArgumentException("Both map width and height should be set");
         msg.setFieldInt32(NXCPCodes.VID_WIDTH, data.getMapWidth());
         msg.setFieldInt32(NXCPCodes.VID_HEIGHT, data.getMapHeight());
      }

      if (data.getMapBackground() != null || data.getMapBackgroundLocation() != null ||
         data.getMapBackgroundZoom() != null || data.getMapBackgroundColor() != null)
      {
         if (data.getMapBackground() == null || data.getMapBackgroundLocation() == null ||
            data.getMapBackgroundZoom() == null || data.getMapBackgroundColor() == null)
            throw new IllegalArgumentException("All map background attributes should be set (image ID, geolocation, zoom, and color)");

         msg.setField(NXCPCodes.VID_BACKGROUND, data.getMapBackground());
         msg.setField(NXCPCodes.VID_BACKGROUND_LATITUDE, data.getMapBackgroundLocation().getLatitude());
         msg.setField(NXCPCodes.VID_BACKGROUND_LONGITUDE, data.getMapBackgroundLocation().getLongitude());
         msg.setFieldInt16(NXCPCodes.VID_BACKGROUND_ZOOM, data.getMapBackgroundZoom());
         msg.setFieldInt32(NXCPCodes.VID_BACKGROUND_COLOR, data.getMapBackgroundColor());
      }

      if (data.getMapImage() != null)
      {
         msg.setField(NXCPCodes.VID_IMAGE, data.getMapImage());
      }

      if (data.getMapElements() != null || data.getMapLinks() != null)
      {
         if (data.getMapElements() == null || data.getMapLinks() == null)
            throw new IllegalArgumentException("Both map elements and map links should be set");
            
         msg.setFieldInt32(NXCPCodes.VID_NUM_ELEMENTS, data.getMapElements().size());
         long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
         for(NetworkMapElement e : data.getMapElements())
         {
            e.fillMessage(msg, fieldId);
            fieldId += 100;
         }

         msg.setFieldInt32(NXCPCodes.VID_NUM_LINKS, data.getMapLinks().size());
         fieldId = NXCPCodes.VID_LINK_LIST_BASE;
         for(NetworkMapLink l : data.getMapLinks())
         {
            l.fillMessage(msg, fieldId);
            fieldId += 20;
         }
      }

      if (data.getColumnCount() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_NUM_COLUMNS, data.getColumnCount());
      }

      if (data.getDashboardElements() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_NUM_ELEMENTS, data.getDashboardElements().size());
         long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
         for(DashboardElement e : data.getDashboardElements())
         {
            e.fillMessage(msg, fieldId);
            fieldId += 10;
         }
      }

      if (data.getDashboardNameTemplate() != null)
      {
         msg.setField(NXCPCodes.VID_DASHBOARD_NAME_TEMPLATE, data.getDashboardNameTemplate());
      }

      if (data.getUrls() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_NUM_URLS, data.getUrls().size());
         long fieldId = NXCPCodes.VID_URL_LIST_BASE;
         for(ObjectUrl u : data.getUrls())
         {
            u.fillMessage(msg, fieldId);
            fieldId += 10;
         }
      }

      if (data.getScript() != null)
      {
         msg.setField(NXCPCodes.VID_SCRIPT, data.getScript());
      }

      if (data.getActivationEvent() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_ACTIVATION_EVENT, data.getActivationEvent());
      }

      if (data.getDeactivationEvent() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_DEACTIVATION_EVENT, data.getDeactivationEvent());
      }

      if (data.getSourceObject() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_SOURCE_OBJECT, data.getSourceObject().intValue());
      }

      if (data.getActiveStatus() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_ACTIVE_STATUS, data.getActiveStatus());
      }

      if (data.getInactiveStatus() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_INACTIVE_STATUS, data.getInactiveStatus());
      }

      if (data.getDciList() != null)
      {
         List<ConditionDciInfo> dciList = data.getDciList();
         msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, dciList.size());
         long varId = NXCPCodes.VID_DCI_LIST_BASE;
         for(ConditionDciInfo dci : dciList)
         {
            msg.setFieldUInt32(varId++, dci.getDciId());
            msg.setFieldUInt32(varId++, dci.getNodeId());
            msg.setFieldInt16(varId++, dci.getFunction());
            msg.setFieldInt16(varId++, dci.getPolls());
            varId += 6;
         }
      }

      if (data.getDrillDownObjectId() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_DRILL_DOWN_OBJECT_ID, data.getDrillDownObjectId().intValue());
      }

      if (data.getServiceType() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_SERVICE_TYPE, data.getServiceType());
      }

      if (data.getIpAddress() != null)
      {
         msg.setField(NXCPCodes.VID_IP_ADDRESS, data.getIpAddress());
      }

      if (data.getIpProtocol() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_IP_PROTO, data.getIpProtocol());
      }

      if (data.getIpPort() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_IP_PORT, data.getIpPort());
      }

      if (data.getPollerNode() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_POLLER_NODE_ID, data.getPollerNode().intValue());
      }

      if (data.getRequiredPolls() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_REQUIRED_POLLS, data.getRequiredPolls());
      }

      if (data.getRequest() != null)
      {
         msg.setField(NXCPCodes.VID_SERVICE_REQUEST, data.getRequest());
      }

      if (data.getResponse() != null)
      {
         msg.setField(NXCPCodes.VID_SERVICE_RESPONSE, data.getResponse());
      }

      if (data.getObjectFlags() != null || data.getObjectFlagsMask() != null)
      {            
         msg.setFieldInt32(NXCPCodes.VID_FLAGS, data.getObjectFlags());
         if (data.getObjectFlagsMask() != null)
            msg.setFieldInt32(NXCPCodes.VID_FLAGS_MASK, data.getObjectFlagsMask());
      }

      if (data.getIfXTablePolicy() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_USE_IFXTABLE, data.getIfXTablePolicy());
      }

      if (data.getReportDefinition() != null)
      {
         msg.setField(NXCPCodes.VID_REPORT_DEFINITION, data.getReportDefinition());
      }

      if (data.getResourceList() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_NUM_RESOURCES, data.getResourceList().size());
         long varId = NXCPCodes.VID_RESOURCE_LIST_BASE;
         for(ClusterResource r : data.getResourceList())
         {
            msg.setFieldUInt32(varId++, r.getId());
            msg.setField(varId++, r.getName());
            msg.setField(varId++, r.getVirtualAddress());
            varId += 7;
         }
      }

      if (data.getNetworkList() != null)
      {
         int count = data.getNetworkList().size();
         msg.setFieldInt32(NXCPCodes.VID_NUM_SYNC_SUBNETS, count);
         long varId = NXCPCodes.VID_SYNC_SUBNETS_BASE;
         for(InetAddressEx n : data.getNetworkList())
         {
            msg.setField(varId++, n);
         }
      }

      if (data.getPrimaryName() != null)
      {
         msg.setField(NXCPCodes.VID_PRIMARY_NAME, data.getPrimaryName());
      }

      if (data.getStatusCalculationMethod() != null || data.getStatusPropagationMethod() != null ||
            data.getFixedPropagatedStatus() != null || data.getStatusShift() != null ||
            data.getStatusTransformation() != null || data.getStatusSingleThreshold() != null ||
            data.getStatusThresholds() != null)         
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

      if (data.getExpectedState() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_EXPECTED_STATE, data.getExpectedState());
      }

      if (data.getLinkColor() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_LINK_COLOR, data.getLinkColor());
      }

      if (data.getConnectionRouting() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_LINK_ROUTING, data.getConnectionRouting());
      }

      if (data.getNetworkMapLinkWidth()!= null)
      {
         msg.setFieldInt16(NXCPCodes.VID_LINK_WIDTH, data.getNetworkMapLinkWidth());
      }

      if (data.getNetworkMapLinkStyle() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_LINK_STYLE, data.getNetworkMapLinkStyle());
      }

      if (data.getDiscoveryRadius() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_DISCOVERY_RADIUS, data.getDiscoveryRadius());
      }

      if (data.getHeight() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_HEIGHT, data.getHeight());
      }

      if (data.isRackNumberingTopBottom() != null)
      {
         msg.setField(NXCPCodes.VID_TOP_BOTTOM, data.isRackNumberingTopBottom());
      }

      if (data.getPeerGatewayId() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_PEER_GATEWAY, data.getPeerGatewayId().intValue());
      }

      if (data.getLocalNetworks() != null || data.getRemoteNetworks() != null)
      {
         if (data.getLocalNetworks() == null || data.getRemoteNetworks() == null)
            throw new IllegalArgumentException("Both local and remote networks should be set together");
         
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

      if (data.getPostalAddress() != null)
      {
         data.getPostalAddress().fillMessage(msg);
      }

      if (data.getAgentCacheMode() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_AGENT_CACHE_MODE, data.getAgentCacheMode().getValue());
      }

      if (data.getAgentCompressionMode() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_AGENT_COMPRESSION_MODE, data.getAgentCompressionMode().getValue());
      }

      if (data.getMapObjectDisplayMode() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_DISPLAY_MODE, data.getMapObjectDisplayMode().getValue());
      }
      
      if (data.getFrontRackImage() != null || data.getRearRackImage() != null || data.getRackPosition() != null || data.getRackHeight() != null || data.getRackOrientation() != null)
      {
         if (data.getFrontRackImage() == null || data.getRearRackImage() == null || data.getRackPosition() == null || data.getRackHeight() == null || data.getRackOrientation() == null)
            throw new IllegalArgumentException("All rack placement attributes should be set (front image, rear image, position, height, and orientation)");

         msg.setFieldInt32(NXCPCodes.VID_PHYSICAL_CONTAINER_ID, data.getPhysicalContainerObjectId()!= null ? data.getPhysicalContainerObjectId().intValue() : 0);
         msg.setField(NXCPCodes.VID_RACK_IMAGE_FRONT, data.getFrontRackImage());
         msg.setField(NXCPCodes.VID_RACK_IMAGE_REAR, data.getRearRackImage());
         msg.setFieldInt16(NXCPCodes.VID_RACK_POSITION, data.getRackPosition());
         msg.setFieldInt16(NXCPCodes.VID_RACK_HEIGHT, data.getRackHeight());
         msg.setFieldInt16(NXCPCodes.VID_RACK_ORIENTATION, data.getRackOrientation().getValue());
      }

      if (data.getPhysicalContainerObjectId() != null || data.getChassisPlacement() != null)
      {
         if (data.getPhysicalContainerObjectId() == null || data.getChassisPlacement() == null)
            throw new IllegalArgumentException("Both physical container object and chassis placement should be set");

         msg.setFieldInt32(NXCPCodes.VID_PHYSICAL_CONTAINER_ID, data.getPhysicalContainerObjectId().intValue());
         msg.setField(NXCPCodes.VID_CHASSIS_PLACEMENT_CONFIG, data.getChassisPlacement());
      }

      if (data.getDashboards() != null)
      {
         msg.setField(NXCPCodes.VID_DASHBOARDS, data.getDashboards());
      }

      if (data.getControllerId() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_CONTROLLER_ID, data.getControllerId().intValue());
      }

      if (data.getSshProxy() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_SSH_PROXY, data.getSshProxy().intValue());
      }

      if (data.getSshKeyId() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_SSH_KEY_ID, data.getSshKeyId());
      }

      if (data.getSshLogin() != null)
      {
         msg.setField(NXCPCodes.VID_SSH_LOGIN, data.getSshLogin());
      }

      if (data.getSshPassword() != null)
      {
         msg.setField(NXCPCodes.VID_SSH_PASSWORD, data.getSshPassword());
      }

      if (data.getSshPort() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_SSH_PORT, data.getSshPort());
      }

      if (data.getVncProxy() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_VNC_PROXY, data.getVncProxy().intValue());
      }

      if (data.getVncPassword() != null)
      {
         msg.setField(NXCPCodes.VID_VNC_PASSWORD, data.getVncPassword());
      }

      if (data.getVncPort() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_VNC_PORT, data.getVncPort());
      }

      if (data.getZoneProxies() != null)
      {
         msg.setField(NXCPCodes.VID_ZONE_PROXY_LIST, data.getZoneProxies());
      }

      if (data.getSeedObjectIds() != null)
      {
         msg.setField(NXCPCodes.VID_SEED_OBJECTS, data.getSeedObjectIds());
      }

      if (data.getMacAddress() != null)
      {
         msg.setField(NXCPCodes.VID_MAC_ADDR, data.getMacAddress());
      }

      if (data.getDeviceClass() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_DEVICE_CLASS, data.getDeviceClass().getValue());
      }

      if (data.getVendor() != null)
      {
         msg.setField(NXCPCodes.VID_VENDOR, data.getVendor());
      }

      if (data.getModel() != null)
      {
         msg.setField(NXCPCodes.VID_MODEL, data.getModel());
      }

      if (data.getSerialNumber() != null)
      {
         msg.setField(NXCPCodes.VID_SERIAL_NUMBER, data.getSerialNumber());
      }

      if (data.getDeviceAddress() != null)
      {
         msg.setField(NXCPCodes.VID_DEVICE_ADDRESS, data.getDeviceAddress());
      }

      if (data.getGatewayNodeId() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_GATEWAY_NODE, data.getGatewayNodeId().intValue());
      }

      if (data.getPassiveElements() != null)
      {
         List<PassiveRackElement> elements = data.getPassiveElements();
         msg.setFieldInt32(NXCPCodes.VID_NUM_ELEMENTS, elements.size());
         long base = NXCPCodes.VID_ELEMENT_LIST_BASE;
         for(int i = 0; i < elements.size(); i++)
         {
            elements.get(i).fillMessage(msg, base);
            base += 10;
         }
      }

      if (data.getResponsibleUsers() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_RESPONSIBLE_USERS_COUNT, data.getResponsibleUsers().size());
         long fieldId = NXCPCodes.VID_RESPONSIBLE_USERS_BASE;
         for(ResponsibleUser r : data.getResponsibleUsers())
         {
            r.fillMessage(msg, fieldId);
            fieldId += 10;
         }
      }

      if (data.getIcmpStatCollectionMode() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_ICMP_COLLECTION_MODE, data.getIcmpStatCollectionMode().getValue());
      }
      
      if (data.getIcmpTargets() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_ICMP_TARGET_COUNT, data.getIcmpTargets().size());
         long fieldId = NXCPCodes.VID_ICMP_TARGET_LIST_BASE;
         for(InetAddress a : data.getIcmpTargets())
            msg.setField(fieldId++, a);
      }
      
      if (data.getEtherNetIPPort() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_ETHERNET_IP_PORT, data.getEtherNetIPPort());
      }

      if (data.getEtherNetIPProxy() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_ETHERNET_IP_PROXY, data.getEtherNetIPProxy().intValue());
      }

      if (data.getModbusTcpPort() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_MODBUS_TCP_PORT, data.getModbusTcpPort());
      }

      if (data.getModbusUnitId() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_MODBUS_UNIT_ID, data.getModbusUnitId());
      }

      if (data.getModbusProxy() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_MODBUS_PROXY, data.getModbusProxy().intValue());
      }

      if (data.getCertificateMappingData() != null || data.getCertificateMappingMethod() != null)
      {
         if (data.getCertificateMappingData() == null || data.getCertificateMappingMethod() == null)
            throw new IllegalArgumentException("Both certificate mapping method and certificate mapping data should be set");
         msg.setFieldInt16(NXCPCodes.VID_CERT_MAPPING_METHOD, data.getCertificateMappingMethod().getValue());
         msg.setField(NXCPCodes.VID_CERT_MAPPING_DATA, data.getCertificateMappingData());
      }

      if (data.getCategoryId() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_CATEGORY_ID, data.getCategoryId());
      }

      if (data.getGeoLocationControlMode() != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_GEOLOCATION_CTRL_MODE, data.getGeoLocationControlMode().getValue());
      }

      if (data.getGeoAreas() != null)
      {
         msg.setField(NXCPCodes.VID_GEO_AREAS, data.getGeoAreas());
      }

      if (data.getInstanceDiscoveryMethod() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_INSTD_METHOD, data.getInstanceDiscoveryMethod());
      }

      if (data.getInstanceDiscoveryData() != null)
      {
         msg.setField(NXCPCodes.VID_INSTD_DATA, data.getInstanceDiscoveryData());
      }

      if (data.getInstanceDiscoveryFilter() != null)
      {
         msg.setField(NXCPCodes.VID_INSTD_FILTER, data.getInstanceDiscoveryFilter());
      }

      if (data.getObjectStatusThreshold() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_OBJECT_STATUS_THRESHOLD, data.getObjectStatusThreshold());
      }

      if (data.getDciStatusThreshold() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_DCI_STATUS_THRESHOLD, data.getDciStatusThreshold());
      }

      if (data.getSourceNode() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_NODE_ID, data.getSourceNode().intValue());
      }

      if (data.getWebServiceProxy() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_WEB_SERVICE_PROXY, data.getWebServiceProxy().intValue());
      }

      if (data.getSyslogCodepage() != null)
      {
         msg.setField(NXCPCodes.VID_SYSLOG_CODEPAGE, data.getSyslogCodepage());
      }

      if (data.getSNMPCodepage() != null)
      {
         msg.setField(NXCPCodes.VID_SNMP_CODEPAGE, data.getSNMPCodepage());
      }

      if (data.getDisplayPriority() != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_DISPLAY_PRIORITY, data.getDisplayPriority());
      }

      if (data.getExpectedCapabilities() != null)
      {
         msg.setFieldInt64(NXCPCodes.VID_EXPECTED_CAPABILITIES, data.getExpectedCapabilities());
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
    * @param objectId The object ID
    * @param attrList The attribute list
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setObjectCustomAttributes(final long objectId, final Map<String, CustomAttribute> attrList) throws IOException, NXCException
   {
      NXCObjectModificationData data = new NXCObjectModificationData(objectId);
      data.setCustomAttributes(attrList);
      modifyObject(data);
   }

   /**
    * Change object's ACL (wrapper for modifyObject())
    *
    * @param objectId            The object id
    * @param acl                 The AccessListElements
    * @param inheritAccessRights true if access rights should be inherited
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setObjectACL(long objectId, Collection<AccessListElement> acl, boolean inheritAccessRights) throws IOException, NXCException
   {
      NXCObjectModificationData data = new NXCObjectModificationData(objectId);
      data.setACL(acl);
      data.setInheritAccessRights(inheritAccessRights);
      modifyObject(data);
   }

   /**
    * Enable anonymous access to object. Server will add read access right for user "anonymous" and issue authentication token for
    * that user. Authentication token will be returned by this call and can be used for anonymous login.
    * 
    * @param objectId object ID
    * @return authentication token for anonymous access
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String enableAnonymousObjectAccess(long objectId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_ENABLE_ANONYMOUS_ACCESS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsString(NXCPCodes.VID_TOKEN);
   }

   /**
    * Move object to different zone. Only nodes and clusters can be moved
    * between zones.
    *
    * @param objectId Node or cluster object ID
    * @param zoneUIN  The zone UIN (unique identification number)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void changeObjectZone(long objectId, int zoneUIN) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_ZONE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
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
   public void updateObjectComments(long objectId, String comments) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_OBJECT_COMMENTS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setField(NXCPCodes.VID_COMMENTS, comments);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Update list of responsible users for given object.
    *
    * @param objectId Object's ID
    * @param users New list of responsible users
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateResponsibleUsers(long objectId, List<ResponsibleUser> users) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_RESPONSIBLE_USERS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setFieldInt32(NXCPCodes.VID_RESPONSIBLE_USERS_COUNT, users.size());
      long fieldId = NXCPCodes.VID_RESPONSIBLE_USERS_BASE;
      for(ResponsibleUser r : users)
      {
         r.fillMessage(msg, fieldId);
         fieldId += 10;
      }
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Set object's managed status.
    *
    * @param objectId object's identifier
    * @param isManaged object's managed status
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setObjectManaged(final long objectId, final boolean isManaged) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_OBJECT_MGMT_STATUS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setFieldInt16(NXCPCodes.VID_MGMT_STATUS, isManaged ? 1 : 0);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get effective rights of currently logged in user to given object.
    *
    * @param objectId The object ID
    * @return The effective rights
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int getEffectiveRights(final long objectId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_EFFECTIVE_RIGHTS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
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
   private void changeObjectBinding(long parentId, long childId, boolean bind, boolean removeDci) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(bind ? NXCPCodes.CMD_BIND_OBJECT : NXCPCodes.CMD_UNBIND_OBJECT);
      msg.setFieldUInt32(NXCPCodes.VID_PARENT_ID, parentId);
      msg.setFieldUInt32(NXCPCodes.VID_CHILD_ID, childId);
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
   public void removeTemplate(final long templateId, final long nodeId, final boolean removeDci) throws IOException, NXCException
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
      changeObjectBinding(templateId, nodeId, true, false);
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
      msg.setFieldInt32(NXCPCodes.VID_PARENT_ID, (int)clusterId);
      msg.setFieldInt32(NXCPCodes.VID_CHILD_ID, (int)nodeId);
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
    * Generic implementation for ad-hoc topology map requests.
    *
    * @param nodeId The node ID
    * @param command command to send to server
    * @param pageIdSuffix map page ID suffix
    * @param discoveryRadius topology discovery radios (use -1 to use server default)
    * @param useL1Topology if true server will add known L1 links to the map
    * @return network map page representing requested topology
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private NetworkMapPage queryAdHocTopologyMap(long nodeId, int command, String pageIdSuffix, int discoveryRadius, boolean useL1Topology) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(command);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      msg.setField(NXCPCodes.VID_USE_L1_TOPOLOGY, useL1Topology);
      msg.setFieldInt32(NXCPCodes.VID_DISCOVERY_RADIUS, discoveryRadius);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_OBJECTS);
      long[] idList = response.getFieldAsUInt32Array(NXCPCodes.VID_OBJECT_LIST);
      if (idList.length != count)
         throw new NXCException(RCC.INTERNAL_ERROR);

      NetworkMapPage page = new NetworkMapPage(msg.getMessageId() + pageIdSuffix);
      for(int i = 0; i < count; i++)
         page.addElement(new NetworkMapObject(page.createElementId(), idList[i]));

      count = response.getFieldAsInt32(NXCPCodes.VID_NUM_LINKS);
      long fieldId = NXCPCodes.VID_OBJECT_LINKS_BASE;
      for(int i = 0; i < count; i++, fieldId++)
      {
         NetworkMapObject obj1 = page.findObjectElement(response.getFieldAsInt64(fieldId++));
         NetworkMapObject obj2 = page.findObjectElement(response.getFieldAsInt64(fieldId++));
         int type = response.getFieldAsInt32(fieldId++);
         String port1 = response.getFieldAsString(fieldId++);
         String port2 = response.getFieldAsString(fieldId++);
         String name = response.getFieldAsString(fieldId++);
         int flags = response.getFieldAsInt32(fieldId++);
         long iface1 = response.getFieldAsInt64(fieldId++);
         long iface2 = response.getFieldAsInt64(fieldId++);
         if ((obj1 != null) && (obj2 != null))
         {
            NetworkMapLink link = new NetworkMapLink(page.createLinkId(), name, type, obj1.getId(), iface1, obj2.getId(), iface2, port1, port2, null, flags);
            if ((iface1 > 0) || (iface2 > 0))
               link.setColorSource(NetworkMapLink.COLOR_SOURCE_INTERFACE_STATUS);
            page.addLink(link);
         }
      }
      return page;
   }

   /**
    * Query layer 2 topology for node
    *
    * @param nodeId The node ID
    * @param discoveryRadius topology discovery radios (use -1 to use server default)
    * @param useL1Topology if true server will add known L1 links to the map
    * @return network map page representing layer 2 topology
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public NetworkMapPage queryLayer2Topology(final long nodeId, int discoveryRadius, boolean useL1Topology) throws IOException, NXCException
   {
      return queryAdHocTopologyMap(nodeId, NXCPCodes.CMD_QUERY_L2_TOPOLOGY, ".L2Topology", discoveryRadius, useL1Topology);
   }

   /**
    * Query IP topology for node
    *
    * @param nodeId The node ID
    * @param discoveryRadius topology discovery radios (use -1 to use server default)
    * @return network map page representing IP topology
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public NetworkMapPage queryIPTopology(final long nodeId, int discoveryRadius) throws IOException, NXCException
   {
      return queryAdHocTopologyMap(nodeId, NXCPCodes.CMD_QUERY_IP_TOPOLOGY, ".IPTopology", discoveryRadius, false);
   }

   /**
    * Query OSPF topology for node
    *
    * @param nodeId The node ID
    * @return network map page representing OSPF topology
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public NetworkMapPage queryOSPFTopology(final long nodeId) throws IOException, NXCException
   {
      return queryAdHocTopologyMap(nodeId, NXCPCodes.CMD_QUERY_OSPF_TOPOLOGY, ".OSPFTopology", -1, false);
   }

   /**
    * Query internal connection topology for node
    *
    * @param nodeId The node ID
    * @return network map page representing internal communication topology
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public NetworkMapPage queryInternalConnectionTopology(final long nodeId) throws IOException, NXCException
   {
      return queryAdHocTopologyMap(nodeId, NXCPCodes.CMD_QUERY_INTERNAL_TOPOLOGY, ".InternalConnectionTopology", -1, false);
   }

   /**
    * Execute action on remote agent
    *
    * @param nodeId Node object ID
    * @param alarmId Alarm ID (used for macro expansion)
    * @param action Action with all arguments, that will be expanded and splitted on server side
    * @param inputValues Input values provided by user for expansion
    * @param maskedFields List if input fields whose content should be masked (can be null)
    * @return Expanded action name
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String executeActionWithExpansion(long nodeId, long alarmId, String action, 
         final Map<String, String> inputValues, final List<String> maskedFields)
         throws IOException, NXCException
   {
      return executeActionWithExpansion(nodeId, alarmId, action, false, inputValues, maskedFields, null, null);
   }

   /**
    * Execute action on remote agent
    *
    * @param nodeId Node object ID
    * @param alarmId Alarm ID (used for macro expansion)
    * @param action Action with all arguments, that will be expanded and splitted on server side
    * @param inputValues Input values provided by user for expansion
    * @param maskedFields List if input fields whose content should be masked (can be null)
    * @param receiveOutput true if action's output has to be read
    * @param listener listener for action's output or null
    * @param writer writer for action's output or null
    * @return Expanded action name
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String executeActionWithExpansion(long nodeId, long alarmId, String action, boolean receiveOutput,
         final Map<String, String> inputValues, final List<String> maskedFields, final TextOutputListener listener, final Writer writer)
         throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_ACTION);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_EXPAND_STRING, true);
      msg.setField(NXCPCodes.VID_ACTION_NAME, action);
      msg.setField(NXCPCodes.VID_RECEIVE_OUTPUT, receiveOutput);
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);

      if (inputValues != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_INPUT_FIELD_COUNT, inputValues.size());
         long varId = NXCPCodes.VID_INPUT_FIELD_BASE;
         for(Entry<String, String> e : inputValues.entrySet())
         {
            msg.setField(varId++, e.getKey());
            msg.setField(varId++, e.getValue());
         }
      }
      else
      {
         msg.setFieldInt16(NXCPCodes.VID_INPUT_FIELD_COUNT, 0);
      }

      if ((maskedFields != null) && !maskedFields.isEmpty())
      {
         msg.setFieldsFromStringCollection(maskedFields, NXCPCodes.VID_MASKED_FIELD_LIST_BASE, NXCPCodes.VID_NUM_MASKED_FIELDS);
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
      NXCPMessage result = waitForRCC(msg.getMessageId());

      if (receiveOutput)
      {
         handler.waitForCompletion();
         if (handler.isExpired())
            throw new NXCException(RCC.TIMEOUT);
      }
      return result.getFieldAsString(NXCPCodes.VID_ACTION_NAME);
   }

   /**
    * Execute action on remote agent
    *
    * @param nodeId Node object ID
    * @param action Action name
    * @param args   Action arguments
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeAction(long nodeId, String action, String[] args) throws IOException, NXCException
   {
      executeAction(nodeId, action, args, false, null, null);
   }

   /**
    * Execute action on remote agent
    *
    * @param nodeId        Node object ID
    * @param action        Action name
    * @param args          Action arguments
    * @param receiveOutput true if action's output has to be read
    * @param listener      listener for action's output or null
    * @param writer        writer for action's output or null
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeAction(long nodeId, String action, String[] args, boolean receiveOutput, final TextOutputListener listener, final Writer writer) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_ACTION);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_ACTION_NAME, action);
      msg.setField(NXCPCodes.VID_RECEIVE_OUTPUT, receiveOutput);

      if (args != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_NUM_ARGS, args.length);
         long fieldId = NXCPCodes.VID_ACTION_ARG_BASE;
         for(String a : args)
            msg.setField(fieldId++, a);
      }
      else
      {
         msg.setFieldInt16(NXCPCodes.VID_NUM_ARGS, 0);
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
         handler.waitForCompletion();
         if (handler.isExpired())
            throw new NXCException(RCC.TIMEOUT);
      }
   }

   /**
    * Execute SSH command on remote agent
    *
    * @param nodeId Node object ID
    * @param alarmId Alarm ID (0 if not executed in alarm context)
    * @param command Command to execute
    * @param inputFields Input fields provided by user or null
    * @param maskedFields List of input field names for which values should be masked (can be null)
    * @param receiveOutput true if command's output has to be read
    * @param listener listener for command's output or null
    * @param writer writer for command's output or null
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeSshCommand(long nodeId, long alarmId, String command, Map<String, String> inputFields, List<String> maskedFields, boolean receiveOutput, final TextOutputListener listener,
         final Writer writer) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_SSH_COMMAND);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
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
      if ((maskedFields != null) && !maskedFields.isEmpty())
      {
         msg.setFieldsFromStringCollection(maskedFields, NXCPCodes.VID_MASKED_FIELD_LIST_BASE, NXCPCodes.VID_NUM_MASKED_FIELDS);
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
         handler.waitForCompletion();
         if (handler.isExpired())
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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new PhysicalComponent(response, NXCPCodes.VID_COMPONENT_LIST_BASE, null);
   }

   /**
    * Get device view for node.
    *
    * @param nodeId node object identifier
    * @return device view object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DeviceView getDeviceView(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DEVICE_VIEW);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      sendMessage(msg);
      return new DeviceView(waitForRCC(msg.getMessageId()));
   }

   /**
    * Get list of available Windows performance objects. Returns empty list if node does is not a Windows node or does not have
    * WinPerf subagent installed.
    *
    * @param nodeId node object ID
    * @return list of available Windows performance objects
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<WinPerfObject> getNodeWinPerfObjects(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_WINPERF_OBJECTS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
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
    * Get list of hardware components as reported by agent.
    *
    * @param nodeId node object identifier
    * @return list of hardware components
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<HardwareComponent> getNodeHardwareComponents(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_NODE_HARDWARE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<HardwareComponent> components = new ArrayList<HardwareComponent>(count);
      long baseId = NXCPCodes.VID_ELEMENT_LIST_BASE;

      for(int i = 0; i < count; i++)
      {
         components.add(new HardwareComponent(response, baseId));
         baseId += 64;
      }
      return components;
   }

   /**
    * Get list of user sessions on given node as reported by agent.
    *
    * @param nodeId node object identifier
    * @return list of user sessions
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<UserSession> getUserSessions(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_USER_SESSIONS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_SESSIONS);
      List<UserSession> sessions = new ArrayList<UserSession>(count);
      long baseId = NXCPCodes.VID_SESSION_DATA_BASE;

      for(int i = 0; i < count; i++)
      {
         sessions.add(new UserSession(response, baseId));
         baseId += 64;
      }
      return sessions;
   }

   /**
    * Get list of dependent nodes for given node. Node is considered dependent if it use given node
    * as any type of proxy or as data collection source for at least one DCI.
    *
    * @param nodeId node object ID
    * @return list of dependent node descriptors
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<DependentNode> getDependentNodes(long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DEPENDENT_NODES);
      msg.setFieldInt32(NXCPCodes.VID_NODE_ID, (int)nodeId);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<DependentNode> nodes = new ArrayList<DependentNode>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++, fieldId += 10)
         nodes.add(new DependentNode(msg, fieldId));
      return nodes;
   }

   /**
    * Get state of background task.
    *
    * @param taskId Task ID
    * @return state of background task
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public BackgroundTaskState getBackgroundTaskState(long taskId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_BACKGROUND_TASK_STATE);
      msg.setFieldInt64(NXCPCodes.VID_TASK_ID, taskId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return BackgroundTaskState.getByValue(response.getFieldAsInt32(NXCPCodes.VID_STATE));
   }

   /**
    * Deploy policy on agent
    *
    * @param policyId Policy object ID
    * @param nodeId Node object ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deployAgentPolicy(final long policyId, final long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DEPLOY_AGENT_POLICY);
      msg.setFieldInt32(NXCPCodes.VID_POLICY_ID, (int)policyId);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
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
      msg.setFieldInt32(NXCPCodes.VID_POLICY_ID, (int)policyId);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
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
         policy.addRule(new EventProcessingPolicyRule(response, i + 1));
      }

      return policy;
   }

   /**
    * Get read-only copy of event processing policy.
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
      if (rules.size() > 0)
         waitForRCC(msgId);
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
      return openDataCollectionConfiguration(nodeId, null);
   }

   /**
    * Open data collection configuration for given node. You must call
    * DataCollectionConfiguration.close() to close data collection configuration
    * when it is no longer needed.
    *
    * @param nodeId            Node object identifier
    * @param changeListener callback that will be called when DCO object is changed by server notification
    * @return Data collection configuration object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DataCollectionConfiguration openDataCollectionConfiguration(long nodeId, RemoteChangeListener changeListener)
         throws IOException, NXCException
   {
      final DataCollectionConfiguration cfg = new DataCollectionConfiguration(this, nodeId);
      cfg.open(changeListener);
      return cfg;
   }

   /**
    * Get data collection object without opening data collection configuration.
    *
    * @param objectId object identifier (node or template)
    * @param dciId    data collection item identifier
    * @return Data collection object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DataCollectionObject getDataCollectionObject(long objectId, long dciId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DC_OBJECT);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setFieldUInt32(NXCPCodes.VID_DCI_ID, dciId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int type = response.getFieldAsInt32(NXCPCodes.VID_DCOBJECT_TYPE);
      DataCollectionObject dco;
      switch(type)
      {
         case DataCollectionObject.DCO_TYPE_ITEM:
            dco = new DataCollectionItem(null, response);
            break;
         case DataCollectionObject.DCO_TYPE_TABLE:
            dco = new DataCollectionTable(null, response);
            break;
         default:
            dco = null;
            break;
      }
      return dco;
   }

   /**
    * Modify data collection object without opening data collection configuration.
    *
    * @param dcObject dcObject collection object
    * @return Identifier assigned to newly created object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long modifyDataCollectionObject(DataCollectionObject dcObject) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_NODE_DCI);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, dcObject.getNodeId());
      dcObject.fillMessage(msg);
      sendMessage(msg);
      return waitForRCC(msg.getMessageId()).getFieldAsInt64(NXCPCodes.VID_DCI_ID);
   }

   /**
    * Clear data collection configuration on agent. Will wipe out all configuration and collected data.
    *
    * @param nodeId node object ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void clearAgentDataCollectionConfiguration(final long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_CLEAN_AGENT_DCI_CONF);
      msg.setFieldInt32(NXCPCodes.VID_NODE_ID, (int)nodeId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Change data collection objects status without opening data collection configuration.
    *
    * @param ownerId DCI owner
    * @param items items to change
    * @param status new status 
    * @return return code for each DCI
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long[] changeDCIStatus(final long ownerId, long[] items, DataCollectionObjectStatus status) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_DCI_STATUS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, ownerId);
      msg.setFieldInt16(NXCPCodes.VID_DCI_STATUS, status.getValue());
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, items.length);
      msg.setField(NXCPCodes.VID_ITEM_LIST, items);
      sendMessage(msg);
      return waitForRCC(msg.getMessageId()).getFieldAsUInt32Array(NXCPCodes.VID_ITEM_LIST);
   }

   /**
    * Force re-synchronization of data collection configuration with agent.
    *
    * @param nodeId node object ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void resyncAgentDataCollectionConfiguration(final long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_RESYNC_AGENT_DCI_CONF);
      msg.setFieldUInt32(NXCPCodes.VID_NODE_ID, nodeId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Test DCI transformation script.
    *
    * @param nodeId     ID of the node object to test script on
    * @param script     script source code
    * @param inputValue input value for the script
    * @param dcObject   optional data collection object data (for $dci variable in script)
    * @return test execution results
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public TransformationTestResult testTransformationScript(long nodeId, String script, String inputValue,
         DataCollectionObject dcObject) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_TEST_DCI_TRANSFORMATION);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_SCRIPT, script);
      msg.setField(NXCPCodes.VID_VALUE, inputValue);
      if (dcObject != null)
      {
         dcObject.fillMessage(msg);
      }
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      TransformationTestResult r = new TransformationTestResult();
      r.success = response.getFieldAsBoolean(NXCPCodes.VID_EXECUTION_STATUS);
      r.result = response.getFieldAsString(NXCPCodes.VID_EXECUTION_RESULT);
      return r;
   }

   /**
    * Message handler for script execution updates
    */
   private static class ScriptExecutionUpdateHandler extends MessageHandler
   {
      private TextOutputListener listener;
      private int errorCode = 0;
      private String errorMessage = null;

      ScriptExecutionUpdateHandler(TextOutputListener listener)
      {
         this.listener = listener;
      }

      /**
       * @see org.netxms.client.MessageHandler#processMessage(org.netxms.base.NXCPMessage)
       */
      @Override
      public boolean processMessage(NXCPMessage m)
      {
         int rcc = m.getFieldAsInt32(NXCPCodes.VID_RCC);
         if (rcc != RCC.SUCCESS)
         {
            errorCode = rcc;
            errorMessage = m.getFieldAsString(NXCPCodes.VID_ERROR_TEXT);
            if (errorMessage == null)
               errorMessage = "Unspecified sript execution error (RCC=" + rcc + ")";

            if (listener != null)
            {
               listener.messageReceived(errorMessage + "\n\n");
               listener.onFailure(null);
            }
         }
         else
         {
            String text = m.getFieldAsString(NXCPCodes.VID_MESSAGE);
            if ((text != null) && (listener != null))
            {
               listener.messageReceived(text);
            }
         }

         if (m.isEndOfSequence())
         {
            if (listener != null)
               listener.onSuccess();
            setComplete();
         }
         return true;
      }

      /**
       * Check if error was reported
       * 
       * @return true if error was reported
       */
      public boolean isFailure()
      {
         return errorMessage != null;
      }

      /**
       * Get error code.
       *
       * @return error code
       */
      public int getErrorCode()
      {
         return errorCode;
      }

      /**
       * Get execution error message.
       *
       * @return execution error message
       */
      public String getErrorMessage()
      {
         return errorMessage;
      }
   }

   /**
    * Process server script execution.
    *
    * @param msg prepared request message
    * @param listener script output listener or null if caller not interested in script output
    * @param resultAsMap if true, expect script execution results to be sent as map
    * @return execution result as map if <code>resultAsMap</code> is set to <code>true</true>, and <code>null</code> otherwise
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private Map<String, String> processScriptExecution(NXCPMessage msg, TextOutputListener listener, boolean resultAsMap) throws IOException, NXCException
   {
      msg.setField(NXCPCodes.VID_RESULT_AS_MAP, resultAsMap);

      ScriptExecutionUpdateHandler handler = new ScriptExecutionUpdateHandler(listener);
      handler.setMessageWaitTimeout(commandTimeout);
      addMessageSubscription(NXCPCodes.CMD_EXECUTE_SCRIPT_UPDATE, msg.getMessageId(), handler);

      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());

      if (listener != null)
         listener.setStreamId(response.getFieldAsInt32(NXCPCodes.VID_PROCESS_ID));

      handler.waitForCompletion();
      if (handler.isExpired())
         throw new NXCException(RCC.TIMEOUT);
      if (handler.isFailure())
         throw new NXCException(handler.getErrorCode(), handler.getErrorMessage());

      if (!resultAsMap)
         return null;

      response = waitForMessage(NXCPCodes.CMD_SCRIPT_EXECUTION_RESULT, msg.getMessageId());
      return response.getStringMapFromFields(NXCPCodes.VID_ELEMENT_LIST_BASE, NXCPCodes.VID_NUM_ELEMENTS);
   }

   /**
    * Execute library script on object. Script name interpreted as command line with server-side macro substitution. Map inputValues
    * can be used to pass data for %() macros.
    *
    * @param nodeId      node ID to execute script on
    * @param script      script name and parameters
    * @param inputFields input values map for %() macro substitution (can be null)
    * @param maskedFields List of input field names for which values should be masked (can be null)
    * @param listener    script output listener
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeLibraryScript(long nodeId, String script, Map<String, String> inputFields, List<String> maskedFields, final TextOutputListener listener)
         throws IOException, NXCException
   {
      executeLibraryScript(nodeId, 0, script, inputFields, maskedFields, listener);
   }

   /**
    * Execute library script on object. Script name interpreted as command line with server-side macro substitution. Map inputValues
    * can be used to pass data for %() macros.
    *
    * @param objectId ID of the object to execute script on
    * @param alarmId alarm ID to use for expansion
    * @param script script name and parameters
    * @param inputFields input values map for %() macro substitution (can be null)
    * @param maskedFields List of input field names for which values should be masked (can be null)
    * @param listener script output listener
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeLibraryScript(long objectId, long alarmId, String script, Map<String, String> inputFields, List<String> maskedFields, final TextOutputListener listener) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_LIBRARY_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      msg.setField(NXCPCodes.VID_SCRIPT, script);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int)alarmId);
      msg.setField(NXCPCodes.VID_RECEIVE_OUTPUT, listener != null);
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
      if ((maskedFields != null) && !maskedFields.isEmpty())
      {
         msg.setFieldsFromStringCollection(maskedFields, NXCPCodes.VID_MASKED_FIELD_LIST_BASE, NXCPCodes.VID_NUM_MASKED_FIELDS);
      }
      processScriptExecution(msg, listener, false);
   }

   /**
    * Execute script.
    *
    * @param objectId ID of the object to execute script on
    * @param script script source code
    * @param listener script output listener
    * @param parameters script parameter list (can be null)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeScript(long objectId, String script, String parameters, final TextOutputListener listener) throws IOException, NXCException
   {
      executeScript(objectId, script, parameters, listener, false);
   }

   /**
    * Execute script.
    *
    * @param objectId ID of the object to execute script on
    * @param script script source code
    * @param listener script output listener
    * @param parameters script parameter list (can be null)
    * @param developmentMode true if script should be run in development mode
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeScript(long objectId, String script, String parameters, final TextOutputListener listener, boolean developmentMode) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      msg.setField(NXCPCodes.VID_SCRIPT, script);
      msg.setField(NXCPCodes.VID_DEVELOPMENT_MODE, developmentMode);
      if (parameters != null)
      {
         msg.setField(NXCPCodes.VID_PARAMETER, parameters);
      }
      processScriptExecution(msg, listener, false);
   }

   /**
    * Execute script.
    *
    * @param objectId ID of the object to execute script on
    * @param script script source code
    * @param parameterList script parameter list (can be null)
    * @param listener script output listener
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeScript(long objectId, String script, List<String> parameterList, final TextOutputListener listener) throws IOException, NXCException
   {
      executeScript(objectId, script, parameterList, listener, false);
   }

   /**
    * Execute script.
    *
    * @param objectId ID of the object to execute script on
    * @param script script source code
    * @param parameterList script parameter list (can be null)
    * @param listener script output listener
    * @param developmentMode true if script should be run in development mode
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeScript(long objectId, String script, List<String> parameterList, final TextOutputListener listener, boolean developmentMode) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      msg.setField(NXCPCodes.VID_SCRIPT, script);
      msg.setField(NXCPCodes.VID_DEVELOPMENT_MODE, developmentMode);
      if (parameterList != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_NUM_FIELDS, parameterList.size());
         long fieldId = NXCPCodes.VID_FIELD_LIST_BASE;
         for(String param : parameterList)
         {
            msg.setField(fieldId++, param);
         }
      }
      processScriptExecution(msg, listener, false);
   }

   /**
    * Stop running NXSL script. VM identifier is provided via output listener's method <code>setStreamId</code>.
    *
    * @param vmId NXSL VM identifier
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void stopScript(long vmId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_STOP_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_PROCESS_ID, (int)vmId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Execute script and get return value as map. Content of returned map depends on actual data type of script return value:
    * <ul>
    * <li>For hash map matching map will be returned;
    * <li>For array all elements will be returned as values and keys will be element positions starting as 1;
    * <li>For all other types map will consist of single element with key "1" and script return value as value.
    * </ul>
    * 
    * @param nodeId ID of the node object to test script on
    * @param script script source code
    * @param parameterList script parameter list can be null
    * @param listener script output listener
    * @return script return value as a map
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<String, String> queryScript(long nodeId, String script, List<String> parameterList, final TextOutputListener listener) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_SCRIPT);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_SCRIPT, script);
      if (parameterList != null)
      {
         msg.setFieldInt16(NXCPCodes.VID_NUM_FIELDS, parameterList.size());
         long fieldId = NXCPCodes.VID_FIELD_LIST_BASE;
         for(String param : parameterList)
         {
            msg.setField(fieldId++, param);
         }
      }
      return processScriptExecution(msg, listener, true);
   }

   /**
    * Compile NXSL script on server. Field *success* in compilation result object will indicate compilation status. If compilation
    * fails, field *errorMessage* will contain compilation error message.
    *
    * @param source script source
    * @param serialize flag to indicate if compiled script should be serialized and sent back to client
    * @return script compilation result object
    * @throws IOException if socket I/O error occurs
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
    * @throws IOException if socket I/O error occurs
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
    * Get alarm categories from server
    *
    * @return List of configured alarm categories
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AlarmCategory> getAlarmCategories() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ALARM_CATEGORIES);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      ArrayList<AlarmCategory> list = new ArrayList<AlarmCategory>();
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      for(int i = 0; i < count; i++)
      {
         list.add(new AlarmCategory(response, fieldId));
         fieldId += 10;
      }
      return list;
   }

   /**
    * Add or update alarm category in DB
    *
    * @param object alarm category
    * @return The ID of the category
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long modifyAlarmCategory(AlarmCategory object) throws IOException, NXCException
   {
      if (object.getName().isEmpty())
         return 0;
      NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_ALARM_CATEGORY);
      object.fillMessage(msg);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt32(NXCPCodes.VID_CATEGORY_ID);
   }

   /**
    * Delete alarm category in DB
    *
    * @param id of alarm category
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAlarmCategory(long id) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_ALARM_CATEGORY);
      msg.setFieldInt32(NXCPCodes.VID_CATEGORY_ID, (int)id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Synchronize alarm category configuration. After call to this method
    * session object will maintain internal list of configured alarm categories
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */

   public void syncAlarmCategories() throws IOException, NXCException
   {
      List<AlarmCategory> categories = getAlarmCategories();
      synchronized(alarmCategories)
      {
         alarmCategories.clear();
         for(AlarmCategory c : categories)
         {
            alarmCategories.put(c.getId(), c);
         }
         alarmCategoriesSynchronized = true;
      }
   }

   /**
    * Check if alarm categories are synchronized.
    *
    * @return true if if alarm categories are synchronized
    */
   public boolean isAlarmCategoriesSynchronized()
   {
      return alarmCategoriesSynchronized;
   }

   /**
    * Find alarm category by id in alarm category database internally
    * maintained by session object. You must call
    * NXCSession.syncAlarmCategories() first to make local copy of event template
    * database.
    *
    * @param id alarm category id
    * @return Event template object or null if not found
    */
   public AlarmCategory findAlarmCategoryById(long id)
   {
      synchronized(alarmCategories)
      {
         return alarmCategories.get(id);
      }
   }

   /**
    * Find alarm category by name in alarm category database internally
    * maintained by session object. You must call
    * NXCSession.syncAlarmCategories() first to make local copy of event template
    * database.
    *
    * @param name alarm category name
    * @return alarm category with given name or null if not found
    */
   public AlarmCategory findAlarmCategoryByName(String name)
   {
      synchronized(alarmCategories)
      {
         for(Map.Entry<Long, AlarmCategory> c : alarmCategories.entrySet())
         {
            if (((AlarmCategory)c.getValue()).getName().equals(name))
               return (AlarmCategory)c.getValue();
         }
         return null;
      }
   }

   /**
    * Find multiple alarm categories by category id`s in alarm category database
    * internally maintained by session object. You must call
    * NXCSession.syncAlarmCategories() first to make local copy of alarm category
    * database.
    *
    * @param ids List of alarm category id`s
    * @return List of found alarm categories
    */
   public List<AlarmCategory> findMultipleAlarmCategories(List<Long> ids)
   {
      List<AlarmCategory> list = new ArrayList<AlarmCategory>();
      synchronized(alarmCategories)
      {
         for(Long id : ids)
         {
            AlarmCategory e = alarmCategories.get(id);
            if (e != null)
               list.add(e);
         }
      }
      return list;
   }

   /**
    * Check if event configuratrion objects are synchronized.
    *
    * @return true if event configuratrion objects are synchronized
    */
   public boolean isEventObjectsSynchronized()
   {
      return eventTemplatesSynchronized;
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
         eventTemplatesSynchronized = true;
      }
   }

   /**
    * Re-synchronize event templates in background
    */
   private void resyncEventTemplates()
   {
      new Thread(new Runnable()
      {
         @Override
         public void run()
         {
            try
            {
               syncEventTemplates();
            }
            catch(Exception e)
            {
               logger.error("Exception in worker thread", e);
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
    * NXCSession.syncEventObjects() first to make local copy of event template
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
         for(EventTemplate t : eventTemplates.values())
         {
            if (t.getName().equalsIgnoreCase(name))
            {
               result = t;
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
   public String getEventName(int code)
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
    * NXCSession.syncEventObjects() first to make local copy of event template
    * database.
    *
    * @param code Event code
    * @return Event template object or null if not found
    */
   public EventTemplate findEventTemplateByCode(int code)
   {
      synchronized(eventTemplates)
      {
         return eventTemplates.get(code);
      }
   }

   /**
    * Find multiple event templates by event codes in event template database internally maintained by session object. You must call
    * <code>NXCSession.syncEventObjects()</code> first to make local copy of event template database.
    *
    * @param codes set of event codes
    * @return List of found event templates
    */
   public List<EventTemplate> findMultipleEventTemplates(Collection<Integer> codes)
   {
      List<EventTemplate> list = new ArrayList<EventTemplate>();
      synchronized(eventTemplates)
      {
         for(int code : codes)
         {
            EventTemplate t = eventTemplates.get(code);
            if (t != null)
               list.add(t);
         }
      }
      return list;
   }

   /**
    * Find multiple event templates by event codes in event template database
    * internally maintained by session object. You must call
    * NXCSession.syncEventObjects() first to make local copy of event template
    * database.
    *
    * @param codes List of event codes
    * @return List of found event templates
    */
   public List<EventTemplate> findMultipleEventTemplates(final int[] codes)
   {
      List<EventTemplate> list = new ArrayList<EventTemplate>();
      synchronized(eventTemplates)
      {
         for(int code : codes)
         {
            EventTemplate t = eventTemplates.get(code);
            if (t != null)
               list.add(t);
         }
      }
      return list;
   }

   /**
    * Get event objects from server
    *
    * @return List of configured event objects
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<EventTemplate> getEventTemplates() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_LOAD_EVENT_DB);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_EVENTS);
      long base = NXCPCodes.VID_ELEMENT_LIST_BASE;
      List<EventTemplate> objects = new ArrayList<EventTemplate>(count);
      for(int i = 0; i < count; i++)
      {
         objects.add(new EventTemplate(response, base));
         base += 10;
      }
      return objects;
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
      msg.setFieldInt32(NXCPCodes.VID_EVENT_CODE, (int)eventCode);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Modify event template.
    *
    * @param tmpl Event template
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyEventObject(EventTemplate tmpl) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_EVENT_INFO);
      tmpl.fillMessage(msg);
      sendMessage(msg);
      tmpl.setCode(waitForRCC(msg.getMessageId()).getFieldAsInt32(NXCPCodes.VID_EVENT_CODE));
   }

   /**
    * Send event to server. Event can be identified either by event code or event name. If event name is given, event code will be
    * ignored.
    * <p>
    * Node: sending events by name supported by server version 1.1.8 and higher.
    *
    * @param eventCode event code. Ignored if event name is not null.
    * @param eventName event name. Must be set to null if event identified by code.
    * @param objectId Object ID to send event on behalf of. If set to 0, server will determine object ID by client IP address.
    * @param parameters event's parameters
    * @param paramerNames event's parameter names
    * @param userTag event's user tag
    * @param originTimestamp origin timestamp or null
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void sendEvent(long eventCode, String eventName, long objectId, String[] parameters, String[] paramerNames, String userTag, Date originTimestamp)
         throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_TRAP);
      msg.setFieldInt32(NXCPCodes.VID_EVENT_CODE, (int)eventCode);
      if (eventName != null)
         msg.setField(NXCPCodes.VID_EVENT_NAME, eventName);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      msg.setField(NXCPCodes.VID_USER_TAG, (userTag != null) ? userTag : "");
      if (originTimestamp != null)
         msg.setField(NXCPCodes.VID_ORIGIN_TIMESTAMP, originTimestamp);
      msg.setFieldInt16(NXCPCodes.VID_NUM_ARGS, parameters.length);
      long varId = NXCPCodes.VID_EVENT_ARG_BASE;
      for(int i = 0; i < parameters.length; i++)
      {
         msg.setField(varId++, parameters[i]);
      }
      if (paramerNames != null)
      {
         varId = NXCPCodes.VID_EVENT_ARG_NAMES_BASE;
         for(int i = 0; i < paramerNames.length; i++)
         {
            msg.setField(varId++, paramerNames[i]);
         }
      }
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Convenience wrapper for sendEvent interface.
    *
    * @param eventCode  event code
    * @param parameters event's parameters
    * @param paramerNames event's parameter names
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void sendEvent(long eventCode, String[] parameters, String[] paramerNames) throws IOException, NXCException
   {
      sendEvent(eventCode, null, 0, parameters, paramerNames, null, null);
   }

   /**
    * Convenience wrapper for sendEvent interface.
    *
    * @param eventName  event name
    * @param parameters event's parameters
    * @param paramerNames event's parameter names
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void sendEvent(String eventName, String[] parameters, String[] paramerNames) throws IOException, NXCException
   {
      sendEvent(0, eventName, 0, parameters, paramerNames, null, null);
   }

   /**
    * Get list of well-known SNMP communities configured on server.
    *
    * @return map of SNMP community strings
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<Integer, List<String>> getSnmpCommunities() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_COMMUNITY_LIST);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_STRINGS);
      long stringBase = NXCPCodes.VID_COMMUNITY_STRING_LIST_BASE;
      long zoneBase = NXCPCodes.VID_COMMUNITY_STRING_ZONE_LIST_BASE;
      Map<Integer, List<String>> map = new HashMap<Integer, List<String>>(count);
      List<String> communities = new ArrayList<String>();
      int currentZoneUIN = response.getFieldAsInt32(zoneBase);
      for(int i = 0; i < count; i++)
      {
         int zoneUIN = response.getFieldAsInt32(zoneBase++);
         if (currentZoneUIN != zoneUIN)
         {
            map.put(currentZoneUIN, communities);
            communities = new ArrayList<String>();
            currentZoneUIN = zoneUIN;
         }
         communities.add(response.getFieldAsString(stringBase++));
      }
      if (count > 0)
         map.put(currentZoneUIN, communities);
      return map;
   }

   /**
    * Get list of well-known SNMP communities configured on server.
    *
    * @param zoneUIN Zone UIN (unique identification number)
    * @return list of SNMP community strings
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<String> getSnmpCommunities(int zoneUIN) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_COMMUNITY_LIST);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_STRINGS);
      long stringBase = NXCPCodes.VID_COMMUNITY_STRING_LIST_BASE;
      List<String> stringList = new ArrayList<String>();
      for(int i = 0; i < count; i++)
      {
         stringList.add(response.getFieldAsString(stringBase++));
      }
      return stringList;
   }

   /**
    * Update list of well-known SNMP community strings on server. Existing list will be replaced by provided one.
    *
    * @param zoneUIN Zone UIN (unique identification number)
    * @param communityStrings New list of SNMP community strings
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateSnmpCommunities(int zoneUIN, List<String> communityStrings) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_COMMUNITY_LIST);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
      long stringBase = NXCPCodes.VID_COMMUNITY_STRING_LIST_BASE;
      for(String s : communityStrings)
      {
         msg.setField(stringBase++, s);
      }
      msg.setFieldInt32(NXCPCodes.VID_NUM_STRINGS, communityStrings.size());
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of well-known SNMP USM (user security model) credentials configured on server.
    *
    * @return Map of SNMP USM credentials
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<Integer, List<SnmpUsmCredential>> getSnmpUsmCredentials() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_USM_CREDENTIALS);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
      Map<Integer, List<SnmpUsmCredential>> map = new HashMap<Integer, List<SnmpUsmCredential>>(count);
      List<SnmpUsmCredential> credentials = new ArrayList<SnmpUsmCredential>();
      long fieldId = NXCPCodes.VID_USM_CRED_LIST_BASE;
      int zoneId = 0;
      for(int i = 0; i < count; i++, fieldId += 10)
      {
         SnmpUsmCredential cred = new SnmpUsmCredential(response, fieldId);
         if ((i != 0) && (zoneId != cred.getZoneId()))
         {
            map.put(zoneId, credentials);
            credentials = new ArrayList<SnmpUsmCredential>();
         }
         credentials.add(cred);
         zoneId = cred.getZoneId();
      }
      if (count > 0)
         map.put(zoneId, credentials);

      return map;
   }

   /**
    * Get list of well-known SNMP USM (user security model) credentials configured on server.
    *
    * @param zoneUIN Zone UIN (unique identification number)
    * @return List of configured SNMP USM credentials
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<SnmpUsmCredential> getSnmpUsmCredentials(int zoneUIN) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_USM_CREDENTIALS);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
      List<SnmpUsmCredential> list = new ArrayList<SnmpUsmCredential>(count);
      long varId = NXCPCodes.VID_USM_CRED_LIST_BASE;
      for(int i = 0; i < count; i++, varId += 10)
      {
         list.add(new SnmpUsmCredential(response, varId));
      }

      return list;
   }

   /**
    * Update list of well-known SNMP USM credentials on server. Existing list will be replaced by provided one.
    *
    * @param zoneUIN Zone UIN (unique identification number)
    * @param usmCredentials List of SNMP credentials
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateSnmpUsmCredentials(int zoneUIN, List<SnmpUsmCredential> usmCredentials) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_USM_CREDENTIALS);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
      long varId = NXCPCodes.VID_USM_CRED_LIST_BASE;
      for(SnmpUsmCredential element : usmCredentials)
      {
         element.fillMessage(msg, varId);
         varId += 10;
      }

      msg.setFieldInt32(NXCPCodes.VID_NUM_RECORDS, usmCredentials.size());
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get SSH credentials for all zones.
    * 
    * @return list of SSH credentials for all zones
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<Integer, List<SSHCredentials>> getSshCredentials() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SSH_CREDENTIALS);
      sendMessage(msg);

      NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      Map<Integer, List<SSHCredentials>> credentials = new HashMap<Integer, List<SSHCredentials>>(count);
      if (count > 0)
      {
         List<SSHCredentials> zoneCredentials = new ArrayList<SSHCredentials>();
         long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
         int currentZoneUIN = response.getFieldAsInt32(fieldId);
         for(int i = 0; i < count; i++, fieldId += 10)
         {
            int zoneUIN = response.getFieldAsInt32(fieldId);
            if (zoneUIN != currentZoneUIN)
            {
               credentials.put(currentZoneUIN, zoneCredentials);
               zoneCredentials = new ArrayList<SSHCredentials>();
            }
            zoneCredentials.add(new SSHCredentials(response, fieldId + 1));
         }
         credentials.put(currentZoneUIN, zoneCredentials);
      }
      return credentials;
   }

   /**
    * Get SSH credentials for specific zone.
    * 
    * @param zoneUIN zone UIN
    * @return list of SSH credentials for specific zone
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<SSHCredentials> getSshCredentials(int zoneUIN) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SSH_CREDENTIALS);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
      sendMessage(msg);

      NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<SSHCredentials> credentials = new ArrayList<SSHCredentials>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;

      for(int i = 0; i < count; i++, fieldId += 10)
         credentials.add(new SSHCredentials(response, fieldId));

      return credentials;
   }

   /**
    * Update list of well-known SSH credentials on the server. Existing list will be replaced by the provided one.
    *
    * @param zoneUIN Zone UIN (unique identification number)
    * @param sshCredentials List of SSH credentials
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateSshCredentials(int zoneUIN, List<SSHCredentials> sshCredentials) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_SSH_CREDENTIALS);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(SSHCredentials element : sshCredentials)
      {
         element.fillMessage(msg, fieldId);
         fieldId += 10;
      }
      msg.setFieldInt32(NXCPCodes.VID_NUM_ELEMENTS, sshCredentials.size());
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get agent's master configuration file.
    *
    * @param nodeId Node ID
    * @return Master configuration file of agent running on given node
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String readAgentConfigurationFile(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_READ_AGENT_CONFIG_FILE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
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
   public void writeAgentConfigurationFile(long nodeId, String config, boolean apply) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_WRITE_AGENT_CONFIG_FILE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      msg.setField(NXCPCodes.VID_CONFIG_FILE, config);
      msg.setFieldInt16(NXCPCodes.VID_APPLY_FLAG, apply ? 1 : 0);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of parameters supported by entity defined by origin on given node.
    *
    * @param nodeId Node ID
    * @param origin data origin (agent, driver, etc.)
    * @return List of parameters supported by agent
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AgentParameter> getSupportedParameters(long nodeId, DataOrigin origin) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PARAMETER_LIST);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      msg.setFieldInt16(NXCPCodes.VID_FLAGS, 0x01); // Indicates request for parameters list
      msg.setFieldInt16(NXCPCodes.VID_DCI_SOURCE_TYPE, origin.getValue());
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
    * Get list of SM-CLP properties supported on given node.
    *
    * @param nodeId Node ID
    * @return List of SM-CLP properties supported on given node
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<String> getSmclpSupportedProperties(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SMCLP_PROPERTIES);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      List<String> list;
      if (response.isFieldPresent(NXCPCodes.VID_PROPERTIES))
         list = response.getStringListFromField(NXCPCodes.VID_PROPERTIES);
      else
         list = new ArrayList<String>();
      return list;
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
      return getSupportedParameters(nodeId, DataOrigin.AGENT);
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
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      msg.setFieldInt16(NXCPCodes.VID_FLAGS, 0x02); // Indicates request for table list
      msg.setFieldInt16(NXCPCodes.VID_DCI_SOURCE_TYPE, DataOrigin.AGENT.getValue());
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
   public int[] getRelatedEvents(long objectId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_RELATED_EVENTS_LIST);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      if (response.getFieldAsInt32(NXCPCodes.VID_NUM_EVENTS) == 0)
         return new int[0];
      return response.getFieldAsInt32Array(NXCPCodes.VID_EVENT_LIST);
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
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
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
    * Export server configuration. Returns requested configuration elements exported into XML.
    *
    * @param description Description of exported configuration
    * @param events List of event codes
    * @param traps List of trap identifiers
    * @param templates List of template object identifiers
    * @param rules List of event processing rule GUIDs
    * @param scripts List of library script identifiers
    * @param objectTools List of object tool identifiers
    * @param dciSummaryTables List of DCI summary table identifiers
    * @param actions List of action codes
    * @param webServices List of web service definition id's
    * @param assetAttributes List of asset management atributes to be exported
    * @return file with resulting XML document
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public File exportConfiguration(String description, long[] events, long[] traps, long[] templates, UUID[] rules,
         long[] scripts, long[] objectTools, long[] dciSummaryTables, long[] actions, long[] webServices,
         String[] assetAttributes) throws IOException, NXCException
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
      msg.setFieldInt32(NXCPCodes.VID_NUM_ACTIONS, actions.length);
      msg.setField(NXCPCodes.VID_ACTION_LIST, actions);
      msg.setFieldInt32(NXCPCodes.VID_WEB_SERVICE_DEF_COUNT, webServices.length);
      msg.setField(NXCPCodes.VID_WEB_SERVICE_DEF_LIST, webServices);
      msg.setField(NXCPCodes.VID_ASSET_ATTRIBUTE_NAMES, assetAttributes);

      msg.setFieldInt32(NXCPCodes.VID_NUM_RULES, rules.length);
      long varId = NXCPCodes.VID_RULE_LIST_BASE;
      for(int i = 0; i < rules.length; i++)
      {
         msg.setField(varId++, rules[i]);
      }

      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      final ReceivedFile file = waitForFile(msg.getMessageId(), 60000);
      if (file.isFailed())
      {
         throw new NXCException(RCC.IO_ERROR);
      }
      return file.getFile();
   }

   /**
    * Import server configuration (events, traps, thresholds) from XML document. Please note that for importing large configuration
    * files it is recommended to use variant that accepts file on file system instead of in-memory document.
    *
    * @param config Configuration in XML format
    * @param flags Import flags
    * @throws IOException if socket I/O error occurs
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
    * Import server configuration (events, traps, thresholds) from XML file.
    *
    * @param configFile Configuration file in XML format
    * @param flags Import flags
    * @return Import output messages
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String importConfiguration(File configFile, int flags) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_IMPORT_CONFIGURATION_FILE);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
      sendMessage(msg);
      waitForRCC(msg.getMessageId()); // wait for initial confirmation

      sendFile(msg.getMessageId(), configFile, null, true, 0);
      waitForRCC(msg.getMessageId()); // wait for file transfer completion

      final NXCPMessage response = waitForRCC(msg.getMessageId()); // wait for import completion
      return response.getFieldAsString(NXCPCodes.VID_EXECUTION_RESULT);
   }

   /**
    * Get server stats. Returns set of named properties. The following properties could be found in result set: String: VERSION
    * Integer: UPTIME, SESSION_COUNT, DCI_COUNT, OBJECT_COUNT, NODE_COUNT, PHYSICAL_MEMORY_USED, VIRTUAL_MEMORY_USED,
    * QSIZE_CONDITION_POLLER, QSIZE_CONF_POLLER, QSIZE_DCI_POLLER, QSIZE_DBWRITER, QSIZE_EVENT, QSIZE_DISCOVERY, QSIZE_NODE_POLLER,
    * QSIZE_ROUTE_POLLER, QSIZE_STATUS_POLLER, QSIZE_DCI_CACHE_LOADER, ALARM_COUNT long[]: ALARMS_BY_SEVERITY
    *
    * @return Server stats as set of named properties.
    * @throws IOException if socket I/O error occurs
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
         if (response.getFieldAsInt64(NXCPCodes.VID_ACTION_ID) == 0)
            break; // End of list
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
      msg.setFieldInt32(NXCPCodes.VID_ACTION_ID, (int)actionId);
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

      long fieldId = NXCPCodes.VID_OBJECT_TOOLS_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new ObjectTool(response, fieldId));
         fieldId += 10000;
      }

      return list;
   }

   /**
    * Create object tool tree
    *
    * @param tools list of object tools
    * @return the root folder of the tree
    */
   public ObjectToolFolder createToolTree(List<ObjectTool> tools)
   {
      ObjectToolFolder root = new ObjectToolFolder("[root]");
      Map<String, ObjectToolFolder> folders = new HashMap<String, ObjectToolFolder>();
      for(ObjectTool t : tools)
      {
         ObjectToolFolder folder = root;
         String[] path = t.getName().split("\\-\\>");
         for(int i = 0; i < path.length - 1; i++)
         {
            String key = folder.hashCode() + "@" + path[i].replace("&", "");
            ObjectToolFolder curr = folders.get(key);
            if (curr == null)
            {
               curr = new ObjectToolFolder(path[i]);
               folders.put(key, curr);
               folder.addFolder(curr);
            }
            folder = curr;
         }
         folder.addTool(t);
      }
      return root;
   }

   /**
    * @return root object tool folder
    * @throws IOException  if socker or file I/O error occours
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ObjectToolFolder getObjectToolsAsTree() throws IOException, NXCException
   {
      return createToolTree(getObjectTools());
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
      msg.setFieldInt32(NXCPCodes.VID_TOOL_ID, (int)toolId);
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
      msg.setFieldInt32(NXCPCodes.VID_TOOL_ID, (int)toolId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Enable or disable object tool/
    *
    * @param toolId Object tool ID
    * @param enable true if object tool should be enabled, false if it should be disabled
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void enableObjectTool(long toolId, boolean enable) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_OBJECT_TOOL_STATUS);
      msg.setFieldInt32(NXCPCodes.VID_TOOL_ID, (int)toolId);
      msg.setField(NXCPCodes.VID_STATE, enable);
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
      msg.setFieldInt32(NXCPCodes.VID_TOOL_ID, (int)toolId);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      sendMessage(msg);

      waitForRCC(msg.getMessageId());
      final NXCPMessage response = waitForMessage(NXCPCodes.CMD_TABLE_DATA, msg.getMessageId(), 300000); // wait up to 5 minutes
      return new Table(response);
   }

   /**
    * Execute server command related to given object (usually defined as object tool)
    *
    * @param objectId object ID
    * @param alarmId Alarm ID (0 if executed outside alarm context)
    * @param command command
    * @param inputFields values for input fields (can be null)
    * @param maskedFields List if input fields whose content should be masked (can be null)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeServerCommand(long objectId, long alarmId, String command, Map<String, String> inputFields, List<String> maskedFields) throws IOException, NXCException
   {
      executeServerCommand(objectId, alarmId, command, inputFields, maskedFields, false, null, null);
   }

   /**
    * Execute server command related to given object (usually defined as object tool)
    *
    * @param objectId object ID
    * @param alarmId Alarm ID (0 if executed outside alarm context)
    * @param command command
    * @param inputFields values for input fields (can be null)
    * @param maskedFields List if input fields whose content should be masked (can be null)
    * @param receiveOutput true if command's output has to be read
    * @param listener listener for command's output or null
    * @param writer writer for command's output or null
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void executeServerCommand(long objectId, long alarmId, String command, Map<String, String> inputFields, List<String> maskedFields, 
         boolean receiveOutput, final TextOutputListener listener, final Writer writer) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_SERVER_COMMAND);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_ID, (int)alarmId);
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
      if ((maskedFields != null) && !maskedFields.isEmpty())
      {
         msg.setFieldsFromStringCollection(maskedFields, NXCPCodes.VID_MASKED_FIELD_LIST_BASE, NXCPCodes.VID_NUM_MASKED_FIELDS);
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
            {
               setComplete();
            }
            return true;
         }
      } : null;
      if (receiveOutput)
      {
         handler.setMessageWaitTimeout(serverCommandOutputTimeout);
         addMessageSubscription(NXCPCodes.CMD_COMMAND_OUTPUT, msg.getMessageId(), handler);
      }

      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());

      if (receiveOutput)
      {
         if (listener != null)
            listener.setStreamId(response.getFieldAsInt64(NXCPCodes.VID_COMMAND_ID));

         handler.waitForCompletion();
         if (handler.isExpired())
            throw new NXCException(RCC.TIMEOUT);
      }
   }

   /**
    * Stop server command
    *
    * @param commandId The command ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void stopServerCommand(long commandId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_STOP_SERVER_COMMAND);
      msg.setFieldInt32(NXCPCodes.VID_COMMAND_ID, (int)commandId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get summary of SNMP trap mapping. Trap configurations returned without parameter mapping.
    *
    * @return List of SnmpTrap objects
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
         if (response.getFieldAsInt64(NXCPCodes.VID_TRAP_ID) == 0)
            break; // end of list
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
      msg.setFieldInt32(NXCPCodes.VID_TRAP_ID, (int)trapId);
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
      } while(ttw > 0);

      if (ttw == 0)
         throw new NXCException(RCC.TIMEOUT);

      if (file.getStatus() == NXCReceivedFile.FAILED)
         throw file.getException();

      return file.getFile();
   }

   /**
    * Get predefined graph information by graph id
    *
    * @param graphId graph id
    * @return predefined chart object
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public GraphDefinition getPredefinedGraph(long graphId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_GRAPH);
      msg.setFieldInt32(NXCPCodes.VID_GRAPH_ID, (int)graphId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return GraphDefinition.createGraphSettings(response, NXCPCodes.VID_GRAPH_LIST_BASE);
   }

   /**
    * Get list of predefined graphs or graph templates
    *
    * @param graphTemplates defines if non template or template graph list should re requested
    * @return message with predefined graphs or with template graphs
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<GraphDefinition> getPredefinedGraphs(boolean graphTemplates) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_GRAPH_LIST);
      msg.setField(NXCPCodes.VID_GRAPH_TEMPALTE, graphTemplates);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_GRAPHS);
      List<GraphDefinition> list = new ArrayList<GraphDefinition>(count);
      long fieldId = NXCPCodes.VID_GRAPH_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(GraphDefinition.createGraphSettings(response, fieldId));
         fieldId += 10;
      }
      return list;
   }

   /**
    * Create graph tree from list
    *
    * @param graphs list of predefined graphs
    * @return graph tree
    */
   public static GraphFolder createGraphTree(List<GraphDefinition> graphs)
   {
      GraphFolder root = new GraphFolder("[root]");
      Map<String, GraphFolder> folders = new HashMap<String, GraphFolder>();
      for(GraphDefinition s : graphs)
      {
         GraphFolder folder = root;
         String[] path = s.getName().split("\\-\\>");
         for(int i = 0; i < path.length - 1; i++)
         {
            String key = folder.hashCode() + "@" + path[i].replace("&", "");
            GraphFolder curr = folders.get(key);
            if (curr == null)
            {
               curr = new GraphFolder(path[i]);
               folders.put(key, curr);
               folder.addFolder(curr);
            }
            folder = curr;
         }
         folder.addGraph(s);
      }
      return root;
   }

   /**
    * @return root graph folder
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public GraphFolder getPredefinedGraphsAsTree() throws IOException, NXCException
   {
      return createGraphTree(getPredefinedGraphs(false));
   }

   /**
    * Checks if graph with specified name can be created/overwritten and creates/overwrites it in DB.
    * If graph id is set to 0 it checks if graph with the same name exists, and if yes checks overwrite parameter. If it is
    * set to false, then function returns error that graph with this name already exists.
    * If there is no graph with the same name it just creates a new one.
    * If id is set it checks that provided name is assigned only to this graph and overwrites it or throws error is the
    * same name was already used.
    * Also check if user have permissions to overwrite graph.
    * <p>
    * If it can, then it returns 1.
    * If graph with this name already exists, but can be overwritten by current user function returns 2.
    * If graph with this name already exists, but can not be overwritten by current user function returns 0.
    *
    * @param graph     predefined graph configuration
    * @param overwrite defines if existing graph should be overwritten
    * @return ID of predefined graph object
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long saveGraph(GraphDefinition graph, boolean overwrite) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SAVE_GRAPH);
      graph.fillMessage(msg);
      msg.setField(NXCPCodes.VID_OVERWRITE, overwrite);
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
      msg.setFieldInt32(NXCPCodes.VID_GRAPH_ID, (int)graphId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of all scripts in script library.
    *
    * @return ID/name pairs for scripts in script library
    * @throws IOException  if socket or file I/O error occurs
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
      for(int i = 0; i < count; i++, varId += 2)
      {
         scripts.add(new Script(response, varId));
      }
      return scripts;
   }

   /**
    * Get script from library
    *
    * @param scriptId script ID
    * @return script source code
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Script getScript(long scriptId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_SCRIPT_ID, (int)scriptId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new Script(response);
   }

   /**
    * Modify script. If scriptId is 0, new script will be created in library.
    *
    * @param scriptId script ID
    * @param name     script name
    * @param source   script source code
    * @return script ID (newly assigned if new script was created)
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long modifyScript(long scriptId, String name, String source) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_SCRIPT_ID, (int)scriptId);
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
    * @param name     new script name
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void renameScript(long scriptId, String name) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RENAME_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_SCRIPT_ID, (int)scriptId);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete script from library
    *
    * @param scriptId script ID
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteScript(long scriptId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_SCRIPT);
      msg.setFieldInt32(NXCPCodes.VID_SCRIPT_ID, (int)scriptId);
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
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.isFieldPresent(NXCPCodes.VID_CONNECTION_TYPE) ? new ConnectionPoint(response) : null;
   }

   /**
    * Find node and/or connection point (either directly connected or most close known
    * interface on a switch) for given MAC address. Will return null if
    * no information can be found.
    *
    * @param macAddr MAC address
    * @return connection point information or null
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ConnectionPoint findConnectionPoint(MacAddress macAddr) throws IOException, NXCException
   {
      List<ConnectionPoint> cpl = findConnectionPoints(macAddr.getValue(), 0);
      return cpl.isEmpty() ? null : cpl.get(0);
   }

   /**
    * Find nodes and/or connection points (either directly connected or most close known
    * interface on a switch) for all MAC addresses that match given MAC address pattern.
    * Will return empty list if no information can be found.
    *
    * @param pattern MAC address pattern (1-6 bytes)
    * @param searchLimit limits count of elements in output list
    * @return list of connection point information
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<ConnectionPoint> findConnectionPoints(final byte[] pattern, int searchLimit) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FIND_MAC_LOCATION);
      msg.setField(NXCPCodes.VID_MAC_ADDR, pattern);
      msg.setFieldInt32(NXCPCodes.VID_MAX_RECORDS, searchLimit);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      List<ConnectionPoint> out = new ArrayList<ConnectionPoint>();
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
		long base = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for (int i = 0; i < count; i++, base += 10)
         out.add(new ConnectionPoint(response, base));
      
      if (out.size() == 0)
         out.add(new ConnectionPoint(new MacAddress(pattern)));         

      return out;
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
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneId);
      msg.setField(NXCPCodes.VID_IP_ADDRESS, ipAddr);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.isFieldPresent(NXCPCodes.VID_CONNECTION_TYPE) ? new ConnectionPoint(response) : null;
   }

   /**
    * Find all nodes that contain the primary hostname
    *
    * @param zoneId   zone ID
    * @param hostname Hostname to find
    * @return List of nodes found
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AbstractNode> findNodesByHostname(int zoneId, String hostname) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FIND_HOSTNAME_LOCATION);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneId);
      msg.setField(NXCPCodes.VID_HOSTNAME, hostname);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      long base = NXCPCodes.VID_ELEMENT_LIST_BASE;
      List<AbstractNode> nodes = new ArrayList<AbstractNode>();

      for(int i = 0; i < count; i++)
      {
         nodes.add((AbstractNode)findObjectById(response.getFieldAsInt32(base++)));
      }
      return nodes;
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
      if (!connected || disconnected)
         return false;

      final NXCPMessage msg = newMessage(NXCPCodes.CMD_KEEPALIVE);
      try
      {
         sendMessage(msg);
         waitForRCC(msg.getMessageId());
         return true;
      }
      catch(Exception e)
      {
         logger.debug("Connection check error", e);
         synchronized(this)
         {
            if (socket != null)
            {
               try
               {
                  socket.close();
               }
               catch(IOException eio)
               {
               }
            }

            // Socket close should cause receiver thread to reconnect; after completion, reconnect will signal session object
            if (reconnectEnabled)
            {
               logger.debug("Reconnect is enabled, wait for reconnect attempt");
               try
               {
                  wait();
               }
               catch(InterruptedException ei)
               {
               }

               if (connected && !disconnected)
                  return true;
            }
         }
         sendNotification(new SessionNotification(SessionNotification.CONNECTION_BROKEN));
         return false;
      }
   }

   /**
    * Get the whole image library
    *
    * @return List of library images
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<LibraryImage> getImageLibrary() throws IOException, NXCException
   {
      return getImageLibrary(null);
   }

   /**
    * Get the image library of specific category
    *
    * @param category The name of the category
    * @return List of library images
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
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

      final int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);

      final List<LibraryImage> images = new ArrayList<LibraryImage>(count);
      long fieldId = NXCPCodes.VID_IMAGE_LIST_BASE;
      for(int i = 0; i < count; i++, fieldId += 5)
      {
         images.add(new LibraryImage(response, fieldId));
      }

      return images;
   }

   /**
    * Get an image from the library
    *
    * @param guid UUID of the image
    * @return The image
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public LibraryImage getImage(UUID guid) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_IMAGE);
      msg.setField(NXCPCodes.VID_GUID, guid);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      final ReceivedFile imageFile = waitForFile(msg.getMessageId(), 60000);
      if (imageFile.isFailed())
         throw new NXCException(RCC.IO_ERROR);
      return new LibraryImage(response, imageFile.getFile());
   }

   /**
    * Create an image
    *
    * @param image    The Image
    * @param listener The ProgressListener
    * @return The image created
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public LibraryImage createImage(LibraryImage image, ProgressListener listener) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_IMAGE);
      image.fillMessage(msg);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      final UUID imageGuid = response.getFieldAsUUID(NXCPCodes.VID_GUID);
      image.setGuid(imageGuid);

      sendFile(msg.getMessageId(), image.getBinaryData(), listener, allowCompression);

      waitForRCC(msg.getMessageId());

      return image;
   }

   /**
    * Delete an image
    *
    * @param image The image to delete
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteImage(LibraryImage image) throws IOException, NXCException
   {
      if (image.isProtected())
         throw new NXCException(RCC.PROTECTED_IMAGE);
      deleteImage(image.getGuid());
   }

   /**
    * Delete an image
    *
    * @param guid ID of image to delete
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteImage(UUID guid) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_IMAGE);
      msg.setField(NXCPCodes.VID_GUID, guid);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Modify an image
    *
    * @param image    The image to modify
    * @param listener The ProgressListener
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyImage(LibraryImage image, ProgressListener listener) throws IOException, NXCException
   {
      if (image.isProtected())
      {
         throw new NXCException(RCC.PROTECTED_IMAGE);
      }

      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_IMAGE);
      image.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());

      sendFile(msg.getMessageId(), image.getBinaryData(), listener, allowCompression);

      waitForRCC(msg.getMessageId());
   }

   /**
    * Perform a forced object poll. This method will not return until poll is complete, so it's advised to run it from separate
    * thread. For each message received from poller listener's method onPollerMessage will be called.
    *
    * @param objectId object ID
    * @param pollType poll type
    * @param listener text output listener (can be null)
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void pollObject(long objectId, ObjectPollType pollType, final TextOutputListener listener) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_POLL_OBJECT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
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

      try 
      {
         sendMessage(msg);
         try
         {
            waitForRCC(msg.getMessageId());
         }
         catch(NXCException e)
         {
            removeMessageSubscription(NXCPCodes.CMD_POLLING_INFO, msg.getMessageId());
            throw e;
         }
   
         handler.waitForCompletion();
         if (handler.isExpired())
            throw new NXCException(RCC.TIMEOUT);
         if (listener != null)
            listener.onSuccess();
      }
      catch (Exception e)
      {

         if (listener != null)
            listener.onFailure(e);
         throw e;
      }
   }

   /**
    * Get list of all values in persistent storage
    *
    * @return Hash map wit persistent storage key, value
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public HashMap<String, String> getPersistentStorageList() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PERSISTENT_STORAGE);
      sendMessage(msg);

      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_PSTORAGE);
      HashMap<String, String> map = new HashMap<String, String>();
      long base = NXCPCodes.VID_PSTORAGE_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         map.put(response.getFieldAsString(base++), response.getFieldAsString(base++));
      }
      return map;
   }

   /**
    * Set persistent storage value. Will create new or update existing
    *
    * @param key   unique key of persistent storage value
    * @param value value
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setPersistentStorageValue(String key, String value) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_PSTORAGE_VALUE);
      msg.setField(NXCPCodes.VID_PSTORAGE_KEY, key);
      msg.setField(NXCPCodes.VID_PSTORAGE_VALUE, value);
      sendMessage(msg);
   }

   /**
    * Delete persistent storage value
    *
    * @param key unique key of persistent storage value
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deletePersistentStorageValue(String key) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_PSTORAGE_VALUE);
      msg.setField(NXCPCodes.VID_PSTORAGE_KEY, key);
      sendMessage(msg);
   }

   /**
    * List custom MIB files in server's file store.
    *
    * @return list of MIB files in server's file store
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ServerFile[] listMibFiles() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LIST_SERVER_FILES);
      msg.setField(NXCPCodes.VID_MIB_FILE, true);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_INSTANCE_COUNT);
      ServerFile[] files = new ServerFile[count];
      long fieldId = NXCPCodes.VID_INSTANCE_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         files[i] = new ServerFile(response, fieldId);
         fieldId += 10;
      }
      return files;
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
    *               applyed.
    * @return list of files in server's file store
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public ServerFile[] listServerFiles(String[] filter) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LIST_SERVER_FILES);
      if (filter != null)
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
      long fieldId = NXCPCodes.VID_INSTANCE_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         files[i] = new ServerFile(response, fieldId);
         fieldId += 10;
      }
      return files;
   }

   /**
    * List files on agent file store.
    *
    * @param file     parent of new coomming list
    * @param fullPath path that will be used on an agent to get list of subfiles
    * @param objectId the ID of the node
    * @return will return the list of sub files or the list of allowed folders if full path is set to "/"
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AgentFile> listAgentFiles(AgentFile file, String fullPath, long objectId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_FOLDER_CONTENT);
      msg.setField(NXCPCodes.VID_FILE_NAME, fullPath);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setField(NXCPCodes.VID_ROOT, file == null);
      msg.setField(NXCPCodes.VID_ALLOW_MULTIPART, true);
      sendMessage(msg);

      List<AgentFile> files = new ArrayList<AgentFile>(64);
      while(true)
      {
         final NXCPMessage response = waitForRCC(msg.getMessageId());
         int count = response.getFieldAsInt32(NXCPCodes.VID_INSTANCE_COUNT);
         long fieldId = NXCPCodes.VID_INSTANCE_LIST_BASE;
         for(int i = 0; i < count; i++)
         {
            files.add(new AgentFile(response, fieldId, file, objectId));
            fieldId += 10;
         }
         if (!response.getFieldAsBoolean(NXCPCodes.VID_ALLOW_MULTIPART))
            return files;  // old version of server or agent without multipart support
         if (response.isEndOfSequence())
            break;
      }
      waitForRCC(msg.getMessageId());
      return files;
   }

   /**
    * Return information about agent file
    *
    * @param file The AgentFile in question
    * @return AgentFileInfo object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public AgentFileInfo getAgentFileInfo(AgentFile file) throws IOException, NXCException
   {
      if (!file.isDirectory())
         return new AgentFileInfo(file.getName(), 0, file.getSize());

      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_FOLDER_SIZE);
      msg.setField(NXCPCodes.VID_FILE_NAME, file.getFullName());
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, file.getNodeId());
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new AgentFileInfo(file.getName(), response.getFieldAsInt64(NXCPCodes.VID_FILE_COUNT),
            response.getFieldAsInt64(NXCPCodes.VID_FOLDER_SIZE));
   }

   /**
    * Start file upload from server's file store to agent. Returns ID of background task.
    *
    * @param nodeId node object ID
    * @param serverFileName file name in server's file store
    * @param remoteFileName fully qualified file name on target system or null to upload file to agent's file store
    * @return ID of background task
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long uploadFileToAgent(long nodeId, String serverFileName, String remoteFileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPLOAD_FILE_TO_AGENT);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, serverFileName);
      if (remoteFileName != null)
         msg.setField(NXCPCodes.VID_DESTINATION_FILE_NAME, remoteFileName);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_TASK_ID);
   }

   /**
    * Upload custom MIB file to server's file store
    *
    * @param localFile local file
    * @param serverFileName name under which file will be stored on server
    * @param listener The ProgressListener to set
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void uploadMibFile(File localFile, String serverFileName, ProgressListener listener) throws IOException, NXCException
   {
      uploadFileToServer(localFile, serverFileName, listener, true);
   }

   /**
    * Upload local file to server's file store
    *
    * @param localFile local file
    * @param serverFileName name under which file will be stored on server
    * @param listener The ProgressListener to set
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void uploadFileToServer(File localFile, String serverFileName, ProgressListener listener) throws IOException, NXCException
   {
      uploadFileToServer(localFile, serverFileName, listener, false);
   }

   /**
    * Upload local file to server's file store
    *
    * @param localFile local file
    * @param serverFileName name under which file will be stored on server
    * @param listener The ProgressListener to set
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private void uploadFileToServer(File localFile, String serverFileName, ProgressListener listener, boolean mibFile) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPLOAD_FILE);
      if ((serverFileName == null) || serverFileName.isEmpty())
      {
         serverFileName = localFile.getName();
      }
      msg.setField(NXCPCodes.VID_FILE_NAME, serverFileName);
      msg.setField(NXCPCodes.VID_MODIFICATION_TIME, new Date(localFile.lastModified()));
      msg.setField(NXCPCodes.VID_MIB_FILE, mibFile);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      sendFile(msg.getMessageId(), localFile, listener, allowCompression, 0);
   }

   /**
    * Calculate MD5 hash of given file or it's part
    * 
    * @param file file to calculate hash for
    * @param size size of file part to calculate hash for
    * @return MD5 hash
    * @throws IOException
    * @throws NoSuchAlgorithmException if MD5 algorithm is not available
    */
   private static byte[] calculateFileHash(File file, long size) throws IOException, NoSuchAlgorithmException
   {
      InputStream in = new FileInputStream(file);
      byte[] buffer = new byte[1024];
      MessageDigest hash = MessageDigest.getInstance("MD5");
      long numRead = 0;
      while((numRead = in.read(buffer)) != -1 && size > 0)
      {
         hash.update(buffer, 0, (int)Math.min(numRead, size));
         size -= numRead;
      }
      in.close();
      return hash.digest();
   }

   /**
    * Upload local file to remote node via agent. If remote file name is not provided local file name will be used.
    *
    * @param nodeId node object ID
    * @param localFile local file
    * @param remoteFileName remote file name (can be null or empty)
    * @param overwrite "allow overwrite" flag (if set to true, agent will overwrite existing file)
    * @param listener progress listener (can be null)
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void uploadLocalFileToAgent(long nodeId, File localFile, String remoteFileName, boolean overwrite, ProgressListener listener) throws IOException, NXCException
   {
      // Possible resume modes
      // 0 - overwrite
      // 1 - check if resume is possible
      // 2 - resume file transfer (append to existing file part)
      int resumeMode = 1; 
      long offset = 0;
      NXCPMessage msg = null;
      NXCPMessage response = null;
      boolean messageResendRequired;
      do 
      {
         messageResendRequired = false;
         msg = newMessage(NXCPCodes.CMD_FILEMGR_UPLOAD);
         if ((remoteFileName == null) || remoteFileName.isEmpty())
         {
            remoteFileName = localFile.getName();
         }
         msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
         msg.setField(NXCPCodes.VID_FILE_NAME, remoteFileName);
         msg.setField(NXCPCodes.VID_MODIFICATION_TIME, new Date(localFile.lastModified()));
         msg.setField(NXCPCodes.VID_OVERWRITE, overwrite);
         msg.setField(NXCPCodes.VID_REPORT_PROGRESS, true); // Indicate that client can accept intermediate progress reports
         msg.setFieldInt16(NXCPCodes.VID_RESUME_MODE, resumeMode);
         sendMessage(msg);
         response = waitForRCC(msg.getMessageId());
         if (response.getFieldAsInt32(NXCPCodes.VID_RCC) == RCC.FILE_APPEND_POSSIBLE)
         {
            byte[] remoteFileHash = response.getFieldAsBinary(NXCPCodes.VID_HASH_MD5);
            long remoteFileSize = response.getFieldAsInt32(NXCPCodes.VID_FILE_SIZE);
            byte[] localFileHash = new byte[1];
            try
            {
               localFileHash = calculateFileHash(localFile, remoteFileSize);
            }
            catch(NoSuchAlgorithmException e)
            {
               messageResendRequired = true;
               resumeMode = 0; // Overwrite existing file
            }

            if (Arrays.equals(remoteFileHash, localFileHash))
            {
               // Even if are equals .part file still might need rename
               messageResendRequired = true;
               resumeMode = 2; // Resume file transfer
               offset = remoteFileSize;
            }
            else
            {
               messageResendRequired = true;
               resumeMode = 0;
            }
         }
      } while (messageResendRequired);

      boolean serverSideProgressReport = response.getFieldAsBoolean(NXCPCodes.VID_REPORT_PROGRESS);
      sendFile(msg.getMessageId(), localFile, serverSideProgressReport ? null : listener, response.getFieldAsBoolean(NXCPCodes.VID_ENABLE_COMPRESSION), offset);
      if (serverSideProgressReport)
      {
         // Newer protocol variant, receive progress updates from server
         if (listener != null)
            listener.setTotalWorkAmount(localFile.length());
         while(true)
         {
            response = waitForRCC(msg.getMessageId(), 900000);
            if (response.isEndOfSequence())
               break;
            if (listener != null)
               listener.markProgress(response.getFieldAsInt64(NXCPCodes.VID_FILE_SIZE));
         }
      }
      else
      {
         // Older protocol variant, just wait for final confirmation from server
         waitForRCC(msg.getMessageId(), 900000);
      }
   }

   /**
    * Create folder on remote system via agent
    *
    * @param nodeId node object ID
    * @param folder folder name
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void createFolderOnAgent(long nodeId, String folder) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_CREATE_FOLDER);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, folder);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Notify progress listener
    *
    * @param id     id
    * @param length lenght
    */
   void notifyProgressListener(long id, int length)
   {
      synchronized(progressListeners)
      {
         ProgressListener listener = progressListeners.get(id);
         if (listener != null)
            listener.markProgress(length);
      }
   }

   /**
    * Remove progress listener when download is done
    *
    * @param id
    */
   void removeProgressListener(long id)
   {
      synchronized(progressListeners)
      {
         progressListeners.remove(id);
      }
   }

   /**
    * Download file from remote host via agent. If <code>maxFileSize</code> is set to non-zero value then last
    * <code>maxFileSize</code> bytes will be retrieved.
    *
    * @param nodeId node object ID
    * @param remoteFileName fully qualified file name on remote system
    * @param maxFileSize maximum download size, 0 == UNLIMITED
    * @param follow if set to true, server will send file updates as they appear (like for tail -f command)
    * @param listener The ProgressListener to set
    * @return agent file handle which contains server assigned ID and handle for local file
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public AgentFileData downloadFileFromAgent(long nodeId, String remoteFileName, long maxFileSize, boolean follow,
         ProgressListener listener) throws IOException, NXCException
   {
      return downloadFileFromAgent(nodeId, remoteFileName, false, 0, null, maxFileSize, follow, listener);
   }

   /**
    * Download file from remote host via agent. If <code>maxFileSize</code> is set to non-zero value then last
    * <code>maxFileSize</code> bytes will be retrieved.
    *
    * @param nodeId node object ID
    * @param remoteFileName fully qualified file name on remote system
    * @param expandMacros if true, macros in remote file name will be expanded on server side
    * @param alarmId alarm ID used for macro expansion
    * @param inputValues input field values for macro expansion (can be null if none provided)
    * @param maxFileSize maximum download size, 0 == UNLIMITED
    * @param follow if set to true, server will send file updates as they appear (like for tail -f command)
    * @param listener The ProgressListener to set
    * @return agent file handle which contains server assigned ID and handle for local file
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public AgentFileData downloadFileFromAgent(long nodeId, String remoteFileName, boolean expandMacros, long alarmId,
         Map<String, String> inputValues, long maxFileSize, boolean follow, ProgressListener listener) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AGENT_FILE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, remoteFileName);
      msg.setFieldUInt32(NXCPCodes.VID_FILE_SIZE_LIMIT, maxFileSize);
      msg.setField(NXCPCodes.VID_FILE_FOLLOW, follow);
      msg.setField(NXCPCodes.VID_EXPAND_STRING, expandMacros);
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
      if (inputValues != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_INPUT_FIELD_COUNT, inputValues.size());
         long varId = NXCPCodes.VID_INPUT_FIELD_BASE;
         for(Entry<String, String> e : inputValues.entrySet())
         {
            msg.setField(varId++, e.getKey());
            msg.setField(varId++, e.getValue());
         }
      }
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId()); // first confirmation - server job started
      if (listener != null)
      {
         final long fileSize = response.getFieldAsInt64(NXCPCodes.VID_FILE_SIZE);
         listener.setTotalWorkAmount(fileSize);
         synchronized(progressListeners)
         {
            progressListeners.put(msg.getMessageId(), listener);
         }
      }

      ReceivedFile remoteFile = waitForFile(msg.getMessageId(), 120000); // 120 seconds timeout for file content
      if (remoteFile.isFailed())
         throw new NXCException(RCC.AGENT_FILE_DOWNLOAD_ERROR);

      try
      {
         waitForRCC(msg.getMessageId()); // second confirmation - file transfered from agent to console
      }
      finally
      {
         removeProgressListener(msg.getMessageId());
      }

      return new AgentFileData(response.getFieldAsString(NXCPCodes.VID_NAME), response.getFieldAsString(NXCPCodes.VID_FILE_NAME), remoteFile.getFile(),
            response.getFieldAsUUID(NXCPCodes.VID_MONITOR_ID));
   }

   /**
    * Download file from server file storage.
    *
    * @param remoteFileName fully qualified file name on remote system
    * @return The downloaded file
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public File downloadFileFromServer(String remoteFileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SERVER_FILE);
      msg.setField(NXCPCodes.VID_FILE_NAME, remoteFileName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      return waitForFile(msg.getMessageId(), 60000).getFile();
   }

   /**
    * Cancel file monitoring
    *
    * @param monitorId file monitor ID (previously returned by <code>downloadFileFromAgent</code>
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void cancelFileMonitoring(UUID monitorId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CANCEL_FILE_MONITORING);
      msg.setField(NXCPCodes.VID_MONITOR_ID, monitorId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get file fingerprint from remote host via agent.
    *
    * @param nodeId node object ID
    * @param remoteFileName fully qualified file name on remote system
    * @return agent file fingerprint which contains size, hashes and partial data for the file
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public AgentFileFingerprint getAgentFileFingerprint(long nodeId, String remoteFileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_GET_FILE_FINGERPRINT);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, remoteFileName);
      sendMessage(msg);
      return new AgentFileFingerprint(waitForRCC(msg.getMessageId()));
   }

   /**
    * Rename file in server's file store
    *
    * @param oldFileName name of existing server file
    * @param newFileName new name for selected file
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void renameServerFile(String oldFileName, String newFileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RENAME_FILE);
      msg.setField(NXCPCodes.VID_FILE_NAME, oldFileName);
      msg.setField(NXCPCodes.VID_NEW_FILE_NAME, newFileName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete custom MIB file from server's file store
    *
    * @param serverFileName name of server file
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteMibFile(String serverFileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_FILE);
      msg.setField(NXCPCodes.VID_FILE_NAME, serverFileName);
      msg.setField(NXCPCodes.VID_MIB_FILE, true);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete file from server's file store
    *
    * @param serverFileName name of server file
    * @throws IOException if socket or file I/O error occurs
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
    * @param nodeId   node id
    * @param fileName full path to file
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAgentFile(long nodeId, String fileName) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_DELETE_FILE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, fileName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Rename agent's file
    *
    * @param nodeId      node id
    * @param oldName     old file path
    * @param newFileName new file path
    * @param overwrite   should the file in destination be overwritten
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void renameAgentFile(long nodeId, String oldName, String newFileName, boolean overwrite) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_RENAME_FILE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, oldName);
      msg.setField(NXCPCodes.VID_NEW_FILE_NAME, newFileName);
      msg.setField(NXCPCodes.VID_OVERWRITE, overwrite);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Move file from agent
    *
    * @param nodeId      node id
    * @param oldName     old file path
    * @param newFileName new file path
    * @param overwrite   should the file in destination be overwritten
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void moveAgentFile(long nodeId, String oldName, String newFileName, boolean overwrite) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_MOVE_FILE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, oldName);
      msg.setField(NXCPCodes.VID_NEW_FILE_NAME, newFileName);
      msg.setField(NXCPCodes.VID_OVERWRITE, overwrite);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Copy file from agent
    *
    * @param nodeId      node id
    * @param oldName     old file path
    * @param newFileName new file path
    * @param overwrite   should the file in destination be overwritten
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void copyAgentFile(long nodeId, String oldName, String newFileName, boolean overwrite) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FILEMGR_COPY_FILE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_FILE_NAME, oldName);
      msg.setField(NXCPCodes.VID_NEW_FILE_NAME, newFileName);
      msg.setField(NXCPCodes.VID_OVERWRITE, overwrite);
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
      synchronized(serverConsoleConnectionLock)
      {
         if (serverConsoleConnectionCount == 0)
         {
            final NXCPMessage msg = newMessage(NXCPCodes.CMD_OPEN_CONSOLE);
            sendMessage(msg);
            waitForRCC(msg.getMessageId());
            serverConsoleConnected = true;
         }
         serverConsoleConnectionCount++;
      }
   }

   /**
    * Close server console.
    *
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void closeConsole() throws IOException, NXCException
   {
      synchronized(serverConsoleConnectionLock)
      {
         if (serverConsoleConnectionCount > 0)
         {
            if (serverConsoleConnectionCount == 1)
            {
               final NXCPMessage msg = newMessage(NXCPCodes.CMD_CLOSE_CONSOLE);
               sendMessage(msg);
               waitForRCC(msg.getMessageId());
               serverConsoleConnected = false;
            }
            serverConsoleConnectionCount--;
         }
      }
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
   public void snmpWalk(long nodeId, SnmpObjectId rootOid, SnmpWalkListener listener) throws IOException, NXCException
   {
      snmpWalk(nodeId, rootOid.toString(), listener);
   }

   /**
    * Do SNMP walk. Operation will start at given root object, and callback will be called one or more times as data will come from
    * server. This method will exit only when walk operation is complete.
    *
    * @param nodeId node object ID
    * @param rootOid root SNMP object ID (as text)
    * @param listener listener
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void snmpWalk(long nodeId, String rootOid, SnmpWalkListener listener) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_START_SNMP_WALK);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_SNMP_OID, rootOid);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      while(true)
      {
         final NXCPMessage response = waitForMessage(NXCPCodes.CMD_SNMP_WALK_DATA, msg.getMessageId());
         final int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_VARIABLES);
         final List<SnmpValue> data = new ArrayList<SnmpValue>(count);
         long fieldId = NXCPCodes.VID_SNMP_WALKER_DATA_BASE;
         for(int i = 0; i < count; i++)
         {
            final String name = response.getFieldAsString(fieldId++);
            final int type = response.getFieldAsInt32(fieldId++);
            final String value = response.getFieldAsString(fieldId++);
            final byte[] rawValue = response.getFieldAsBinary(fieldId++);
            data.add(new SnmpValue(name, type, value, rawValue, nodeId));
         }
         listener.onSnmpWalkData(nodeId, data);
         if (response.isEndOfSequence())
            break;
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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_VLANS);
      List<VlanInfo> vlans = new ArrayList<VlanInfo>(count);
      long fieldId = NXCPCodes.VID_VLAN_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         VlanInfo v = new VlanInfo(response, fieldId);
         vlans.add(v);
         fieldId = v.getNextFieldId();
      }
      return vlans;
   }

   /**
    * @return the objectsSynchronized
    */
   public boolean areObjectsSynchronized()
   {
      return objectsSynchronized;
   }

   /**
    * Get address list.
    *
    * @param listId list identifier (defined in NXCSession as ADDRESS_LIST_xxx)
    * @return address list
    * @throws IOException  if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<InetAddressListElement> getAddressList(int listId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ADDR_LIST);
      msg.setFieldInt32(NXCPCodes.VID_ADDR_LIST_TYPE, listId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
      final List<InetAddressListElement> list = new ArrayList<InetAddressListElement>(count);
      long varId = NXCPCodes.VID_ADDR_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new InetAddressListElement(response, varId));
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
   public void setAddressList(int listId, List<InetAddressListElement> list) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_ADDR_LIST);
      msg.setFieldInt32(NXCPCodes.VID_ADDR_LIST_TYPE, listId);
      msg.setFieldInt32(NXCPCodes.VID_NUM_RECORDS, list.size());
      long fieldId = NXCPCodes.VID_ADDR_LIST_BASE;
      for(InetAddressListElement e : list)
      {
         e.fillMessage(msg, fieldId);
         fieldId += 10;
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
      msg.setFieldUInt32(NXCPCodes.VID_SOURCE_OBJECT_ID, node1);
      msg.setFieldUInt32(NXCPCodes.VID_DESTINATION_OBJECT_ID, node2);
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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId(), commandTimeout * 10); // Routing table may take a long time to retrieve
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<Route> rt = new ArrayList<Route>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         rt.add(new Route(response, fieldId));
         fieldId += 10;
      }
      return rt;
   }

   /**
    * Get ARP cache from node
    *
    * @param nodeId node object ID
    * @param forceRead if true, ARP cache will be read from node, otherwise cached version may be returned
    * @return list of ARP cache entries
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<ArpCacheEntry> getArpCache(long nodeId, boolean forceRead) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ARP_CACHE);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
      msg.setField(NXCPCodes.VID_FORCE_RELOAD, forceRead);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<ArpCacheEntry> arpCache = new ArrayList<ArpCacheEntry>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         arpCache.add(new ArpCacheEntry(response, fieldId));
         fieldId += 10;
      }
      return arpCache;
   }

   /**
    * Get OSPF information for given node.
    *
    * @param nodeId node object ID
    * @return OSPF information
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public OSPFInfo getOSPFInfo(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_OSPF_DATA);
      msg.setFieldUInt32(NXCPCodes.VID_NODE_ID, nodeId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new OSPFInfo(response);
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
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, nodeId);
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
    * Get list of wireless stations registered at given wireless controller or access point.
    *
    * @param objectId controller node ID or access point ID
    * @return list of wireless stations
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<WirelessStation> getWirelessStations(long objectId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_WIRELESS_STATIONS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<WirelessStation> stations = new ArrayList<WirelessStation>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         stations.add(new WirelessStation(response, fieldId));
         fieldId += 10;
      }
      return stations;
   }

   /**
    * Add controller node to wireless domain.
    *
    * @param wirelessDomainId wireless domain object ID
    * @param nodeId node object ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void addWirelessDomainController(long wirelessDomainId, long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_ADD_WIRELESS_DOMAIN_CNTRL);
      msg.setFieldUInt32(NXCPCodes.VID_DOMAIN_ID, wirelessDomainId);
      msg.setFieldUInt32(NXCPCodes.VID_NODE_ID, nodeId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Remove controller node from wireless domain.
    *
    * @param wirelessDomainId wireless domain object ID
    * @param nodeId node object ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void removeWirelessDomainController(long wirelessDomainId, long nodeId) throws IOException, NXCException
   {
      changeObjectBinding(wirelessDomainId, nodeId, false, false);
   }

   /**
    * Remove agent package from server
    *
    * @param packageId The package ID
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void removePackage(long packageId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_REMOVE_PACKAGE);
      msg.setFieldInt32(NXCPCodes.VID_PACKAGE_ID, (int)packageId);
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
   public long installPackage(PackageInfo info, File pkgFile, ProgressListener listener) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_INSTALL_PACKAGE);
      info.fillMessage(msg);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      final long id = response.getFieldAsInt64(NXCPCodes.VID_PACKAGE_ID);
      try
      {
         sendFile(msg.getMessageId(), pkgFile, listener, allowCompression, 0);
         waitForRCC(msg.getMessageId());
      }
      catch(IOException e)
      {
         
      }
      return id;
   }

   /**
    * Update metadata for existing package.
    *
    * @param info package information (file name field will be ignored)
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updatePackageMetadata(PackageInfo info) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_PACKAGE_METADATA);
      info.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of installed packages
    *
    * @return List of PackageInfo objects
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
         if (response.getFieldAsInt64(NXCPCodes.VID_PACKAGE_ID) == 0)
            break;
         list.add(new PackageInfo(response));
      }
      return list;
   }

   /**
    * Deploy agent packages onto given nodes
    *
    * @param packageId package ID
    * @param nodeList list of nodes
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deployPackage(long packageId, Collection<Long> nodeList) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DEPLOY_PACKAGE);
      msg.setFieldUInt32(NXCPCodes.VID_PACKAGE_ID, packageId);
      msg.setFieldInt32(NXCPCodes.VID_NUM_OBJECTS, nodeList.size());
      msg.setField(NXCPCodes.VID_OBJECT_LIST, nodeList);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of package deployment jobs
    *
    * @return list of package deployment jobs
    * @throws IOException if socket or file I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<PackageDeploymentJob> getPackageDeploymentJobs() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PACKAGE_DEPLOYMENT_JOBS);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<PackageDeploymentJob> jobs = new ArrayList<>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++, fieldId += 50)
         jobs.add(new PackageDeploymentJob(response, fieldId));
      return jobs;
   }

   public void cancelPackageDeploymentJob(long jobId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CANCEL_PACKAGE_DEPLOYMENT_JOB);
      msg.setFieldUInt32(NXCPCodes.VID_JOB_ID, jobId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Send Notification via server. User should have appropriate rights to execute this command.
    *
    * @param channelName channel name
    * @param phoneNumber target phone number
    * @param subject     message subject
    * @param message     message text
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void sendNotification(String channelName, String phoneNumber, String subject, String message) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SEND_NOTIFICATION);
      msg.setField(NXCPCodes.VID_CHANNEL_NAME, channelName);
      msg.setField(NXCPCodes.VID_RCPT_ADDR, phoneNumber);
      msg.setField(NXCPCodes.VID_EMAIL_SUBJECT, subject);
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
         msg.setFieldInt32(varId++, (int)d.nodeId);
         if (d.nodeId == 0)
            msg.setField(varId++, d.nodeName);
         msg.setFieldInt32(varId++, (int)d.dciId);
         if (d.dciId == 0)
            msg.setField(varId++, d.dciName);
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
      pushDciData(new DciPushData[] { new DciPushData(nodeId, dciId, value) });
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
      pushDciData(new DciPushData[] { new DciPushData(nodeName, dciName, value) });
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
    * @return The current date format
    */
   public String getDateFormat()
   {
      return dateFormat;
   }

   /**
    * Get default time format provided by server
    *
    * @return The current time format
    */
   public String getTimeFormat()
   {
      return timeFormat;
   }

   /**
    * Get time format for short form (usually without seconds).
    *
    * @return The current short time format
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
      target.objectListGUID = objectListGUID;
      target.zoneList = zoneList;
      for(AbstractObject o : objectList.values())
      {
         o.setSession(target);
      }
      objectList = null;
      objectListGUID = null;
      zoneList = null;
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
    * @return List of MappingTableDescriptor objects
    * @throws IOException  if socket I/O error occurs
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
    * Get list of specific mapping table
    *
    * @param id The ID of mapping table
    * @return The MappingTable
    * @throws IOException  if socket I/O error occurs
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
    * @param name        name of new table
    * @param description description for new table
    * @param flags       flags for new table
    * @return ID of new table object
    * @throws IOException  if socket I/O error occurs
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
    * @throws IOException  if socket I/O error occurs
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
    * @throws IOException  if socket I/O error occurs
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
    * Get the default DCI retention time
    *
    * @return the default DCI retention time in days
    */
   public final int getDefaultDciRetentionTime()
   {
      return defaultDciRetentionTime;
   }

   /**
    * Get the default DCI polling interval
    *
    * @return the default DCI polling interval in seconds
    */
   public final int getDefaultDciPollingInterval()
   {
      return defaultDciPollingInterval;
   }

   /**
    * Get the minimal view refresh interval
    *
    * @return the minViewRefreshInterval
    */
   public int getMinViewRefreshInterval()
   {
      return minViewRefreshInterval;
   }

   /**
    * Get the state of alarm status flow
    *
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
    * @return List of DciSummaryTableDescriptor objects
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
    * @return The DciSummaryTable object
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
    * @param id The ID of the summary table
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
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)baseObjectId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      return new Table(response);
   }

   /**
    * Query ad-hoc DCI summary table.
    *
    * @param baseObjectId  base container object ID
    * @param columns       columns for resulting table
    * @param function      data aggregation function
    * @param periodStart   start of query period
    * @param periodEnd     end of query period
    * @param multiInstance The multiInstance flag
    * @return table with last values data for all nodes under given base container
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table queryAdHocDciSummaryTable(long baseObjectId, List<DciSummaryTableColumn> columns, AggregationFunction function,
         Date periodStart, Date periodEnd, boolean multiInstance) throws IOException, NXCException
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
    * List reports
    *
    * @return List of report UUIDs
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
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
    * Get the report definition
    *
    * @param reportId The UUID of the report
    * @return The ReportDefinition object
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
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
    * Execute a report
    *
    * @param jobConfiguration The parameters to set
    * @return the UUID of the report
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public UUID executeReport(ReportingJobConfiguration jobConfiguration) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_EXECUTE_REPORT);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, jobConfiguration.reportId);
      try
      {
         msg.setField(NXCPCodes.VID_EXECUTION_PARAMETERS, jobConfiguration.createXml());
      }
      catch(Exception e)
      {
         throw new NXCException(RCC.INTERNAL_ERROR, e);
      }
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsUUID(NXCPCodes.VID_TASK_ID);
   }

   /**
    * List report results
    *
    * @param reportId The report UUID
    * @return List of ReportResult objects
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<ReportResult> getReportResults(UUID reportId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_LIST_RESULTS);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      List<ReportResult> results = new ArrayList<ReportResult>(count);
      long fieldId = NXCPCodes.VID_ROW_DATA_BASE;
      for(int i = 0; i < count; i++, fieldId += 10)
      {
         results.add(new ReportResult(response, fieldId));
      }
      return results;
   }

   /**
    * Delete report result
    *
    * @param reportId The report UUID
    * @param jobId    The job UUID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteReportResult(UUID reportId, UUID jobId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_DELETE_RESULT);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      msg.setField(NXCPCodes.VID_TASK_ID, jobId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Render report
    *
    * @param reportId The report UUID
    * @param jobId    The job UUID
    * @param format   The format of the render
    * @return The render of the report
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public File renderReport(UUID reportId, UUID jobId, ReportRenderFormat format) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RS_RENDER_RESULT);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      msg.setField(NXCPCodes.VID_TASK_ID, jobId);
      msg.setFieldInt32(NXCPCodes.VID_RENDER_FORMAT, format.getCode());
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
      final ReceivedFile file = waitForFile(msg.getMessageId(), 60000);
      if (file.isFailed())
      {
         throw new NXCException(RCC.IO_ERROR);
      }
      return file.getFile();
   }

   /**
    * List scheduled jobs
    *
    * @param reportId The report UUID
    * @return List of ReportingJob objects
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<ReportingJob> getScheduledReportingJobs(UUID reportId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SCHEDULED_REPORTING_TASKS);
      msg.setField(NXCPCodes.VID_REPORT_DEFINITION, reportId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());

      long fieldId = NXCPCodes.VID_SCHEDULE_LIST_BASE;
      int count = response.getFieldAsInt32(NXCPCodes.VID_SCHEDULE_COUNT);
      List<ReportingJob> result = new ArrayList<ReportingJob>(count);
      for(int i = 0; i < count; i++)
      {
         result.add(new ReportingJob(response, fieldId));
         fieldId += 100;
      }
      return result;
   }

   /**
    * Set the client address
    *
    * @param clientAddress the clientAddress to set
    */
   public void setClientAddress(String clientAddress)
   {
      this.clientAddress = clientAddress;
   }

   /**
    * Sign a challenge
    *
    * @param signature user's signature
    * @param challenge challenge string received from server
    * @return signed server challenge
    * @throws NXCException if NetXMS server returns an error or operation was timed out
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
    * Get the client`s language
    *
    * @return the clientLanguage
    */
   public String getClientLanguage()
   {
      return clientLanguage;
   }

   /**
    * Set client`s language
    *
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
    * @param subnetId The subnet ID
    * @return Address map
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
   public List<AgentConfigurationHandle> getAgentConfigurations() throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AGENT_CONFIGURATION_LIST);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_RECORDS);
      List<AgentConfigurationHandle> elements = new ArrayList<AgentConfigurationHandle>(count);
      long fieldId = NXCPCodes.VID_AGENT_CFG_LIST_BASE;
      for(int i = 0; i < count; i++, fieldId += 10)
      {
         elements.add(new AgentConfigurationHandle(response, fieldId));
      }
      Collections.sort(elements);
      return elements;
   }

   /**
    * Get server side agent configuration
    *
    * @param id configuration object ID
    * @return agent configuration object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public AgentConfiguration getAgentConfiguration(long id) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AGENT_CONFIGURATION);
      msg.setFieldInt32(NXCPCodes.VID_CONFIG_ID, (int)id);
      sendMessage(msg);

      NXCPMessage response = waitForRCC(msg.getMessageId());
      AgentConfiguration content = new AgentConfiguration(id, response);
      return content;
   }

   /**
    * Update server side agent configuration
    *
    * @param configuration agent configuration object
    * @return configuration ID (possibly newly assigned)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long saveAgentConfig(AgentConfiguration configuration) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_AGENT_CONFIGURATION);
      configuration.fillMessage(msg);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_CONFIG_ID);
   }

   /**
    * Delete server side agent configuration. This call will not change sequence numbers of other configurations.
    *
    * @param id agent configuration ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAgentConfig(long id) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_AGENT_CONFIGURATION);
      msg.setFieldInt32(NXCPCodes.VID_CONFIG_ID, (int)id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Swap sequence numbers of two server side agent configurations.
    *
    * @param id1 first agent configuration ID
    * @param id2 second agent configuration ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void swapAgentConfigs(long id1, long id2) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SWAP_AGENT_CONFIGURATIONS);
      msg.setFieldInt32(NXCPCodes.VID_CONFIG_ID, (int)id1);
      msg.setFieldInt32(NXCPCodes.VID_CONFIG_ID_2, (int)id2);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get location history for given object.
    *
    * @param objectId The object ID
    * @param from     The date from
    * @param to       The date to
    * @return List of location history
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
      List<GeoLocation> elements = new ArrayList<GeoLocation>();
      long fieldId = NXCPCodes.VID_LOC_LIST_BASE;
      for(int i = 0; i < size; i++, fieldId += 10)
      {
         elements.add(new GeoLocation(response, fieldId));
      }
      Collections.sort(elements, new Comparator<GeoLocation>()
      {
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
    * @param nodeId      node object ID
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

      NXCPMessage response = waitForRCC(msg.getMessageId(), msgWaitQueue.getDefaultTimeout() + 60000);
      return response.getFieldAsBinary(NXCPCodes.VID_FILE_DATA);
   }

   /**
    * Get list of available scheduled task handlers.
    *
    * @return list of available scheduled task handlers.
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<String> getScheduledTaskHandlers() throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LIST_SCHEDULE_CALLBACKS);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getStringListFromFields(NXCPCodes.VID_CALLBACK_BASE, NXCPCodes.VID_CALLBACK_COUNT);
   }

   /**
    * Get list of scheduled tasks.
    *
    * @return list of scheduled tasks
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<ScheduledTask> getScheduledTasks() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LIST_SCHEDULES);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());

      int size = response.getFieldAsInt32(NXCPCodes.VID_SCHEDULE_COUNT);
      long i, base;
      ArrayList<ScheduledTask> list = new ArrayList<ScheduledTask>(size);
      for(i = 0, base = NXCPCodes.VID_SCHEDULE_LIST_BASE; i < size; i++, base += 100)
      {
         list.add(new ScheduledTask(response, base));
      }

      return list;
   }

   /**
    * Add new scheduled task
    *
    * @param task new scheduled task
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void addScheduledTask(ScheduledTask task) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_ADD_SCHEDULE);
      task.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Update existing scheduled task.
    *
    * @param task updated scheduled task
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateScheduledTask(ScheduledTask task) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_SCHEDULE);
      task.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete scheduled task.
    *
    * @param taskId scheduled task ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteScheduledTask(long taskId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_REMOVE_SCHEDULE);
      msg.setFieldInt32(NXCPCodes.VID_SCHEDULED_TASK_ID, (int)taskId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Set maintenance mode for object
    *
    * @param objectId      object ID
    * @param inMaintenance new maintenance mode setting (true = on, false = off)
    * @param comments      comments for entering maintenance
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setObjectMaintenanceMode(long objectId, boolean inMaintenance, String comments) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(inMaintenance ? NXCPCodes.CMD_ENTER_MAINT_MODE : NXCPCodes.CMD_LEAVE_MAINT_MODE);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      if (comments != null)
         msg.setField(NXCPCodes.VID_COMMENTS, comments);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Set maintenance mode for object
    *
    * @param objectId      object ID
    * @param inMaintenance new maintenance mode setting (true = on, false = off)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setObjectMaintenanceMode(long objectId, boolean inMaintenance) throws NXCException, IOException
   {
      setObjectMaintenanceMode(objectId, inMaintenance, null);
   }

   /**
    * Get list of configured repositories
    *
    * @return list of configured repositories
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<Repository> getRepositories() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_REPOSITORIES);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<Repository> list = new ArrayList<Repository>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         list.add(new Repository(response, fieldId));
         fieldId += 10;
      }
      return list;
   }

   /**
    * Add repository. Will update given repository object with assigned ID.
    *
    * @param r The repository
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void addRepository(Repository r) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_ADD_REPOSITORY);
      r.fillMessage(msg);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      r.setId(response.getFieldAsInt32(NXCPCodes.VID_OBJECT_ID));
   }

   /**
    * Modify repository.
    *
    * @param r The repository
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyRepository(Repository r) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_REPOSITORY);
      r.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete repository.
    *
    * @param id The id
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteRepository(int id) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_REPOSITORY);
      msg.setFieldInt32(NXCPCodes.VID_REPOSITORY_ID, id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of registered prediction engines
    *
    * @return List of PredictionEngine objects
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public synchronized List<PredictionEngine> getPredictionEngines() throws IOException, NXCException
   {
      if (predictionEngines != null)
         return predictionEngines;

      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PREDICTION_ENGINES);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<PredictionEngine> engines = new ArrayList<PredictionEngine>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         engines.add(new PredictionEngine(response, fieldId));
         fieldId += 10;
      }
      predictionEngines = engines;  // cache received list
      return engines;
   }

   /**
    * Get predicted DCI data from server.
    *
    * @param nodeId Node ID
    * @param dciId  DCI ID
    * @param from   Start of time range
    * @param to     End of time range
    * @return DCI data set
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DataSeries getPredictedData(long nodeId, long dciId, Date from, Date to) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PREDICTED_DATA);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      msg.setFieldInt32(NXCPCodes.VID_DCI_ID, (int)dciId);

      DataSeries data = new DataSeries(nodeId, dciId);

      int rowsReceived;
      int timeFrom = (int)(from.getTime() / 1000);
      int timeTo = (int)(to.getTime() / 1000);

      do
      {
         msg.setMessageId(requestId.getAndIncrement());
         msg.setFieldInt32(NXCPCodes.VID_TIME_FROM, timeFrom);
         msg.setFieldInt32(NXCPCodes.VID_TIME_TO, timeTo);
         sendMessage(msg);

         waitForRCC(msg.getMessageId());

         NXCPMessage response = waitForMessage(NXCPCodes.CMD_DCI_DATA, msg.getMessageId());
         if (!response.isBinaryMessage())
            throw new NXCException(RCC.INTERNAL_ERROR);

         rowsReceived = parseDataRows(response.getBinaryData(), data);
         if (rowsReceived == MAX_DCI_DATA_ROWS)
         {
            // Rows goes in newest to oldest order, so if we need to
            // retrieve additional data, we should update timeTo limit
            DciDataRow row = data.getLastValue();
            if (row != null)
            {
               // There should be only one value per second, so we set
               // last row's timestamp - 1 second as new boundary
               timeTo = (int)(row.getTimestamp().getTime() / 1000) - 1;
            }
         }
      } while((rowsReceived == MAX_DCI_DATA_ROWS) && (timeTo > timeFrom));
      return data;
   }

   /**
    * Get list of agent tunnels
    *
    * @return list of agent tunnels
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AgentTunnel> getAgentTunnels() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AGENT_TUNNELS);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<AgentTunnel> tunnels = new ArrayList<AgentTunnel>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         tunnels.add(new AgentTunnel(response, fieldId));
         fieldId += 64;
      }
      return tunnels;
   }

   /**
    * Wait for bound tunnel for given node to appear. This method will not throw usual exceptions on communication or server errors
    * but will return false instead.
    *
    * @param nodeId node ID
    * @param timeout waiting timeout in milliseconds
    * @return true if bound tunnel for given node exist
    */
   public boolean waitForAgentTunnel(long nodeId, long timeout)
   {
      try
      {
         while(timeout > 0)
         {
            long startTime = System.currentTimeMillis();
            for(AgentTunnel t : getAgentTunnels())
            {
               if (t.isBound() && (t.getNodeId() == nodeId))
                  return true;
            }
            Thread.sleep(1000);
            timeout -= System.currentTimeMillis() - startTime;
         }
         return false;
      }
      catch(Exception e)
      {
         return false;
      }
   }

   /**
    * Bind agent tunnel to node
    *
    * @param tunnelId tunnel ID
    * @param nodeId   node ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void bindAgentTunnel(int tunnelId, long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_BIND_AGENT_TUNNEL);
      msg.setFieldInt32(NXCPCodes.VID_TUNNEL_ID, tunnelId);
      msg.setFieldInt32(NXCPCodes.VID_NODE_ID, (int)nodeId);
      sendMessage(msg);
      // Bind operation can take significant time on slow links and/or slow devices
      waitForRCC(msg.getMessageId(), Math.max(60000, msgWaitQueue.getDefaultTimeout()));
   }

   /**
    * Unbind agent tunnel to node
    *
    * @param nodeId node ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unbindAgentTunnel(long nodeId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UNBIND_AGENT_TUNNEL);
      msg.setFieldInt32(NXCPCodes.VID_NODE_ID, (int)nodeId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Substitute macros in multiple strings within one context.
    *
    * @param context expansion context
    * @param textsToExpand texts to be expanded
    * @param inputValues input values provided by used used for %() expansion
    * @return same count and order of strings already expanded
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<String> substituteMacros(ObjectContextBase context, List<String> textsToExpand, Map<String, String> inputValues)
         throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_EXPAND_MACROS);
      long varId;
      if (inputValues != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_INPUT_FIELD_COUNT, inputValues.size());
         varId = NXCPCodes.VID_INPUT_FIELD_BASE;
         for(Entry<String, String> e : inputValues.entrySet())
         {
            msg.setField(varId++, e.getKey());
            msg.setField(varId++, e.getValue());
         }
      }
      msg.setFieldInt32(NXCPCodes.VID_STRING_COUNT, textsToExpand.size());
      varId = NXCPCodes.VID_EXPAND_STRING_BASE;
      for(String s : textsToExpand)
      {
         context.fillMessage(msg, varId, s);
         varId += 5;
      }

      List<String> result = new ArrayList<String>();
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      varId = NXCPCodes.VID_EXPAND_STRING_BASE;
      for(int i = 0; i < textsToExpand.size(); i++)
      {
         result.add(response.getFieldAsString(varId++));
      }
      return result;
   }

   /**
    * Substitute macros in one string within multiple contexts.
    *
    * @param context expansion contexts
    * @param textToExpand text to be expanded
    * @param inputValues input values provided by used used for %() expansion
    * @return same count and order of strings already expanded
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<String> substituteMacros(ObjectContextBase context[], String textToExpand, Map<String, String> inputValues)
         throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_EXPAND_MACROS);
      long varId;
      if (inputValues != null)
      {
         msg.setFieldInt32(NXCPCodes.VID_INPUT_FIELD_COUNT, inputValues.size());
         varId = NXCPCodes.VID_INPUT_FIELD_BASE;
         for(Entry<String, String> e : inputValues.entrySet())
         {
            msg.setField(varId++, e.getKey());
            msg.setField(varId++, e.getValue());
         }
      }
      msg.setFieldInt32(NXCPCodes.VID_STRING_COUNT, context.length);
      varId = NXCPCodes.VID_EXPAND_STRING_BASE;
      for(ObjectContextBase c : context)
      {
         c.fillMessage(msg, varId, textToExpand);
         varId += 5;
      }

      List<String> result = new ArrayList<String>();
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      varId = NXCPCodes.VID_EXPAND_STRING_BASE;
      for(int i = 0; i < context.length; i++)
      {
         result.add(response.getFieldAsString(varId++));
      }
      return result;
   }

   /**
    * Setup new TCP proxy channel. Proxy object should be disposed with TcpProxy.close() call when no longer needed. If zone object
    * ID passed as proxyId then server will select most suitable proxy node from zone's proxy node list.
    *
    * @param proxyId proxy node ID (node that will initiate TCP connection to target) or zone object ID
    * @param address target IP address
    * @param port target TCP port
    * @return TCP proxy object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public TcpProxy setupTcpProxy(long proxyId, InetAddress address, int port) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SETUP_TCP_PROXY);
      msg.setFieldInt32(NXCPCodes.VID_TCP_PROXY, (int)proxyId);
      msg.setField(NXCPCodes.VID_IP_ADDRESS, address);
      msg.setFieldInt16(NXCPCodes.VID_PORT, port);
      return setupTcpProxyInternal(msg);
   }

   /**
    * Setup new TCP proxy channel to given node. Server will select appropriate proxy node. Proxy object should be disposed with
    * TcpProxy.close() call when no longer needed.
    *
    * @param nodeId target node ID
    * @param port target TCP port
    * @return TCP proxy object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public TcpProxy setupTcpProxy(long nodeId, int port) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SETUP_TCP_PROXY);
      msg.setFieldInt32(NXCPCodes.VID_TCP_PROXY, 0);
      msg.setFieldInt32(NXCPCodes.VID_NODE_ID, (int)nodeId);
      msg.setFieldInt16(NXCPCodes.VID_PORT, port);
      return setupTcpProxyInternal(msg);
   }

   /**
    * Internal implementation of different setupTcpProxy() variants
    *
    * @param request prepared request message
    * @return TCP proxy object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private TcpProxy setupTcpProxyInternal(NXCPMessage request) throws IOException, NXCException
   {
      request.setField(NXCPCodes.VID_ENABLE_TWO_PHASE_SETUP, true);

      TcpProxy proxy = null;
      if (clientAssignedTcpProxyId)
      {
         int channelId = tcpProxyChannelId.getAndIncrement();
         request.setFieldInt32(NXCPCodes.VID_CHANNEL_ID, channelId);
         proxy = new TcpProxy(this, channelId);
         synchronized(tcpProxies)
         {
            tcpProxies.put(proxy.getChannelId(), proxy);
            logger.debug("Registered new TCP proxy channel " + proxy.getChannelId() + " (client assigned ID)");
         }
      }

      try
      {
         sendMessage(request);
         final NXCPMessage response = waitForRCC(request.getMessageId());

         if (!clientAssignedTcpProxyId)
         {
            proxy = new TcpProxy(this, response.getFieldAsInt32(NXCPCodes.VID_CHANNEL_ID));
            synchronized(tcpProxies)
            {
               tcpProxies.put(proxy.getChannelId(), proxy);
               logger.debug("Registered new TCP proxy channel " + proxy.getChannelId() + " (server assigned ID)");
            }
         }

         if (response.getFieldAsBoolean(NXCPCodes.VID_ENABLE_TWO_PHASE_SETUP))
         {
            // Server supports two-phase setup, wait for final confirmation
            logger.debug("Two-phase setup for TCP proxy channel " + proxy.getChannelId() + " - (waiting for second confirmation)");
            waitForRCC(request.getMessageId());
            logger.debug("Two-phase setup for TCP proxy channel " + proxy.getChannelId() + " completed");
         }
      }
      catch(Exception e)
      {
         if (proxy != null)
         {
            synchronized(tcpProxies)
            {
               tcpProxies.remove(proxy.getChannelId());
            }
         }
         throw e;
      }

      return proxy;
   }

   /**
    * Close TCP proxy channel.
    *
    * @param channelId proxy channel ID
    */
   protected void closeTcpProxy(int channelId)
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CLOSE_TCP_PROXY);
      msg.setFieldInt32(NXCPCodes.VID_CHANNEL_ID, channelId);
      try
      {
         sendMessage(msg);
         waitForRCC(msg.getMessageId());
      }
      catch(Exception e)
      {
         logger.debug("Error closing TCP proxy channel " + channelId, e);
      }
      synchronized(tcpProxies)
      {
         tcpProxies.remove(channelId);
      }
   }
   
   /**
    * Get agent policy by template id and GUID
    * 
    * @param templateId template id policy is in
    * @param policyGUID policy GUID
    * @return agent policy
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public AgentPolicy getAgentPolicy(long templateId, UUID policyGUID) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AGENT_POLICY);
      msg.setFieldInt32(NXCPCodes.VID_TEMPLATE_ID, (int)templateId);
      msg.setField(NXCPCodes.VID_GUID, policyGUID);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      AgentPolicy policy = new AgentPolicy(response, NXCPCodes.VID_AGENT_POLICY_BASE);
      return policy;
   }

   /**
    * Returns agent policy list
    * 
    * @param templateId id of the template where policy are defined
    * @return hash map of policy UUID to policy
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public HashMap<UUID, AgentPolicy> getAgentPolicyList(long templateId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AGENT_POLICY_LIST);
      msg.setFieldInt32(NXCPCodes.VID_TEMPLATE_ID, (int)templateId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      HashMap<UUID, AgentPolicy> map = new HashMap<UUID, AgentPolicy>();
      long base = NXCPCodes.VID_AGENT_POLICY_BASE;
      int size = response.getFieldAsInt32(NXCPCodes.VID_POLICY_COUNT);
      for(int i = 0; i < size; i++, base += 100)
      {
         AgentPolicy policy = new AgentPolicy(response, base);
         map.put(policy.getGuid(), policy);
      }
      return map;
   }

   /**
    * Saves new or updated policy
    * 
    * @param templateId id of template where policy is defined
    * @param policy policy data to be updated or created. For new policy GUID should be null
    * @param duplicate if set to true server will make copy of existing policy instead of updating existing one 
    * @return UUID of saved policy
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public UUID savePolicy(long templateId, AgentPolicy policy, boolean duplicate) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_AGENT_POLICY);
      msg.setFieldInt32(NXCPCodes.VID_TEMPLATE_ID, (int)templateId);
      msg.setField(NXCPCodes.VID_DUPLICATE, duplicate);
      policy.fillMessage(msg);
      sendMessage(msg);
      return waitForRCC(msg.getMessageId()).getFieldAsUUID(NXCPCodes.VID_GUID);
   }

   /**
    * Delete policy 
    * 
    * @param templateId id of template where policy is defined
    * @param guid guid of the policy that should be deleted
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deletePolicy(long templateId, UUID guid) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_AGENT_POLICY);
      msg.setFieldInt32(NXCPCodes.VID_TEMPLATE_ID, (int)templateId);
      msg.setField(NXCPCodes.VID_GUID, guid);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Command sent on policyEditor close to send updates to all applied nodes
    * 
    * @param templateId id of the closed template
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void onPolicyEditorClose(long templateId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_POLICY_EDITOR_CLOSED);
      msg.setFieldInt32(NXCPCodes.VID_TEMPLATE_ID, (int)templateId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Force policy installation on all nodes where template is applied 
    * 
    * @param templateId template id
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void forcePolicyInstallation(long templateId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_POLICY_FORCE_APPLY);
      msg.setFieldInt32(NXCPCodes.VID_TEMPLATE_ID, (int)templateId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get user support application notifications list
    * 
    * @return list of user support application notifications
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<UserAgentNotification> getUserAgentNotifications() throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_UA_NOTIFICATIONS);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_USER_AGENT_MESSAGE_COUNT);
      List<UserAgentNotification> list = new ArrayList<UserAgentNotification>(count);
      long fieldId = NXCPCodes.VID_UA_NOTIFICATION_BASE;
      for (int i = 0 ; i < count; i++, fieldId += 10)
         list.add(new UserAgentNotification(response, fieldId, this));
      return list;
   }

   /**
    * Recall user support application notification
    * 
    * @param id recall id
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void recallUserAgentNotification(long id) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RECALL_UA_NOTIFICATION);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Create new user support application notifications
    * 
    * @param message notification message
    * @param objects objects to show notifications on
    * @param startTime notification's activation time
    * @param endTime notificaiton's display end time
    * @param onStartup true if notification should be shown only after user agent startup (usually immediately after user login)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void createUserAgentNotification(String message, long[] objects, Date startTime, Date endTime, boolean onStartup) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_ADD_UA_NOTIFICATION);
      msg.setField(NXCPCodes.VID_UA_NOTIFICATION_BASE, message);
      msg.setField(NXCPCodes.VID_UA_NOTIFICATION_BASE + 1, objects);
      msg.setField(NXCPCodes.VID_UA_NOTIFICATION_BASE + 2, startTime);
      msg.setField(NXCPCodes.VID_UA_NOTIFICATION_BASE + 3, endTime);
      msg.setField(NXCPCodes.VID_UA_NOTIFICATION_BASE + 4, onStartup);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Create new user support application notifications
    * 
    * @param message notification message
    * @param objects objects to show notifications on
    * @param startTime notification's activation time
    * @param endTime notificaiton's display end time
    * @param onStartup true if notification should be shown only after user agent startup (usually immediately after user login)
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void createUserAgentNotification(String message, Collection<Long> objects, Date startTime, Date endTime, boolean onStartup) throws NXCException, IOException
   {
      long[] idList = new long[objects.size()];
      int i = 0;
      for(Long id : objects)
         idList[i++] = id;
      createUserAgentNotification(message, idList, startTime, endTime, onStartup);
   }

   /**
    * Get server notification channels
    * 
    * @return list of server notifications channels
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<NotificationChannel> getNotificationChannels() throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_NOTIFICATION_CHANNELS);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_CHANNEL_COUNT);
      List<NotificationChannel> channels = new ArrayList<NotificationChannel>(count);
      long base = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++, base += 20)
         channels.add(new NotificationChannel(response, base));            
      return channels;
   }

   /**
    * Create notification channel 
    * 
    * @param nc new notification channel
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void createNotificationChannel(NotificationChannel nc) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_ADD_NOTIFICATION_CHANNEL);
      nc.fillMessage(msg);
      sendMessage(msg);  
      waitForRCC(msg.getMessageId());    
   }
   
   /**
    * Update notification channel 
    * 
    * @param nc update notification channel
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateNotificationChannel(NotificationChannel nc) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_NOTIFICATION_CHANNEL);
      nc.fillMessage(msg);
      sendMessage(msg);  
      waitForRCC(msg.getMessageId());          
   }
   
   /**
    * Delete notification channel
    * 
    * @param name name of notification channel to be deleted
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteNotificationChannel(String name) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_NOTIFICATION_CHANNEL);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);  
      waitForRCC(msg.getMessageId());    
   }

   /**
    * Rename notification channel
    * 
    * @param oldName old notification channel name
    * @param newName new notification channel name
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void renameNotificationChannel(String oldName, String newName) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_RENAME_NOTIFICATION_CHANNEL);
      msg.setField(NXCPCodes.VID_NAME, oldName);
      msg.setField(NXCPCodes.VID_NEW_NAME, newName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of available notification channel drivers.
    * 
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    * @return list of available notification channel drivers
    */
   public List<String> getNotificationDrivers() throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_NOTIFICATION_DRIVERS);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      List<String> drivers = response.getStringListFromFields(NXCPCodes.VID_ELEMENT_LIST_BASE, NXCPCodes.VID_DRIVER_COUNT);
      return drivers;
   }

   /**
    * Start active discovery for provided list manually
    * 
    * @param ranges IP address ranges to scan
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void startManualActiveDiscovery(List<InetAddressListElement> ranges) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_START_ACTIVE_DISCOVERY);
      msg.setFieldInt32(NXCPCodes.VID_NUM_RECORDS, ranges.size());
      long fieldId = NXCPCodes.VID_ADDR_LIST_BASE;
      for(InetAddressListElement e : ranges)
      {
         e.fillMessage(msg, fieldId);
         fieldId += 10;
      }
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of physical links filtered by provided options
    * 
    * @param objectId node id or rack id to filter out physical links
    * @param patchPanelId patch panel id to filter out it's physical links (first parameter should be rack id)
    * @return list of physical links
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<PhysicalLink> getPhysicalLinks(long objectId, long patchPanelId) throws NXCException, IOException
   {
      List<PhysicalLink> links = new ArrayList<PhysicalLink>();
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_PHYSICAL_LINKS);
      if (objectId > 0)
         msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      if (patchPanelId > 0)
         msg.setFieldInt32(NXCPCodes.VID_PATCH_PANEL_ID, (int)patchPanelId);      
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_LINK_COUNT);
      long base = NXCPCodes.VID_LINK_LIST_BASE;
      for(int i = 0; i < count; i ++)
      {
         links.add(new PhysicalLink(response, base));
         base += 20;
      }      
      return links;
   }
   
   /**
    * Create new or update existing physical link
    * 
    * @param link link to be created with ID 0 or link to be updated with correct ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updatePhysicalLink(PhysicalLink link) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_PHYSICAL_LINK);
      link.fillMessage(msg, NXCPCodes.VID_LINK_LIST_BASE);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Delete physical link.
    * 
    * @param linkId physical link ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deletePhysicalLink(long linkId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_PHYSICAL_LINK);
      msg.setFieldInt32(NXCPCodes.VID_PHYSICAL_LINK_ID, (int)linkId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());      
   }

   /**
    * Get configured web service definitions.
    * 
    * @return list of configured web service definitions
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<WebServiceDefinition> getWebServiceDefinitions() throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_WEB_SERVICES);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<WebServiceDefinition> definitions = new ArrayList<WebServiceDefinition>(count);
      for(int i = 0; i < count; i++)
      {
         response = waitForMessage(NXCPCodes.CMD_WEB_SERVICE_DEFINITION, msg.getMessageId());
         definitions.add(new WebServiceDefinition(response));
      }
      return definitions;
   }

   /**
    * Modify (or create new) web service definition. New definition will be created if ID of provided definition object set to 0.
    * 
    * @param definition web service definition to create or modify
    * @return assigned ID of web service definition (same as provided if modifying existing definition)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int modifyWebServiceDefinition(WebServiceDefinition definition) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_WEB_SERVICE);
      definition.fillMessage(msg);
      sendMessage(msg);
      return waitForRCC(msg.getMessageId()).getFieldAsInt32(NXCPCodes.VID_WEBSVC_ID);
   }

   /**
    * Delete web service definition.
    * 
    * @param id web service definition ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteWebServiceDefinition(int id) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_WEB_SERVICE);
      msg.setFieldInt32(NXCPCodes.VID_WEBSVC_ID, id);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get agent shared secret list (global and all zones).
    * 
    * @return map with zone UIN and shared secret list (-1 for global secrets).
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<Integer, List<String>> getAgentSharedSecrets() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SHARED_SECRET_LIST);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      Map<Integer, List<String>> map = new HashMap<Integer, List<String>>(count);
      List<String> stringList = new ArrayList<String>();
      long fieldId = NXCPCodes.VID_SHARED_SECRET_LIST_BASE;
      int currentZoneUIN = response.getFieldAsInt32(fieldId + 1);
      for(int i = 0; i < count; i++, fieldId += 10)
      {
         int zoneUIN = response.getFieldAsInt32(fieldId + 1);
         if (currentZoneUIN != zoneUIN)
         {
            map.put(currentZoneUIN, stringList);
            stringList = new ArrayList<String>();
            currentZoneUIN = zoneUIN;
         }
         stringList.add(response.getFieldAsString(fieldId));
      }
      if (count > 0)
         map.put(currentZoneUIN, stringList);
      return map;
   }

   /**
    * Get agent shared secret list for specific zone.
    *
    * @param zoneUIN zone UIN (-1 for global shared secret list)
    * @return map with zone id and shared secret list
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<String> getAgentSharedSecrets(int zoneUIN) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SHARED_SECRET_LIST);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      long fieldId = NXCPCodes.VID_SHARED_SECRET_LIST_BASE;
      List<String> stringList = new ArrayList<String>();
      for(int i = 0; i < count; i++)
      {
         stringList.add(response.getFieldAsString(fieldId++));
      }
      return stringList;
   }

   /**
    * Update agent shared secret list on server.
    *
    * @param zoneUIN zone UIN (-1 for global shared secret list)
    * @param sharedSecrets list of shared secrets
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateAgentSharedSecrets(int zoneUIN, List<String> sharedSecrets) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_SHARED_SECRET_LIST);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
      long baseId = NXCPCodes.VID_SHARED_SECRET_LIST_BASE;
      for(String s : sharedSecrets)
      {
         msg.setField(baseId++, s);
      }
      msg.setFieldInt32(NXCPCodes.VID_NUM_ELEMENTS, sharedSecrets.size());
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of well-known ports (global and all zones).
    *
    * @param tag port list tag
    * @return list of well-known ports
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<Integer, List<Integer>> getWellKnownPorts(String tag) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SNMP_PORT_LIST);
      msg.setField(NXCPCodes.VID_TAG, tag);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_ZONE_PORT_COUNT);
      Map<Integer, List<Integer>> map = new HashMap<Integer, List<Integer>>(count);
      List<Integer> portList = new ArrayList<Integer>();
      long fieldId = NXCPCodes.VID_ZONE_PORT_LIST_BASE;
      int currentZoneUIN = response.getFieldAsInt32(fieldId + 1);
      for(int i = 0; i < count; i++, fieldId += 10)
      {
         int zoneUIN = response.getFieldAsInt32(fieldId + 1);
         if (currentZoneUIN != zoneUIN)
         {
            map.put(currentZoneUIN, portList);
            portList = new ArrayList<Integer>();
            currentZoneUIN = zoneUIN;
         }
         portList.add(response.getFieldAsInt32(fieldId));
      }
      if (count > 0)
         map.put(currentZoneUIN, portList);
      return map;
   }

   /**
    * Get list of well-known ports for given zone.
    *
    * @param zoneUIN zone UIN (-1 for global port list)
    * @param tag port list tag
    * @return list of well-known ports
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<Integer> getWellKnownPorts(int zoneUIN, String tag) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SNMP_PORT_LIST);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
      msg.setField(NXCPCodes.VID_TAG, tag);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_ZONE_PORT_COUNT);
      List<Integer> portList = new ArrayList<Integer>(count);
      long fieldId = NXCPCodes.VID_ZONE_PORT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         portList.add(response.getFieldAsInt32(fieldId++));
      }
      return portList;
   }

   /**
    * Update list of well-known ports.
    *
    * @param zoneUIN zone UIN for which port list should be updated (-1 for global port list)
    * @param tag port list tag
    * @param ports new port list
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateWellKnownPorts(int zoneUIN, String tag, List<Integer> ports) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_WELL_KNOWN_PORT_LIST);
      msg.setFieldInt32(NXCPCodes.VID_ZONE_UIN, zoneUIN);
      msg.setField(NXCPCodes.VID_TAG, tag);
      long fieldId = NXCPCodes.VID_ZONE_PORT_LIST_BASE;
      for(Integer port : ports)
      {
         msg.setFieldInt16(fieldId++, port);
      }
      msg.setFieldInt32(NXCPCodes.VID_ZONE_PORT_COUNT, ports.size());
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get configured geographical areas
    *
    * @return list of configured geographical areas
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<GeoArea> getGeoAreas() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_GEO_AREAS);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<GeoArea> areas = new ArrayList<GeoArea>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++, fieldId += 4096)
         areas.add(new GeoArea(response, fieldId));
      return areas;
   }

   /**
    * Modify geographical area.
    *
    * @param area updated geographical area object
    * @return ID of geographical area object
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public int modifyGeoArea(GeoArea area) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_GEO_AREA);
      area.fillMessage(msg);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt32(NXCPCodes.VID_AREA_ID);
   }

   /**
    * Delete geographical area. This method will fail if area is in use (set to at least one object) unless <b>forceDelete</b>
    * parameter set to <b>true</b>.
    *
    * @param areaId area ID
    * @param forceDelete force deletion flag - if set to true area will be deleted even if in use
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteGeoArea(int areaId, boolean forceDelete) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_GEO_AREA);
      msg.setFieldInt32(NXCPCodes.VID_AREA_ID, areaId);
      msg.setField(NXCPCodes.VID_FORCE_DELETE, forceDelete);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Find suitable proxy for given node.
    *
    * @param nodeId node object ID
    * @return proxy node object ID or 0 if proxy cannot be found
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long findProxyForNode(long nodeId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_FIND_PROXY_FOR_NODE);
      msg.setFieldInt32(NXCPCodes.VID_NODE_ID, (int)nodeId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_AGENT_PROXY);
   }

   /**
    * Get list of SSH key
    * 
    * @param withPublicKey true if ssh data should be returned with public key
    * @return list of SSH keys
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<SshKeyPair> getSshKeys(boolean withPublicKey) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_SSH_KEYS_LIST);
      msg.setField(NXCPCodes.VID_INCLUDE_PUBLIC_KEY, withPublicKey);      
      sendMessage(msg);
      
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_SSH_KEY_COUNT);
      List<SshKeyPair> sshKeys = new ArrayList<SshKeyPair>(count);
      long fieldId = NXCPCodes.VID_SSH_KEY_LIST_BASE;
      for (int i = 0; i < count; i++, fieldId += 5)
      {
         sshKeys.add(new SshKeyPair(response, fieldId));
      }
      
      return sshKeys;
   }

   /**
    * Delete SSH key. 
    * 
    * @param id key ID
    * @param force if set to true key will be deleted even if it is in use
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteSshKey(int id, boolean force) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_SSH_KEY);
      msg.setFieldInt32(NXCPCodes.VID_SSH_KEY_ID, id);
      msg.setField(NXCPCodes.VID_FORCE_DELETE, force);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Update or import new SSH keys
    * 
    * @param sshCertificateData ssh key information
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateSshKey(SshKeyPair sshCertificateData) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_SSH_KEYS);
      sshCertificateData.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Generate new SSH keys .
    * 
    * @param name for new keys
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void generateSshKeys(String name) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_GENERATE_SSH_KEYS);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get list of available two-factor authentication drivers.
    * 
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    * @return list of available two-factor authentication drivers
    */
   public List<String> get2FADrivers() throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_2FA_GET_DRIVERS);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      List<String> drivers = response.getStringListFromFields(NXCPCodes.VID_ELEMENT_LIST_BASE, NXCPCodes.VID_DRIVER_COUNT);
      return drivers;
   }

   /**
    * Get list of configured two-factor authentication methods. Depending on user access method configuration may not be returned.
    *
    * @return list of configured two-factor authentication methods
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<TwoFactorAuthenticationMethod> get2FAMethods() throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_2FA_GET_METHODS);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_2FA_METHOD_COUNT);
      List<TwoFactorAuthenticationMethod> methods = new ArrayList<TwoFactorAuthenticationMethod>(count);
      long fieldId = NXCPCodes.VID_2FA_METHOD_LIST_BASE;
      for(int i = 0; i < count; i++, fieldId += 10)
      {
         methods.add(new TwoFactorAuthenticationMethod(response, fieldId));
      }

      return methods;
   }

   /**
    * Modify existing two-factor authentication method or create new one.
    *
    * @param method method definition
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modify2FAMethod(TwoFactorAuthenticationMethod method) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_2FA_MODIFY_METHOD);
      method.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Rename two-factor authentication method
    * 
    * @param oldName old method name
    * @param newName new method name
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void rename2FAMethod(String oldName, String newName) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_2FA_RENAME_METHOD);
      msg.setField(NXCPCodes.VID_NAME, oldName);
      msg.setField(NXCPCodes.VID_NEW_NAME, newName);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete two-factor authentication method.
    *
    * @param name method name
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void delete2FAMethod(String name) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_2FA_DELETE_METHOD);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**    
    * Get list business service checks
    * 
    * @param serviceId business service id 
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    * @return list of business service checks
    */
   public Map<Long, BusinessServiceCheck> getBusinessServiceChecks(long serviceId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_BIZSVC_CHECK_LIST);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)serviceId);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());
      Map<Long, BusinessServiceCheck> checks = new HashMap<Long, BusinessServiceCheck>();
      int count = response.getFieldAsInt32(NXCPCodes.VID_CHECK_COUNT);
      long fieldId = NXCPCodes.VID_CHECK_LIST_BASE;
      for (int i= 0; i < count; i ++)
      {
         BusinessServiceCheck check = new BusinessServiceCheck(response, fieldId);
         checks.put(check.getId(), check);
         fieldId += 100;
      }
      return checks;
   }

   /**  
    * Delete check form businsess service
    * 
    * @param businessServiceid business service id 
    * @param checkId cehck id
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteBusinessServiceCheck(long businessServiceid, long checkId) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_BIZSVC_CHECK);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, businessServiceid);
      msg.setFieldUInt32(NXCPCodes.VID_CHECK_ID, checkId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**  
    * Modify check form businsess service
    * 
    * @param businessServiceid business service id 
    * @param check cehck id
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyBusinessServiceCheck(long businessServiceid, BusinessServiceCheck check) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_BIZSVC_CHECK);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, businessServiceid);
      check.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }  

   /**  
    * Get business service availability
    * 
    * @param businessServiceid business service id 
    * @param timePeriod time period
    * @return uptime for selected period 
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public double getBusinessServiceAvailablity(long businessServiceid, TimePeriod timePeriod) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_BUSINESS_SERVICE_UPTIME);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, businessServiceid);
      msg.setField(NXCPCodes.VID_TIME_FROM, timePeriod.getPeriodStart());
      msg.setField(NXCPCodes.VID_TIME_TO, timePeriod.getPeriodEnd());
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsDouble(NXCPCodes.VID_BUSINESS_SERVICE_UPTIME);
   }  

   /**  
    * Get business service tickets
    * 
    * @param businessServiceid business service id 
    * @param timePeriod time period
    * @return list of tickets for business service
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<BusinessServiceTicket> getBusinessServiceTickets(long businessServiceid, TimePeriod timePeriod) throws NXCException, IOException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_BUSINESS_SERVICE_TICKETS);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, businessServiceid);
      msg.setField(NXCPCodes.VID_TIME_FROM, timePeriod.getPeriodStart());
      msg.setField(NXCPCodes.VID_TIME_TO, timePeriod.getPeriodEnd());
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      List<BusinessServiceTicket> tickets = new ArrayList<BusinessServiceTicket>();
      int count = response.getFieldAsInt32(NXCPCodes.VID_TICKET_COUNT);
      long fieldId = NXCPCodes.VID_TICKET_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         BusinessServiceTicket ticket = new BusinessServiceTicket(response, fieldId);
         tickets.add(ticket);
         fieldId += 10;
      }
      return tickets;
   }

   /**
    * Send request for last values using prepared message
    *
    * @param rootObjectId root object id
    * @param query qury string
    * @return The DCI values
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public DciValue[] findDCI(long rootObjectId, String query) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FIND_DCI);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, rootObjectId);
      msg.setField(NXCPCodes.VID_SEARCH_PATTERN, query);
      sendMessage(msg);
      final NXCPMessage response = waitForRCC(msg.getMessageId());

      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      DciValue[] list = new DciValue[count];
      long base = NXCPCodes.VID_DCI_VALUES_BASE;
      for(int i = 0; i < count; i++, base += 50)
      {
         list[i] = DciValue.createFromMessage(response, base);
      }

      return list;
   }

   /**
    * Get list of objects that are using specified event
    * @param eventCode code of the event
    * @return list of event references
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<EventReference> getEventReferences(long eventCode) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_EVENT_REFERENCES);
      msg.setFieldUInt32(NXCPCodes.VID_EVENT_CODE, eventCode);
      sendMessage(msg);

      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<EventReference> eventReferences = new ArrayList<EventReference>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         eventReferences.add(new EventReference(response, fieldId));
         fieldId += 100;
      }
      return eventReferences;
   }

   /**
    * Get list of maintenance journal entries for specified object and it's child objects
    * @param objectId journal owner object ID
    * @param maxEntries upper entries count limit. No limit used if set to 0
    * @return list of maintenance journal entries
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<MaintenanceJournalEntry> getAllMaintenanceEntries(long objectId, int maxEntries) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_READ_MAINTENANCE_JOURNAL);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setFieldInt32(NXCPCodes.VID_MAX_RECORDS, maxEntries);
      sendMessage(msg);

      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<MaintenanceJournalEntry> maintenanceEntries = new ArrayList<MaintenanceJournalEntry>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         maintenanceEntries.add(new MaintenanceJournalEntry(response, fieldId));
         fieldId += 10;
      }
      return maintenanceEntries;
   }

   /**
    * Create new maintenance journal entry for specified object
    * 
    * @param objectId journal owner object ID
    * @param description journal entry description
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void createMaintenanceEntry(long objectId, String description) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_WRITE_MAINTENANCE_JOURNAL);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Edit specified maintenance journal entry for specified object
    * 
    * @param objectId journal owner object ID
    * @param entryId journal entry ID
    * @param description journal entry description
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void editMaintenanceEntry(long objectId, long entryId, String description) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_MAINTENANCE_JOURNAL);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      msg.setFieldInt32(NXCPCodes.VID_RECORD_ID, (int)entryId);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
   
   /**
    * Create network map clone
    * 
    * @param mapId map id to clone
    * @param newObjectName name of the new object
    * @param alias new object alias
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void cloneNetworkMap(long mapId, String newObjectName, String alias) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CLONE_MAP);
      msg.setFieldInt32(NXCPCodes.VID_MAP_ID, (int)mapId);
      msg.setField(NXCPCodes.VID_NAME, newObjectName);
      msg.setField(NXCPCodes.VID_ALIAS, alias);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Get vendor name by OUI part of MAC address. Local cache is checked first, and only if OUI is not found in local cache request
    * to server is made. If server request is made method will return null and queue background request to the server, and if
    * completion callback is provided it will be called after local cache is populated, so next request to
    * <code>getVendorByMac</code> will complete without request to the server.
    *
    * @param mac MAC address
    * @param callback request completion callback
    * @return vendor name, could be null or empty string if OUI is not known (yet)
    */
   public String getVendorByMac(MacAddress mac, Runnable callback)
   {
      return (ouiCache != null) ? ouiCache.getVendor(mac, callback) : null;
   }

   /**
    * Get vendor names by OUI part of provided MAC addresses. Resulting map will contain all unique OUIs extracted from provided MAC
    * addresses.
    *
    * @param macList list of MAC addresses
    * @return OUI to vendor map
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<MacAddress, String> getVendorByMac(Set<MacAddress> macList) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_FIND_VENDOR_BY_MAC);
      msg.setFieldInt32(NXCPCodes.VID_NUM_ELEMENTS, macList.size());
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for (MacAddress mac : macList)
         msg.setField(fieldId++, mac);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      Map<MacAddress, String> result = new HashMap<MacAddress, String>(count);
      fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         result.put(response.getFieldAsMacAddress(fieldId), response.getFieldAsString(fieldId + 1));         
         fieldId += 2;
      }
      return result;
   }
   
   /**
    * Synchronize asset management schema.
    *
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncAssetManagementSchema() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ASSET_MANAGEMENT_SCHEMA);
      sendMessage(msg);

      final NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ASSET_ATTRIBUTES);
      long fieldId = NXCPCodes.VID_AM_ATTRIBUTES_BASE;
      synchronized(assetManagementSchema)
      {
         assetManagementSchema.clear();
         for(int i = 0; i < count; i++)
         {
            AssetAttribute attr = new AssetAttribute(response, fieldId);
            assetManagementSchema.put(attr.getName(), attr);
            fieldId += 256;
         }
      }
   }

   /**
    * Get asset management schema from client-side cache.
    *
    * @return asset management attributes
    */
   public Map<String, AssetAttribute> getAssetManagementSchema()
   {
      Map<String, AssetAttribute> result;
      synchronized(assetManagementSchema)
      {
         result = new HashMap<String, AssetAttribute>(assetManagementSchema);
      }
      return result;
   }

   /**
    * Get size of asset management schema (number of defined attributes).
    *
    * @return size of asset management schema
    */
   public int getAssetManagementSchemaSize()
   {
      synchronized(assetManagementSchema)
      {
         return assetManagementSchema.size();
      }
   }

   /**
    * Create asset attribute (definition in asset management schema)..
    * 
    * @param attribute attribute to create
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void createAssetAttribute(AssetAttribute attribute) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_ASSET_ATTRIBUTE);
      attribute.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());      
   }

   /**
    * Update asset attribute (definition in asset management schema).
    * 
    * @param attribute attribute to update
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateAssetAttribute(AssetAttribute attribute) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_ASSET_ATTRIBUTE);
      attribute.fillMessage(msg);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());      
   }

   /**
    * Delete asset attribute (definition in asset management schema).
    * 
    * @param name name of attribute to be deleted
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAssetAttribute(String name) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_ASSET_ATTRIBUTE);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());      
   }

   /**
    * Check if asset management attribute name is unique
    * 
    * @param newName new attribute name
    * 
    * @return true if it is unique
    */
   public boolean isAssetAttributeUnique(String newName)
   {
      boolean isUnique;
      synchronized(assetManagementSchema)
      {
         isUnique = !assetManagementSchema.containsKey(newName);
      }
      return isUnique;
   }

   /**
    * Set asset property value.
    * 
    * @param objectId object id
    * @param name property name
    * @param value new value
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setAssetProperty(long objectId, String name, String value) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_ASSET_PROPERTY);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_VALUE, value);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());      
   }

   /**
    * Delete asset property.
    * 
    * @param objectId object id
    * @param name property name
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAssetProperty(long objectId, String name) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_ASSET_PROPERTY);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setField(NXCPCodes.VID_NAME, name);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());         
   }

   /**
    * Link asset object to given object. If asset is already linked to another object, that link will be removed.
    * 
    * @param assetId asset object ID
    * @param objectId other object ID
    * @param updateIdentification if identification filed (serial or MAC address should be updated on link)
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void linkAsset(long assetId, long objectId, boolean updateIdentification) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LINK_ASSET);
      msg.setFieldUInt32(NXCPCodes.VID_ASSET_ID, assetId);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setField(NXCPCodes.VID_UPDATE_IDENTIFICATION, updateIdentification);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Unlink asset from object it is currently linked to. Will do nothing if asset is not linked.
    * 
    * @param assetId asset object ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void unlinkAsset(long assetId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UNLINK_ASSET);
      msg.setFieldUInt32(NXCPCodes.VID_ASSET_ID, assetId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }


   /**
    * Update network map object location 
    * 
    * @param mapId network map id
    * @param elements element list to be updated
    * @param links link list to be updated
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void updateNetworkMapElementPosition(long mapId, Set<NetworkMapElement> elements, Set<NetworkMapLink> links) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_MAP_ELEMENT_UPDATE);      
      msg.setFieldUInt32(NXCPCodes.VID_MAP_ID, mapId);
      msg.setFieldInt32(NXCPCodes.VID_NUM_ELEMENTS, elements.size());
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(NetworkMapElement e : elements)
      {
         e.fillMessage(msg, fieldId);
         fieldId += 100;
      }

      msg.setFieldInt32(NXCPCodes.VID_NUM_LINKS, links.size());
      fieldId = NXCPCodes.VID_LINK_LIST_BASE;
      for(NetworkMapLink l : links)
      {
         l.fillMessage(msg, fieldId);
         fieldId += 20;
      }
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * @return the networkMapDefaultWidth
    */
   public int getNetworkMapDefaultWidth()
   {
      return networkMapDefaultWidth;
   }

   /**
    * @return the networkMapDefaultHeight
    */
   public int getNetworkMapDefaultHeight()
   {
      return networkMapDefaultHeight;
   }

   /**
    * Compile MIBs 
    * 
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void compileMibs(final Consumer<MibCompilationLogEntry> outputCallback) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_COMPILE_MIB_FILES);
      sendMessage(msg);
      
      if (outputCallback != null)
      {
         addMessageSubscription(NXCPCodes.CMD_COMMAND_OUTPUT, msg.getMessageId(), new MessageHandler() {
            String buffer = "";
            
            @Override
            public boolean processMessage(NXCPMessage msg)
            {          
               buffer = buffer.concat(msg.getFieldAsString(NXCPCodes.VID_MESSAGE));
               String[] lines = buffer.split("\\r?\\n", -1);
               buffer = lines[lines.length - 1];
               for(int i = 0; i < lines.length - 1; i++)
               {
                  MibCompilationLogEntry entry = MibCompilationLogEntry.createEntry(lines[i]);
                  if (entry != null)
                     outputCallback.accept(entry);
                  else
                  {
                     logger.debug("Failed to parse line: " + lines[i]);
                  }
               }
               return true;
            }
         });
      }

      waitForRCC(msg.getMessageId());
      removeMessageSubscription(NXCPCodes.CMD_COMMAND_OUTPUT, msg.getMessageId());
   }

   /**
    * Execute dashboard script and get return value as map. Content of returned map depends on actual data type of script return value:
    * <ul>
    * <li>For hash map matching map will be returned;
    * <li>For array all elements will be returned as values and keys will be element positions starting as 1;
    * <li>For all other types map will consist of single element with key "1" and script return value as value.
    * </ul>
    * 
    * @param dashboardId ID of the dashboard script should be taken from
    * @param elementId index of the dashboard element script should be taken from
    * @param objectId ID of the object to run script on
    * @return script return value as a map
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Map<String, String> executeScriptedComparisonChartElement(long dashboardId, long elementId, long objectId) throws IOException, NXCException
   {
      NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_DASHBOARD_SCRIPT);
      msg.setFieldUInt32(NXCPCodes.VID_DASHBOARD_ID, dashboardId);
      msg.setFieldUInt32(NXCPCodes.VID_ELEMENT_INDEX, elementId);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());

      response = waitForMessage(NXCPCodes.CMD_SCRIPT_EXECUTION_RESULT, msg.getMessageId());
      return response.getStringMapFromFields(NXCPCodes.VID_ELEMENT_LIST_BASE, NXCPCodes.VID_NUM_ELEMENTS);
   }

   /**
    * Set peer interface for given interface.
    * 
    * @param localInterfaceId ID of interface object to set peer information on.
    * @param peerInterfaceId remote interface object to be set as peer.
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setInterfacePeer(long localInterfaceId, long peerInterfaceId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_PEER_INTERFACE);
      msg.setFieldUInt32(NXCPCodes.VID_LOCAL_INTERFACE_ID, localInterfaceId);
      msg.setFieldUInt32(NXCPCodes.VID_PEER_INTERFACE_ID, peerInterfaceId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Clear peer interface information.
    * 
    * @param interfaceId interface object to clear peer information on
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void clearInterfacePeer(long interfaceId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CLEAR_PEER_INTERFACE);
      msg.setFieldUInt32(NXCPCodes.VID_INTERFACE_ID, interfaceId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Create new AI assistant chat.
    *
    * @return chat ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long createAiAssistantChat() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_AI_ASSISTANT_CHAT);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_CHAT_ID);
   }

   /**
    * Send message to AI assistant within existing chat.
    *
    * @param chatId chat ID
    * @param message new user message
    * @return assistant response
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String updateAiAssistantChat(long chatId, String message) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_QUERY_AI_ASSISTANT);
      msg.setFieldUInt32(NXCPCodes.VID_CHAT_ID, chatId);
      msg.setField(NXCPCodes.VID_MESSAGE, message);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId(), commandTimeout * 10);   // LLM response can take significant amount of time
      return response.getFieldAsString(NXCPCodes.VID_MESSAGE);
   }

   /**
    * Clear AI assistant chat history.
    * 
    * @param chatId chat ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void clearAiAssistantChat(long chatId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CLEAR_AI_ASSISTANT_CHAT);
      msg.setFieldUInt32(NXCPCodes.VID_CHAT_ID, chatId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Delete AI assistant chat.
    * 
    * @param chatId chat ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAiAssistantChat(long chatId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_AI_ASSISTANT_CHAT);
      msg.setFieldUInt32(NXCPCodes.VID_CHAT_ID, chatId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Request AI assistant comment for given alarm.
    *
    * @param alarmId alarm ID
    * @return assistant comment
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String requestAiAssistantComment(long alarmId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_REQUEST_AI_ASSISTANT_COMMENT);
      msg.setFieldUInt32(NXCPCodes.VID_ALARM_ID, alarmId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId(), commandTimeout * 10); // LLM response can take significant amount of time
      return response.getFieldAsString(NXCPCodes.VID_MESSAGE);
   }

   /**
    * Get list of available AI assistant functions.
    * 
    * @return list of available AI assistant functions
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AiAssistantFunction> getAiAssistantFunctions() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AI_ASSISTANT_FUNCTIONS);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<AiAssistantFunction> functions = new ArrayList<>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         functions.add(new AiAssistantFunction(response, fieldId));
         fieldId += 10;
      }
      return functions;
   }

   /**
    * Call AI assistant function. Intended for use by MCP servers.
    *
    * @param name function name
    * @param arguments function arguments in JSON format
    * @return function return value
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public String callAiAssistantFunction(String name, String arguments) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_CALL_AI_ASSISTANT_FUNCTION);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_ARGUMENTS, arguments);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsString(NXCPCodes.VID_MESSAGE);
   }

   /**
    * Get list of AI agent tasks.
    * 
    * @return list of AI agent tasks
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public List<AiAgentTask> getAiAgentTasks() throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_AI_AGENT_TASKS);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      int count = response.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      List<AiAgentTask> tasks = new ArrayList<>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         tasks.add(new AiAgentTask(response, fieldId));
         fieldId += 20;
      }
      return tasks;
   }

   /**
    * Delete AI agent task.
    * 
    * @param taskId task ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteAiAgentTask(long taskId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_AI_AGENT_TASK);
      msg.setFieldUInt32(NXCPCodes.VID_TASK_ID, taskId);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }

   /**
    * Create AI agent task.
    *
    * @param description task description
    * @param prompt task prompt
    * @return created task ID
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long createAiAgentTask(String description, String prompt) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_ADD_AI_AGENT_TASK);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setField(NXCPCodes.VID_PROMPT, prompt);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      return response.getFieldAsInt64(NXCPCodes.VID_TASK_ID);
   }

   /**
    * Get interface traffic DCIs
    * 
    * @param interfaceId interface id to find DCIs for
    * @return DCI and it's unit: 4 DCIs - 2 pairs (first are trafic, second utilization).
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public InterfaceTrafficDcis getInterfaceTrafficDcis(long interfaceId) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_INTERFACE_TRAFFIC_DCIS);
      msg.setFieldUInt32(NXCPCodes.VID_INTERFACE_ID, interfaceId);
      sendMessage(msg);
      NXCPMessage response = waitForRCC(msg.getMessageId());
      Long[] dciList = response.getFieldAsUInt32ArrayEx(NXCPCodes.VID_DCI_IDS);
      String[] unitNames = new String[4];
      long base = NXCPCodes.VID_UNIT_NAMES_BASE;
      for (int i = 0; i < 4; i++)
         unitNames[i] = response.getFieldAsString(base++);
      return new InterfaceTrafficDcis(dciList, unitNames);
   }

   /**
    * Auto link nodes based on L2 topology information
    * 
    * @param mapId network map id
    * @param nodes list of nodes to link
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void autoLinkNetworkMapNodes(long mapId, Collection<Long> nodes) throws IOException, NXCException
   {
      final NXCPMessage msg = newMessage(NXCPCodes.CMD_LINK_NETWORK_MAP_NODES);
      msg.setFieldUInt32(NXCPCodes.VID_MAP_ID, mapId);
      msg.setField(NXCPCodes.VID_NODE_LIST, nodes);
      sendMessage(msg);
      waitForRCC(msg.getMessageId());
   }
}
