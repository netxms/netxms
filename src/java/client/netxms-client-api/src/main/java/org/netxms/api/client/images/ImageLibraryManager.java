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
package org.netxms.api.client.images;

import java.io.IOException;
import java.util.List;
import java.util.UUID;

import org.netxms.api.client.NetXMSClientException;
import org.netxms.api.client.ProgressListener;

public interface ImageLibraryManager
{
	public List<LibraryImage> getImageLibrary() throws IOException, NetXMSClientException;

	public List<LibraryImage> getImageLibrary(String category) throws IOException, NetXMSClientException;

	public LibraryImage getImage(UUID guid) throws IOException, NetXMSClientException;

	public LibraryImage createImage(LibraryImage image, ProgressListener listener) throws IOException, NetXMSClientException;

	public void deleteImage(LibraryImage image) throws IOException, NetXMSClientException;

	public void modifyImage(LibraryImage image, ProgressListener listener) throws IOException, NetXMSClientException;
}
