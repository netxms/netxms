package org.netxms.client;

import java.util.List;
import org.netxms.api.client.images.LibraryImage;

public class ImageLibraryTest extends SessionTest
{
	public void testGetLibrary() throws Exception
	{
		final NXCSession session = connect();

		final List<LibraryImage> library = session.getImageLibrary();
		assertTrue(library.size() > 1);
		final LibraryImage image1 = library.get(0);
		assertEquals("ATM", image1.getName());
		assertEquals(false, image1.isComplete());
		assertEquals("Network Objects", image1.getCategory());
		assertEquals("1ddb76a3-a05f-4a42-acda-22021768feaf", image1.getGuid());
	}

	public void testGetLibraryCategory() throws Exception
	{
		final NXCSession session = connect();

		final List<LibraryImage> library = session.getImageLibrary("Network Objects");
		assertTrue(library.size() > 1);
	}

	public void testGetImage() throws Exception
	{
		final NXCSession session = connect();

		final LibraryImage image = session.getImage("1ddb76a3-a05f-4a42-acda-22021768feaf"); // ATM
		assertEquals("1ddb76a3-a05f-4a42-acda-22021768feaf", image.getGuid());
		assertEquals("ATM", image.getName());
		assertEquals("Network Objects", image.getCategory());
		assertEquals(true, image.isProtected());
		assertEquals(true, image.isComplete());
		assertEquals("image/png", image.getMimeType());
		assertEquals(2718, image.getBinaryData().length);
	}

	public void testDeleteStockImage() throws Exception
	{
		final NXCSession session = connect();

		final LibraryImage image1 = new LibraryImage("1ddb76a3-a05f-4a42-acda-22021768feaf");
		session.deleteImage(image1);

		final LibraryImage image = session.getImage("1ddb76a3-a05f-4a42-acda-22021768feaf"); // ATM
		assertEquals("1ddb76a3-a05f-4a42-acda-22021768feaf", image.getGuid());
		assertEquals("ATM", image.getName());
		assertEquals("Network Objects", image.getCategory());
		assertEquals(true, image.isProtected());
		assertEquals(true, image.isComplete());
		assertEquals("image/png", image.getMimeType());
		assertEquals(2718, image.getBinaryData().length);
	}

	public void testCreateImage() throws Exception
	{
		final NXCSession session = connect();

		final LibraryImage image = new LibraryImage();
		image.setName("image1");
		image.setCategory("category");
		image.setBinaryData("data".getBytes());
		final LibraryImage createdImage = session.createImage(image);

		assertNotNull(createdImage.getGuid());
		assertTrue(createdImage.getGuid().length() == 36);
		
		System.out.println(createdImage.getGuid());

		session.deleteImage(createdImage);
	}

	public void testModifyImage() throws Exception
	{
		final NXCSession session = connect();

		final LibraryImage image = new LibraryImage("c263037f-021d-41b8-ac73-b5fd64ee3a85");
		image.setName("image1");
		image.setCategory("category");
		image.setBinaryData("new data".getBytes());
		session.modifyImage(image);
	}
}
