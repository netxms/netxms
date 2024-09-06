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
   public static String SERVER_PASSWORD = "netxms";
   public static String SERVER_LOGIN_2FA = "test2fa";
   public static String SERVER_PASSWORD_2FA = "test";

   // Other constants
   public static String TEST_NODE_1 = "10.5.4.2"; /* remote test node */
   public static String TEST_NODE_2 = "10.5.4.3";
   public static String TEST_NODE_3 = "10.5.4.4";
   public static long SUBNET_ID = 4796;
   public static String FILE_NAME = "/opt/jenkins/netxms/log/nxagentd";
   public static int FILE_OFFSET = 0;
   public static String ACTION = "netstat";
   public static int EVENT_CODE = 100000;
   public static int CONNECTION_POOL = 100;
   public static int USER_ID = 1;
   public static String IMAGE_CATEGORY = "Network Objects";
   public static String MOBILE_DEVICE_IMEI = "0000000000";
   public static int AGENT_PORT = 5019; /* remote test node */

   // Reinitialize variables from properties file
   public static TestConstants testConstants = new TestConstants();

   public TestConstants()
   {
      Properties properties = new Properties();
      properties.putAll(System.getProperties());

      String propFile = "/etc/test.properties";
      if (System.getProperty("ConfigFile") != null && !System.getProperty("ConfigFile").isEmpty())
         propFile = System.getProperty("ConfigFile");

      try (InputStream stream = new FileInputStream(propFile))
      {
         properties.load(stream);
      }
      catch (Exception e)
      {
         System.out.println("Cannot load properties file: " + e.getMessage());
      }

      TEST_NODE_1 = properties.getProperty("test.node.1.ip", "10.5.4.2");
      TEST_NODE_2 = properties.getProperty("test.node.2.ip", "10.5.4.3");
      TEST_NODE_3 = properties.getProperty("test.node.3.ip", "10.5.4.4");
      SERVER_ADDRESS = properties.getProperty("server.address", "127.0.0.1");
      SERVER_PORT_CLIENT = Integer.parseInt(properties.getProperty("server.port.client", Integer.toString(NXCSession.DEFAULT_CONN_PORT)));
      SERVER_PORT_MOBILE_AGENT = Integer.parseInt(properties.getProperty("server.port.mobile-agent", Integer.toString(4747)));
      SERVER_LOGIN = properties.getProperty("server.login", "admin");
      SERVER_PASSWORD = properties.getProperty("server.password", "netxms");
      SUBNET_ID = Integer.parseInt(properties.getProperty("objects.subnet.id", "497"));
      MOBILE_DEVICE_IMEI = properties.getProperty("objects.mobile-device.imei", "0000000000");
      FILE_NAME = properties.getProperty("file.name", "/opt/jenkins/netxms/log/nxagentd");
      FILE_OFFSET = Integer.parseInt(properties.getProperty("file.offset", "1000"));
      ACTION = properties.getProperty("action", "netstat");
      EVENT_CODE = Integer.parseInt(properties.getProperty("event.code", "29"));
      CONNECTION_POOL = Integer.parseInt(properties.getProperty("connection.pull", "100"));
      USER_ID = Integer.parseInt(properties.getProperty("user.id", "1"));
      AGENT_PORT = Integer.parseInt(properties.getProperty("agent.port", "5019"));
   }
}
