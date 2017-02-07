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
	
	/**
	 * Create new "range" element
	 * 
	 * @param baseAddress FIXME
	 * @param endAddress FIXME
	 */
	public InetAddressListElement(InetAddress baseAddress, InetAddress endAddress)
	{
		this.type = RANGE;
		this.baseAddress = baseAddress;
		this.endAddress = endAddress;
		this.maskBits = 0;
	}

   /**
    * Create new "subnet" element
    * 
    * @param baseAddress FIXME
    * @param maskBits FIXME
    */
   public InetAddressListElement(InetAddress baseAddress, int maskBits)
   {
      this.type = SUBNET;
      this.baseAddress = baseAddress;
      this.endAddress = null;
      this.maskBits = maskBits;
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
	}

	/**
	 * Fill NXCP message
	 * 
	 * @param msg FIXME
	 * @param baseId FIXME
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
   {
	   msg.setFieldInt16(baseId, type);
	   msg.setField(baseId + 1, baseAddress);
	   if (type == SUBNET)
	      msg.setFieldInt16(baseId + 2, maskBits);
	   else
	      msg.setField(baseId + 2, endAddress);
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

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return (type == SUBNET) ? baseAddress.getHostAddress() + "/" + maskBits : baseAddress.getHostAddress() + " - " + endAddress.getHostAddress();
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
      result = prime * result + type;
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
      if (type != other.type)
         return false;
      return true;
   }
}
