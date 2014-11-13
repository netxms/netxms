/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.client.snmp;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import org.netxms.base.NXCPDataInputStream;
import org.netxms.client.NXCException;
import org.netxms.client.constants.RCC;

/**
 * This class represents single MIB tree object.
 *
 */
public class MibObject
{
	public static final int STATUS_MANDATORY              = 1;
	public static final int STATUS_OPTIONAL               = 2;
	public static final int STATUS_OBSOLETE               = 3;
	public static final int STATUS_DEPRECATED             = 4;
	public static final int STATUS_CURRENT                = 5;
	
	public static final int ACCESS_READONLY               = 1;
	public static final int ACCESS_READWRITE              = 2;
	public static final int ACCESS_WRITEONLY              = 3;
	public static final int ACCESS_NOACCESS               = 4;
	public static final int ACCESS_NOTIFY                 = 5;
	public static final int ACCESS_CREATE                 = 6;

	public static final int MIB_TYPE_OTHER                = 0;
	public static final int MIB_TYPE_IMPORT_ITEM          = 1;
	public static final int MIB_TYPE_OBJID                = 2;
	public static final int MIB_TYPE_BITSTRING            = 3;
	public static final int MIB_TYPE_INTEGER              = 4;
	public static final int MIB_TYPE_INTEGER32            = 5;
	public static final int MIB_TYPE_INTEGER64            = 6;
	public static final int MIB_TYPE_UNSIGNED32           = 7;
	public static final int MIB_TYPE_COUNTER              = 8;
	public static final int MIB_TYPE_COUNTER32            = 9;
	public static final int MIB_TYPE_COUNTER64            = 10;
	public static final int MIB_TYPE_GAUGE                = 11;
	public static final int MIB_TYPE_GAUGE32              = 12;
	public static final int MIB_TYPE_TIMETICKS            = 13;
	public static final int MIB_TYPE_OCTETSTR             = 14;
	public static final int MIB_TYPE_OPAQUE               = 15;
	public static final int MIB_TYPE_IPADDR               = 16;
	public static final int MIB_TYPE_PHYSADDR             = 17;
	public static final int MIB_TYPE_NETADDR              = 18;
	public static final int MIB_TYPE_NAMED_TYPE           = 19;
	public static final int MIB_TYPE_SEQID                = 20;
	public static final int MIB_TYPE_SEQUENCE             = 21;
	public static final int MIB_TYPE_CHOICE               = 22;
	public static final int MIB_TYPE_TEXTUAL_CONVENTION   = 23;
	public static final int MIB_TYPE_MACRO_DEFINITION     = 24;
	public static final int MIB_TYPE_MODCOMP              = 25;
	public static final int MIB_TYPE_TRAPTYPE             = 26;
	public static final int MIB_TYPE_NOTIFTYPE            = 27;
	public static final int MIB_TYPE_MODID                = 28;
	public static final int MIB_TYPE_NSAPADDRESS          = 29;
	public static final int MIB_TYPE_AGENTCAP             = 30;
	public static final int MIB_TYPE_UINTEGER             = 31;
	public static final int MIB_TYPE_NULL                 = 32;
	public static final int MIB_TYPE_OBJGROUP             = 33;
	public static final int MIB_TYPE_NOTIFGROUP           = 34;
	
	protected static final int MIB_TAG_OBJECT             = 0x01;
	protected static final int MIB_TAG_NAME               = 0x02;
	protected static final int MIB_TAG_DESCRIPTION        = 0x03;
	protected static final int MIB_TAG_ACCESS             = 0x04;
	protected static final int MIB_TAG_STATUS             = 0x05;
	protected static final int MIB_TAG_TYPE               = 0x06;
	protected static final int MIB_TAG_BYTE_OID           = 0x07;   /* Used if OID < 256 */
	protected static final int MIB_TAG_WORD_OID           = 0x08;   /* Used if OID < 65536 */
	protected static final int MIB_TAG_DWORD_OID          = 0x09;
	protected static final int MIB_TAG_TEXTUAL_CONVENTION = 0x0A;
	protected static final int MIB_END_OF_TAG             = 0x80;

	private long id;
	private String name;
	private String description;
	private String textualConvention;
	private int type;
	private int status;
	private int access;
	private Map<Long, MibObject> childObjects = new HashMap<Long, MibObject>();
	private MibObject parent;
	private SnmpObjectId objectId;
	
	/**
	 * Default constructor - create empty "[root]" object.
	 */
	public MibObject()
	{
		id = 0;
		name = "[root]";
		description = "";
		textualConvention = null;
		type = -1;
		status = -1;
		access = -1;
		parent = null;
		objectId = null;
	}
	
	/**
	 * Read MIB object from compiled MIB file
	 * @param in Input stream attached to MIB file
	 * @throws IOException 
	 * @throws NXCException 
	 */
	protected MibObject(NXCPDataInputStream in, MibObject parent) throws IOException, NXCException
	{
		this.parent = parent;
		name = null;
		description = "";
		
		boolean stop = false;
		while(!stop)
		{
			int tag = in.readUnsignedByte();
			switch(tag)
			{
				case (MIB_TAG_OBJECT | MIB_END_OF_TAG):
					stop = true;
					break;
	         case MIB_TAG_BYTE_OID:
	         	id = in.readUnsignedByte();
	         	break;
	         case MIB_TAG_WORD_OID:
	         	id = in.readUnsignedShort();
	         	break;
	         case MIB_TAG_DWORD_OID:
	         	id = in.readUnsignedInt();
	         	break;
	         case MIB_TAG_NAME:
	         	name = readStringFromStream(in);
	         	break;
	         case MIB_TAG_DESCRIPTION:
	         	description = readStringFromStream(in);
	         	break;
	         case MIB_TAG_TEXTUAL_CONVENTION:
	         	textualConvention = readStringFromStream(in);
	         	break;
	         case MIB_TAG_TYPE:
	         	type = in.readUnsignedByte();
	         	break;
	         case MIB_TAG_STATUS:
	         	status = in.readUnsignedByte();
	         	break;
	         case MIB_TAG_ACCESS:
	         	access = in.readUnsignedByte();
	         	break;
	         case MIB_TAG_OBJECT:
	         	MibObject object = new MibObject(in, this);
	         	childObjects.put(object.getId(), object);
	         	break;
	         default:
	         	throw new NXCException(RCC.BAD_MIB_FILE_DATA);
			}
			
			// Check closing tag for all tags except OBJECT tags (both opening and closing)
			if ((tag & ~MIB_END_OF_TAG) != MIB_TAG_OBJECT)
			{
				int closeTag = in.readUnsignedByte();
				if (((closeTag & MIB_END_OF_TAG) == 0) || ((closeTag & ~MIB_END_OF_TAG) != tag))
	         	throw new NXCException(RCC.BAD_MIB_FILE_DATA);
			}
		}
		
		if (parent == null)
		{
			name = "[root]";
			
			// Set correct full OIDs for all objects in the tree 
			for(MibObject o : childObjects.values())
				o.setObjectIdFromParent(null);
		}
		else if ((name == null) || name.isEmpty())
		{
			name = "#" + Long.toString(id);
		}
	}

	/**
	 * Read string in format <length><value> from input stream
	 * @param in Input stream
	 * @return String read
	 * @throws IOException if I/O error occurs
	 */
	private String readStringFromStream(NXCPDataInputStream in) throws IOException
	{
		int len = in.readUnsignedShort();
		if (len == 0)
			return "";
		
		byte[] buffer = new byte[len];
		in.read(buffer);
		return new String(buffer);
	}
	
	/**
	 * Set OID based on parent's OID
	 * @param oid Parent's OID
	 */
	private void setObjectIdFromParent(SnmpObjectId oid)
	{
		objectId = new SnmpObjectId(oid, id);
		for(MibObject o : childObjects.values())
			o.setObjectIdFromParent(objectId);
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
	
	/**
	 * Get object's full name (including all parents)
	 * 
	 * @return
	 */
	public String getFullName()
	{
		return (parent == null) ? "" : (parent.getFullName() + "." + name);
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return the status
	 */
	public int getStatus()
	{
		return status;
	}

	/**
	 * @return the access
	 */
	public int getAccess()
	{
		return access;
	}
	
	/**
	 * Get all child objects
	 * 
	 * @return Array of child objects
	 */
	public MibObject[] getChildObjects()
	{
		return childObjects.values().toArray(new MibObject[childObjects.size()]);
	}

	/**
	 * @return the parent
	 */
	public MibObject getParent()
	{
		return parent;
	}

	/**
	 * @return the objectId
	 */
	public SnmpObjectId getObjectId()
	{
		return objectId;
	}

	/**
	 * Check if object has child objects.
	 * 
	 * @return true if object has child objects
	 */
	public boolean hasChildren()
	{
		return childObjects.size() > 0;
	}
	
	/**
	 * Find child object with given ID
	 * @param oid
	 * @return
	 */
	protected MibObject findChildObject(SnmpObjectId oid)
	{
		long nextId = oid.getIdFromPos((objectId != null) ? objectId.getLength() : 0); 
		for(MibObject o : childObjects.values())
		{
			if (o.getId() == nextId)
			{
				MibObject result = o.findChildObject(oid);
				return (result != null) ? result : o;
			}
		}
		return null;
	}

	/**
	 * @return the textualConvention
	 */
	public final String getTextualConvention()
	{
		return (textualConvention != null) ? textualConvention : "";
	}
}
