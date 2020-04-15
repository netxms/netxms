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

import java.net.InetAddress;
import org.netxms.base.NXCPMessage;

/**
 * Element of IP address list. Can represent either subnet or address range.
 */
public class InetAddressListElement
{
	public static final int SUBNET = 0;
	public static final int RANGE = 1;
	
	private int type;
	private InetAddress baseAddress;
	private InetAddress endAddress;
	private int maskBits;
	private int zoneUIN;
	private long proxyId;
	private String comment;
	
	/**
	 * Create new "range" element
	 * 
	 * @param baseAddress base address
	 * @param endAddress end address
	 * @param zoneUIN zone UIN
	 * @param proxyId proxy node ID
	 * @param comment element comment
	 */
	public InetAddressListElement(InetAddress baseAddress, InetAddress endAddress, int zoneUIN, long proxyId, String comment)
	{
		this.type = RANGE;
		this.baseAddress = baseAddress;
		this.endAddress = endAddress;
		this.maskBits = 0;
		this.zoneUIN = zoneUIN;
		this.proxyId = proxyId;
		this.comment = comment;
	}

   /**
    * Create new "subnet" element
    * 
    * @param baseAddress base address
    * @param maskBits mask bits
    * @param zoneUIN zone UIN
    * @param proxyId proxy node ID
    * @param comment element comment
    */
   public InetAddressListElement(InetAddress baseAddress, int maskBits, int zoneUIN, long proxyId, String comment)
   {
      this.type = SUBNET;
      this.baseAddress = baseAddress;
      this.endAddress = null;
      this.maskBits = maskBits;
      this.zoneUIN = zoneUIN;
      this.proxyId = proxyId;
      this.comment = comment;
   }

   /**
    * Update object method
    * 
    * @param baseAddress base address
    * @param endAddress end address
    * @param zoneUIN zone UIN
    * @param proxyId proxy node ID
    * @param comment element comment
    */
   public void update(InetAddress baseAddress, InetAddress endAddress, int zoneUIN, long proxyId, String comment)
   {
      this.type = RANGE;
      this.baseAddress = baseAddress;
      this.endAddress = endAddress;
      this.maskBits = 0;
      this.zoneUIN = zoneUIN;
      this.proxyId = proxyId;
      this.comment = comment;
   }
   
   /**
    * Update object method
    * 
    * @param baseAddress base address
    * @param maskBits mask bits
    * @param zoneUIN zone UIN
    * @param proxyId proxy node ID
    * @param comment element comment
    */
   public void update(InetAddress baseAddress, int maskBits, int zoneUIN, long proxyId, String comment)
   {
      this.type = SUBNET;
      this.baseAddress = baseAddress;
      this.endAddress = null;
      this.maskBits = maskBits;
      this.zoneUIN = zoneUIN;
      this.proxyId = proxyId;
      this.comment = comment;      
   }

	/**
	 * Create element from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	protected InetAddressListElement(NXCPMessage msg, long baseId)
	{
		type = msg.getFieldAsInt32(baseId);
		baseAddress = msg.getFieldAsInetAddress(baseId + 1);
		if (type == SUBNET)
		{
		   maskBits = msg.getFieldAsInt32(baseId + 2);
		   endAddress = null;
		}
		else
		{
		   endAddress = msg.getFieldAsInetAddress(baseId + 2);
		   maskBits = 0;
		}
      zoneUIN = msg.getFieldAsInt32(baseId + 3);
      proxyId = msg.getFieldAsInt64(baseId + 4);
      comment = msg.getFieldAsString(baseId + 5);
	}

	/**
	 * Fill NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId object base id
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
   {
	   msg.setFieldInt16(baseId, type);
	   msg.setField(baseId + 1, baseAddress);
	   if (type == SUBNET)
	      msg.setFieldInt16(baseId + 2, maskBits);
	   else
	      msg.setField(baseId + 2, endAddress);
      msg.setFieldInt32(baseId + 3, (int)zoneUIN);
      msg.setFieldInt32(baseId + 4, (int)proxyId);
      msg.setField(baseId + 5, comment);
   }

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

   /**
    * @return the baseAddress
    */
   public InetAddress getBaseAddress()
   {
      return baseAddress;
   }

   /**
    * @return the endAddress
    */
   public InetAddress getEndAddress()
   {
      return endAddress;
   }

   /**
    * @return the maskBits
    */
   public int getMaskBits()
   {
      return maskBits;
   }

   /**
    * @return the zoneUIN
    */
   public int getZoneUIN()
   {
      return zoneUIN;
   }

   /**
    * @return the proxyId
    */
   public long getProxyId()
   {
      return proxyId;
   }

   /**
    * @return the comment
    */
   public String getComment()
   {
      return comment;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "InetAddressListElement [type=" + type + ", baseAddress=" + baseAddress + ", endAddress=" + endAddress + ", maskBits="
            + maskBits + ", zoneUIN=" + zoneUIN + ", proxyId=" + proxyId + "]";
   }

   /* (non-Javadoc)
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      final int prime = 31;
      int result = 1;
      result = prime * result + ((baseAddress == null) ? 0 : baseAddress.hashCode());
      result = prime * result + ((endAddress == null) ? 0 : endAddress.hashCode());
      result = prime * result + maskBits;
      result = prime * result + (int)(proxyId ^ (proxyId >>> 32));
      result = prime * result + type;
      result = prime * result + (int)(zoneUIN ^ (zoneUIN >>> 32));
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
      InetAddressListElement other = (InetAddressListElement)obj;
      if (baseAddress == null)
      {
         if (other.baseAddress != null)
            return false;
      }
      else if (!baseAddress.equals(other.baseAddress))
         return false;
      if (endAddress == null)
      {
         if (other.endAddress != null)
            return false;
      }
      else if (!endAddress.equals(other.endAddress))
         return false;
      if (maskBits != other.maskBits)
         return false;
      if (proxyId != other.proxyId)
         return false;
      if (type != other.type)
         return false;
      if (zoneUIN != other.zoneUIN)
         return false;
      return true;
   }
   
   public boolean isSubnet()
   {
      return type == SUBNET;
   }
}
