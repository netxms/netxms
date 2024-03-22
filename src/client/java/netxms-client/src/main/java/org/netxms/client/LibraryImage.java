/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * This class represents image in image library
 */
public class LibraryImage implements Comparable<LibraryImage>
{
   private static Logger logger = LoggerFactory.getLogger(LibraryImage.class);
   
	private UUID guid;
	private String name;
	private String category;
	private String mimeType;
	private byte[] binaryData;
	private boolean imageProtected;
	private boolean complete = false;

   /**
    * Default constructor
    */
   public LibraryImage()
   {
      guid = NXCommon.EMPTY_GUID;
      name = "";
      category = "Default";
      mimeType = "image/unknown";
   }

	/**
    * Create new library image
    * 
    * @param guid GUID
    * @param name name
    * @param category image category name
    * @param mimeType image MIME type
    */
   public LibraryImage(UUID guid, String name, String category, String mimeType)
	{
		this.guid = guid;
      this.name = name;
      this.category = category;
      this.mimeType = (mimeType != null) ? mimeType : "image/unknown";
	}

   /**
    * Create library image object from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public LibraryImage(NXCPMessage msg, long baseId)
	{
      guid = msg.getFieldAsUUID(baseId);
      name = msg.getFieldAsString(baseId + 1);
      category = msg.getFieldAsString(baseId + 2);
      mimeType = msg.getFieldAsString(baseId + 3);
      imageProtected = msg.getFieldAsBoolean(baseId + 4);
	}

	/**
    * Create library image object from NXCP message and image file
    * 
    * @param msg Message containing object's data
    * @param imageFile image file
    */
   public LibraryImage(NXCPMessage msg, File imageFile)
	{
		guid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
		name = msg.getFieldAsString(NXCPCodes.VID_NAME);
		category = msg.getFieldAsString(NXCPCodes.VID_CATEGORY);
		mimeType = msg.getFieldAsString(NXCPCodes.VID_IMAGE_MIMETYPE);
		imageProtected = msg.getFieldAsBoolean(NXCPCodes.VID_IMAGE_PROTECTED);

		binaryData = new byte[(int)imageFile.length()];
		InputStream in = null;
		try
		{
			in = new FileInputStream(imageFile);
			in.read(binaryData);
		}
		catch(Exception e)
		{
		   logger.error("Failed to load image file", e);
		}
		finally
		{
			try
			{
				if (in != null)
				{
					in.close();
				}
				imageFile.delete();
			}
			catch(Exception e)
			{
			}
		}

		this.complete = true;
	}

	/**
    * Fill NXCP message with image data (metadata only)
    *
    * @param msg NXCP message
    */
	public void fillMessage(final NXCPMessage msg)
	{
      msg.setField(NXCPCodes.VID_GUID, guid);
		msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_CATEGORY, category);
      msg.setField(NXCPCodes.VID_IMAGE_MIMETYPE, mimeType);
	}

	/**
	 * @return the guid
	 */
	public UUID getGuid()
	{
		return guid;
	}

	/**
	 * @param guid
	 *           the guid to set
	 */
	public void setGuid(UUID guid)
	{
		this.guid = guid;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name
	 *           the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the category
	 */
	public String getCategory()
	{
		return category;
	}

	/**
	 * @param category
	 *           the category to set
	 */
	public void setCategory(String category)
	{
		this.category = category;
	}

	/**
	 * @return the binaryData
	 */
	public byte[] getBinaryData()
	{
		return binaryData;
	}

	/**
	 * Set image binary data.
	 *
	 * @param binaryData image binary data
	 * @param mimeType image MIME type (will be set to "image/unknown" if mimeType is null)
	 */
   public void setBinaryData(byte[] binaryData, String mimeType)
	{
		this.binaryData = binaryData;
      this.mimeType = (mimeType != null) ? mimeType : "image/unknown";
	}

	/**
	 * @return is image protected?
	 */
	public boolean isProtected()
	{
		return imageProtected;
	}

	/**
	 * @return is complete (contains binary data)?
	 */
	public boolean isComplete()
	{
		return complete;
	}

	/**
	 * Get image MIME type.
	 *
	 * @return image MIME type
	 */
	public String getMimeType()
	{
		return mimeType;
	}

	/**
	 * @param mimeType
	 *           the mimeType to set
	 */
	public void setMimeType(String mimeType)
	{
		this.mimeType = mimeType;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		return "LibraryImage [guid=" + guid + ", name=" + name + ", category=" + category + ", mimeType=" + mimeType
				+ ", binaryData=" + (binaryData == null ? "[null]" : "[not null]") + ", imageProtected=" + imageProtected
				+ ", complete=" + complete + "]";
	}

   /* (non-Javadoc)
    * @see java.lang.Comparable#compareTo(java.lang.Object)
    */
   @Override
   public int compareTo(LibraryImage o)
   {
      return name.compareToIgnoreCase(o.getName());
   }
}
