package org.netxms.tcpproxy;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Zone;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * TCP port forwarding tool that forwards local TCP ports through NetXMS agents to remote targets.
 */
public class TcpProxyApp
{
   private static final Logger logger = LoggerFactory.getLogger(TcpProxyApp.class);

   private String server;
   private int serverPort = 4701;
   private String login;
   private String password;
   private String token;
   private boolean debug = false;
   private List<ForwardingRule> rules = new ArrayList<>();

   public static void main(String[] args)
   {
      TcpProxyApp app = new TcpProxyApp();
      try
      {
         if (!app.parseArguments(args))
            return;
         app.run();
      }
      catch(Exception e)
      {
         logger.error("Fatal error", e);
         System.exit(1);
      }
   }

   /**
    * Parse command line arguments with environment variable fallback.
    *
    * @return true if arguments are valid and execution should continue
    */
   private boolean parseArguments(String[] args)
   {
      for(int i = 0; i < args.length; i++)
      {
         switch(args[i])
         {
            case "-H":
            case "--host":
               if (++i >= args.length)
               {
                  System.err.println("Option " + args[i - 1] + " requires an argument");
                  return false;
               }
               parseServer(args[i]);
               break;
            case "-u":
            case "--user":
               if (++i >= args.length)
               {
                  System.err.println("Option " + args[i - 1] + " requires an argument");
                  return false;
               }
               login = args[i];
               break;
            case "-P":
            case "--password":
               if (++i >= args.length)
               {
                  System.err.println("Option " + args[i - 1] + " requires an argument");
                  return false;
               }
               password = args[i];
               break;
            case "-t":
            case "--token":
               if (++i >= args.length)
               {
                  System.err.println("Option " + args[i - 1] + " requires an argument");
                  return false;
               }
               token = args[i];
               break;
            case "-L":
            case "--forward":
               if (++i >= args.length)
               {
                  System.err.println("Option " + args[i - 1] + " requires an argument");
                  return false;
               }
               rules.add(ForwardingRule.parse(args[i]));
               break;
            case "-d":
            case "--debug":
               debug = true;
               break;
            case "-h":
            case "--help":
               printUsage();
               return false;
            default:
               System.err.println("Unknown option: " + args[i]);
               printUsage();
               return false;
         }
      }

      if (server == null)
      {
         String env = System.getenv("NETXMS_SERVER");
         if (env != null)
            parseServer(env);
      }
      if (login == null)
         login = System.getenv("NETXMS_LOGIN");
      if (password == null)
         password = System.getenv("NETXMS_PASSWORD");
      if (token == null)
         token = System.getenv("NETXMS_TOKEN");

      if (debug)
         System.setProperty("nxtcpproxy.debug", "true");

      if (server == null)
      {
         System.err.println("Server address is required (-H or NETXMS_SERVER)");
         return false;
      }
      if (token == null && login == null)
      {
         System.err.println("Authentication is required (-u/-P or -t, or NETXMS_LOGIN/NETXMS_PASSWORD/NETXMS_TOKEN)");
         return false;
      }
      if (rules.isEmpty())
      {
         System.err.println("At least one forwarding rule (-L) is required");
         return false;
      }

      Set<Integer> ports = new HashSet<>();
      for(ForwardingRule rule : rules)
      {
         if (!ports.add(rule.getLocalPort()))
         {
            System.err.println("Duplicate local port: " + rule.getLocalPort());
            return false;
         }
      }

      return true;
   }

   private void parseServer(String serverSpec)
   {
      String[] parts = serverSpec.split(":");
      if (parts.length == 2)
      {
         server = parts[0];
         serverPort = Integer.parseInt(parts[1]);
      }
      else
      {
         server = serverSpec;
      }
   }

   private void run() throws Exception
   {
      logger.info("Connecting to NetXMS server {}:{}", server, serverPort);
      NXCSession session = new NXCSession(server, serverPort);
      session.connect(new int[] { ProtocolVersion.INDEX_TCPPROXY });

      if (token != null)
      {
         logger.info("Authenticating with token");
         session.login(token);
      }
      else
      {
         logger.info("Authenticating as {}", login);
         session.login(login, password);
      }

      session.enableReconnect(true);

      logger.info("Synchronizing objects");
      session.syncObjects(false);

      // Resolve unique proxy node names to object IDs
      Map<String, Long> proxyNodeCache = new HashMap<>();
      for(ForwardingRule rule : rules)
      {
         String nodeName = rule.getProxyNode();
         if (proxyNodeCache.containsKey(nodeName))
            continue;

         AbstractObject object = session.findObjectByName(nodeName);
         if ((object == null) || (!(object instanceof Node) && !(object instanceof Zone)))
            throw new IllegalArgumentException("Proxy node or zone \"" + nodeName + "\" not found");

         proxyNodeCache.put(nodeName, object.getObjectId());
         logger.info("Resolved proxy {} \"{}\" (ID {})", (object instanceof Zone) ? "zone" : "node",
               nodeName, object.getObjectId());
      }

      // Create and start all forwarders
      List<PortForwarder> forwarders = new ArrayList<>();
      for(ForwardingRule rule : rules)
      {
         long proxyObjectId = proxyNodeCache.get(rule.getProxyNode());
         PortForwarder forwarder = new PortForwarder(session, proxyObjectId, rule);
         forwarders.add(forwarder);
         forwarder.start();
      }

      CountDownLatch shutdownLatch = new CountDownLatch(1);
      Runtime.getRuntime().addShutdownHook(new Thread(() -> {
         logger.info("Shutting down");
         for(PortForwarder f : forwarders)
         {
            f.close();
         }
         session.disconnect();
         shutdownLatch.countDown();
      }, "Shutdown"));

      logger.info("All forwarders started, {} rule(s) active", forwarders.size());
      shutdownLatch.await();
   }

   private static void printUsage()
   {
      System.err.println("Usage: nxtcpproxy [options]");
      System.err.println();
      System.err.println("Options:");
      System.err.println("  -H, --host <host[:port]>     Server address (default port 4701)");
      System.err.println("  -u, --user <user>            Login name");
      System.err.println("  -P, --password <password>    Password");
      System.err.println("  -t, --token <token>          Authentication token (alternative to user/password)");
      System.err.println("  -L, --forward <rule>         Forwarding rule (repeatable)");
      System.err.println("  -d, --debug                  Enable debug logging");
      System.err.println("  -h, --help                   Print this help message");
      System.err.println();
      System.err.println("Forwarding rule format: localPort:proxyNode:remoteAddr:remotePort");
      System.err.println();
      System.err.println("Environment variables:");
      System.err.println("  NETXMS_SERVER    Server address (fallback for -H)");
      System.err.println("  NETXMS_LOGIN     Login name (fallback for -u)");
      System.err.println("  NETXMS_PASSWORD  Password (fallback for -P)");
      System.err.println("  NETXMS_TOKEN     Auth token (fallback for -t)");
      System.err.println();
      System.err.println("Example:");
      System.err.println("  nxtcpproxy -H server.local -u admin -P secret \\");
      System.err.println("    -L 2222:proxy-node:10.0.0.5:22 \\");
      System.err.println("    -L 3389:proxy-node:10.0.0.5:3389");
   }
}
