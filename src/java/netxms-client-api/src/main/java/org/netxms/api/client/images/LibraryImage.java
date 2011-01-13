package org.netxms.api.client.images;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

public class LibraryImage
{
	private String name;
	private String category;
	private byte[] binaryData;
	private boolean imageProtected;
	private boolean dirty;

	public LibraryImage(final String name)
	{
		this.name = name;
		this.dirty = true;
	}

	public LibraryImage(final String name, final String category, final boolean imageProtected)
	{
		this.name = name;
		this.category = category;
		this.imageProtected = imageProtected;
		this.dirty = true;
	}

	/**
	 * Create object from NXCP message
	 * 
	 * @param msg
	 *           Message containing object's data
	 */
	public LibraryImage(final NXCPMessage msg)
	{
		name = msg.getVariableAsString(NXCPCodes.VID_NAME);
		category = msg.getVariableAsString(NXCPCodes.VID_CATEGORY);
		binaryData = msg.getVariableAsBinary(NXCPCodes.VID_IMAGE_DATA);
		imageProtected = msg.getVariableAsBoolean(NXCPCodes.VID_IMAGE_PROTECTED);
	}

	/**
	 * Fill NXCP message with object data
	 */
	public void fillMessage(final NXCPMessage msg)
	{
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
	 * @return is dirty?
	 */
	public boolean isDirty()
	{
		return dirty;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		return "LibraryImage [name=" + name + ", category=" + category + ", binaryData=" + (binaryData == null ? "null" : "not null")
				+ ", imageProtected=" + imageProtected + ", dirty=" + dirty + "]";
	}

}
