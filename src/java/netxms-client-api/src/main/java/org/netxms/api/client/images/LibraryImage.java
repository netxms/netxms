package org.netxms.api.client.images;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

public class LibraryImage
{
	private String guid;
	private String name;
	private String category;
	private String mimeType;
	private byte[] binaryData;
	private boolean imageProtected;
	private boolean complete = false;

	public LibraryImage(final String guid)
	{
		this.guid = guid;
	}

	public LibraryImage(final String guid, final String name, final String category, final String mimeType,
			final boolean imageProtected)
	{
		this.guid = guid;
		this.name = name;
		this.category = category;
		this.mimeType = mimeType;
		this.imageProtected = imageProtected;
	}

	/**
	 * Create object from NXCP message
	 * 
	 * @param msg
	 *           Message containing object's data
	 */
	public LibraryImage(final NXCPMessage msg)
	{
		guid = msg.getVariableAsString(NXCPCodes.VID_GUID);
		name = msg.getVariableAsString(NXCPCodes.VID_NAME);
		category = msg.getVariableAsString(NXCPCodes.VID_CATEGORY);
		binaryData = msg.getVariableAsBinary(NXCPCodes.VID_IMAGE_DATA);
		mimeType = msg.getVariableAsString(NXCPCodes.VID_IMAGE_MIMETYPE);
		imageProtected = msg.getVariableAsBoolean(NXCPCodes.VID_IMAGE_PROTECTED);
		this.complete = true;
	}

	public LibraryImage()
	{
	}

	/**
	 * Fill NXCP message with object data
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		msg.setVariable(NXCPCodes.VID_GUID, guid);
		msg.setVariable(NXCPCodes.VID_NAME, name);
		if (category != null)
		{
			msg.setVariable(NXCPCodes.VID_CATEGORY, category);
		}
		if (binaryData != null)
		{
			msg.setVariable(NXCPCodes.VID_IMAGE_DATA, binaryData);
		}
	}

	/**
	 * @return the guid
	 */
	public String getGuid()
	{
		return guid;
	}

	/**
	 * @param guid
	 *           the guid to set
	 */
	public void setGuid(String guid)
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
	 * @param binaryData
	 *           the binaryData to set
	 */
	public void setBinaryData(byte[] binaryData)
	{
		this.binaryData = binaryData;
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
	 * @return the mimeType
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

	@Override
	public String toString()
	{
		return "LibraryImage [guid=" + guid + ", name=" + name + ", category=" + category + ", mimeType=" + mimeType
				+ ", binaryData=" + (binaryData == null ? "[null]" : "[not null]") + ", imageProtected=" + imageProtected
				+ ", complete=" + complete + "]";
	}

}
