/**
 * 
 */
package org.netxms.tcpproxy;

import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import org.netxms.base.Logger;
import org.netxms.base.LoggingFacility;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.TcpProxy;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;

/**
 * Sample TCP proxy application
 */
public class TcpProxyApp
{
   private String server; 
   private String login;
   private String password;
   private String node;
   private InetAddress remoteAddress;
   private int remotePort;
   private int localPort;
   private int sessionId = 0;
   
   public TcpProxyApp(String server, String login, String password, String node, InetAddress remoteAddress, int remotePort, int localPort)
   {
      this.server = server;
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
      print("Connecting to NetXMS server " + server + " as user " + login);
      NXCSession session = new NXCSession(server);
      session.connect(new int[] { ProtocolVersion.INDEX_TCPPROXY });
      session.login(login, password);

      print("Synchronizing objects");
      session.syncObjects();
      AbstractObject object = session.findObjectByName(node);
      if ((object == null) || !(object instanceof Node))
         throw new IllegalArgumentException("Node object with given name does not exist");
      print("Found node " + node + " with ID " + object.getObjectId());

      ServerSocket listener = new ServerSocket(localPort);
      print("Listening on port " + localPort);
      try
      {
         while(true)
         {
            final Socket socket = listener.accept();
            try
            {
               print("Establishing proxy session to " + remoteAddress.getHostAddress() + ":" + remotePort);
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
         print("Required arguments: server login password proxy_node remote_address remote_port local_port");
         return;
      }
      
      Logger.setLoggingFacility(new LoggingFacility() {
         @Override
         public void writeLog(int level, String tag, String message, Throwable t)
         {
            System.out.println(tag + ": " + message);
            if (t != null)
               t.printStackTrace();
         }
      });
      
      try
      {
         new TcpProxyApp(args[0], args[1], args[2], args[3], InetAddress.getByName(args[4]), Integer.parseInt(args[5]), Integer.parseInt(args[6])).run();
      }
      catch(Exception e)
      {
         e.printStackTrace();
      }
   }

   private static void print(String text)
   {
      System.out.println(text);
   }
}
