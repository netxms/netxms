/**
 * 
 */
package org.netxms.tcpproxy;

import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.TcpProxy;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Zone;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Sample TCP proxy application
 */
public class TcpProxyApp
{
   private static final Logger logger = LoggerFactory.getLogger(TcpProxyApp.class);

   private String server;
   private int serverPort; 
   private String login;
   private String password;
   private String node;
   private InetAddress remoteAddress;
   private int remotePort;
   private int localPort;
   private int sessionId = 0;
   
   public TcpProxyApp(String server, String login, String password, String node, InetAddress remoteAddress, int remotePort, int localPort)
   {
      String[] parts = server.split(":");
      if (parts.length == 2)
      {
         this.server = parts[0];
         this.serverPort = Integer.parseInt(parts[1]);
      }
      else
      {
         this.server = server;
         this.serverPort = 4701;
      }
      this.login = login;
      this.password = password;
      this.node = node;
      this.remoteAddress = remoteAddress;
      this.remotePort = remotePort;
      this.localPort = localPort;
   }

   /**
    * Run proxy 
    */
   private void run() throws Exception
   {
      logger.info("Connecting to NetXMS server " + server + " as user " + login);
      NXCSession session = new NXCSession(server, serverPort);
      session.connect(new int[] { ProtocolVersion.INDEX_TCPPROXY });
      session.login(login, password);

      logger.info("Synchronizing objects");
      session.syncObjects(false);
      AbstractObject object = session.findObjectByName(node);
      if ((object == null) || (!(object instanceof Node) && !(object instanceof Zone)))
         throw new IllegalArgumentException("Node or zone object with given name does not exist");
      logger.info("Found " + ((object instanceof Zone) ? "zone " : "node ") + node + " with ID " + object.getObjectId());

      ServerSocket listener = new ServerSocket(localPort);
      logger.info("Listening on port " + localPort);
      try
      {
         while(true)
         {
            final Socket socket = listener.accept();
            try
            {
               logger.info("Establishing proxy session to " + remoteAddress.getHostAddress() + ":" + remotePort);
               final TcpProxy proxy = session.setupTcpProxy(object.getObjectId(), remoteAddress, remotePort);
               new Session(++sessionId, socket, proxy);
            }
            catch(Exception e)
            {
               socket.close();
               e.printStackTrace();
            }
         }
      }
      finally
      {
         listener.close();
      }
   }

   /**
    * @param args
    */
   public static void main(String[] args)
   {
      if (args.length < 6)
      {
         System.out.println("Required arguments: server login password proxy_node remote_address remote_port local_port");
         return;
      }
      
      try
      {
         new TcpProxyApp(args[0], args[1], args[2], args[3], InetAddress.getByName(args[4]), Integer.parseInt(args[5]), Integer.parseInt(args[6])).run();
      }
      catch(Exception e)
      {
         e.printStackTrace();
      }
   }
}
