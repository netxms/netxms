/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
package org.netxms.api.client.users;

import org.netxms.base.NXCPMessage;

/**
 * NetXMS object tool representation
 *
 */
public class AuthCertificate
{	
	protected long id;
	protected int type;
	protected String subject;
	protected String comments;

	/**
	 * Default implicit constructor.
	 */
	public AuthCertificate()
	{
	  id = 0;
	  type = 0;
	  subject = "";
	  comments = "";
	}
	
	/**
	 * Create certificate from NXCP message. Intended to be called only by NXCSession methods.
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	public AuthCertificate(NXCPMessage msg, long baseId)
	{
		id = msg.getFieldAsInt64(baseId);
		type = msg.getFieldAsInt32(baseId + 1);
		comments = msg.getFieldAsString(baseId + 2);
		subject = msg.getFieldAsString(baseId + 3);
	}
	
	/**
	 * Create certificate and set all fields.
	 * 
	 * @param id Id of certificate, should be set to 0 if new certificate is created.
	 * @param type Type of certificate mapping. Can be USER_MAP_CERT_BY_SUBJECT or USER_MAP_CERT_BY_PUBKEY.
	 * @param certData Certificate in PEM format.
	 * @param subjec Subject of the certificate
	 * @param comments Comments for certificate
	 */
	public AuthCertificate(long id, int type, String certData, String subject, String comments)
	{
	   this.id = id;
	   this.type = type;
	   this.subject = subject;
	   this.comments = comments;
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}
   
   /**
    * @return the subject
    */
   public String getSubject()
   {
      return subject;
   }
   
   /**
    * @return the comments
    */
   public String getComments()
   {
      return comments;
   }

   /**
    * @param id the id to set
    */
   public void setId(long id)
   {
      this.id = id;
   }
   
   /**
    * @param type the type to set
    */
   public void setType(int type)
   {
      this.type = type;
   }
   
   /**
    * @param subject the subject to set
    */
   public void setSubject(String subject)
   {
      this.subject = subject;
   }
   
   /**
    * @param comments the comments to set
    */
   public void setComments(String comments)
   {
      this.comments = comments;
   }
}
