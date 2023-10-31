package org.netxms.tests;

import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Properties;
import org.netxms.client.NXCSession;

/******************************************************************
 * Documentation about test server configuration
 ******************************************************************
 * Test servers ID should be set as LOCAL_NODE_ID
 *
 * Test server agent configuration should contain:
 *    EnableActions = yes
 *    Action = echo: echo "Hi"
 *    Subagent = filemgr.nsm
 *
 *    [filemgr]
 *    RootFolder = /var/log/netxms/
 *
 * Test server configuration should contain:
 *    LogFile = /var/log/netxms/netxmsd
 ******************************************************************/
public class TestConstants
{   
   // Server connection constants
   public static String SERVER_ADDRESS = "127.0.0.1";
   public static int SERVER_PORT_CLIENT = NXCSession.DEFAULT_CONN_PORT;
   public static int SERVER_PORT_MOBILE_AGENT = 4747;
   public static String SERVER_LOGIN = "admin";
   public static String SERVER_PASSWORD = "";

   // Other constants
   public static String TEST_NODE_1 = "10.5.4.2"; /* remote test node */
   public static String TEST_NODE_2 = "10.5.4.3";
   public static String TEST_NODE_3 = "10.5.4.4";
   public static int TEST_NODE_ID = 4946; /* remote test node */
   public static int LOCAL_NODE_ID = 9190; /* server node itself */
   public static long SUBNET_ID = 4796;
   public static String FILE_NAME = "/opt/netxms/log/nxagentd";
   public static int FILE_OFFSET = 0;
   public static String ACTION = "netstat";
   public static int EVENT_CODE = 100000;
   public static int CONNECTION_POOL = 100;
   public static int USER_ID = 1;
   public static String IMAGE_CATEGORY = "Network Objects";
   public static String MOBILE_DEVICE_IMEI = "0000000000";

   // Reinitialize variables from properties file
   public static TestConstants testConstants = new TestConstants();

   public TestConstants()
   {
      try
      {
         String propFile = "/etc/test.properties";
         if (System.getProperty("ConfigFile") != null && !System.getProperty("ConfigFile").isEmpty())
            propFile = System.getProperty("ConfigFile");
         InputStream stream = new FileInputStream(propFile);
         Properties properties = new Properties();
         properties.load(stream);
         SERVER_ADDRESS = properties.getProperty("server.address", "127.0.0.1");
         SERVER_PORT_CLIENT = Integer.parseInt(properties.getProperty("server.port.client", Integer.toString(NXCSession.DEFAULT_CONN_PORT)));
         SERVER_PORT_MOBILE_AGENT = Integer.parseInt(properties.getProperty("server.port.mobile-agent", Integer.toString(4747)));
         SERVER_LOGIN = properties.getProperty("server.login", "admin");
         SERVER_PASSWORD = properties.getProperty("server.password", "");
         TEST_NODE_ID = Integer.parseInt(properties.getProperty("objects.node.test.id", "100"));
         LOCAL_NODE_ID = Integer.parseInt(properties.getProperty("objects.node.local.id", "100"));
         SUBNET_ID = Integer.parseInt(properties.getProperty("objects.subnet.id", "497"));
         MOBILE_DEVICE_IMEI = properties.getProperty("objects.mobile-device.imei", "0000000000");
         FILE_NAME = properties.getProperty("file.name", "/var/log/netxms/netxmsd");
         FILE_OFFSET = Integer.parseInt(properties.getProperty("file.offset", "1000"));
         ACTION = properties.getProperty("action", "echo");
         EVENT_CODE = Integer.parseInt(properties.getProperty("event.code", "29"));
         CONNECTION_POOL = Integer.parseInt(properties.getProperty("connection.pull", "100"));
         USER_ID = Integer.parseInt(properties.getProperty("user.id", "1"));
      }
      catch (Exception e)
      {
         System.out.println("No properties file found: " + e.getMessage());
      }
   }
}
