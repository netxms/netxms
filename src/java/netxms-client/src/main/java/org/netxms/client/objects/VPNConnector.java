/**
 * 
 */
package org.netxms.client.objects;

import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.IpAddressListElement;
import org.netxms.client.NXCSession;

/**
 * VPN connector object
 */
public class VPNConnector extends GenericObject
{
   private long peerGatewayId;
   private List<IpAddressListElement> localSubnets;
   private List<IpAddressListElement> remoteSubnets;
   
   /**
    * Create from NXCP message
    * 
    * @param msg
    * @param session
    */
   public VPNConnector(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      
      peerGatewayId = msg.getVariableAsInt64(NXCPCodes.VID_PEER_GATEWAY);
      
      long varId = NXCPCodes.VID_VPN_NETWORK_BASE;
      int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_LOCAL_NETS);
      localSubnets = new ArrayList<IpAddressListElement>(count);
      for(int i = 0; i < count; i++)
      {
         InetAddress addr = msg.getVariableAsInetAddress(varId++);
         InetAddress mask = msg.getVariableAsInetAddress(varId++);
         localSubnets.add(new IpAddressListElement(IpAddressListElement.SUBNET, addr, mask));
      }

      count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_REMOTE_NETS);
      remoteSubnets = new ArrayList<IpAddressListElement>(count);
      for(int i = 0; i < count; i++)
      {
         InetAddress addr = msg.getVariableAsInetAddress(varId++);
         InetAddress mask = msg.getVariableAsInetAddress(varId++);
         remoteSubnets.add(new IpAddressListElement(IpAddressListElement.SUBNET, addr, mask));
      }
   }

   /**
    * Get parent node object.
    * 
    * @return parent node object or null if it is not exist or inaccessible
    */
   public AbstractNode getParentNode()
   {
      AbstractNode node = null;
      synchronized(parents)
      {
         for(Long id : parents)
         {
            AbstractObject object = session.findObjectById(id);
            if (object instanceof AbstractNode)
            {
               node = (AbstractNode)object;
               break;
            }
         }
      }
      return node;
   }
   
   /* (non-Javadoc)
    * @see org.netxms.client.objects.GenericObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "VPNConnector";
   }

   /**
    * @return the peerGatewayId
    */
   public long getPeerGatewayId()
   {
      return peerGatewayId;
   }

   /**
    * @return the localSubnets
    */
   public List<IpAddressListElement> getLocalSubnets()
   {
      return localSubnets;
   }

   /**
    * @return the remoteSubnets
    */
   public List<IpAddressListElement> getRemoteSubnets()
   {
      return remoteSubnets;
   }
}
