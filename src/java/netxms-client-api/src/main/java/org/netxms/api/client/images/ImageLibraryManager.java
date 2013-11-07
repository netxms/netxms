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
