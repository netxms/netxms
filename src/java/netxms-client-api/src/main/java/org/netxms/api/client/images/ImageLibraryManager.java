package org.netxms.api.client.images;

import java.io.IOException;
import java.util.List;
import org.netxms.api.client.NetXMSClientException;

public interface ImageLibraryManager
{
	public List<LibraryImage> getImageLibrary() throws IOException, NetXMSClientException;

	public List<LibraryImage> getImageLibrary(String category) throws IOException, NetXMSClientException;

	public LibraryImage getImage(String key) throws IOException, NetXMSClientException;

	public void createImage(LibraryImage image) throws IOException, NetXMSClientException;

	public void deleteImage(LibraryImage image) throws IOException, NetXMSClientException;

	public void modifyImage(LibraryImage image) throws IOException, NetXMSClientException;

}
