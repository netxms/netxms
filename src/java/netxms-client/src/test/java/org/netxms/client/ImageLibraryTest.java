package org.netxms.client;

import java.util.List;
import org.netxms.api.client.images.LibraryImage;

public class ImageLibraryTest extends SessionTest
{
	public void testGetLibrary() throws Exception
	{
		final NXCSession session = connect();

		final List<LibraryImage> library = session.getImageLibrary();
		assertEquals(3, library.size());
		assertEquals("image1", library.get(0).getName());
		assertEquals(true, library.get(0).isDirty());
		assertEquals("image2", library.get(1).getName());
		assertEquals("image3", library.get(2).getName());
	}

	public void testGetLibraryCategory() throws Exception
	{
		final NXCSession session = connect();

		final List<LibraryImage> library = session.getImageLibrary("cat1");
		assertEquals(1, library.size());
		assertEquals("image1", library.get(0).getName());
		assertEquals(true, library.get(0).isDirty());
	}

	public void testGetImage() throws Exception
	{
		final NXCSession session = connect();

		final LibraryImage image = session.getImage("image1");
		assertEquals("image1", image.getName());
		assertEquals("cat1", image.getCategory());
		assertEquals(true, image.isProtected());
		assertEquals(153, image.getBinaryData().length);
	}

	public void testDeleteImage() throws Exception
	{
		final NXCSession session = connect();

		final LibraryImage image = new LibraryImage("image4");
		session.deleteImage(image);
	}

	public void testCreateImage() throws Exception
	{
		final NXCSession session = connect();

		final LibraryImage image = new LibraryImage("image.png");
		image.setCategory("category");
		image.setBinaryData("data".getBytes());
		session.createImage(image);
	}

	public void testModifyImage() throws Exception
	{
		final NXCSession session = connect();

		final LibraryImage image = new LibraryImage("image.png");
		image.setCategory("category");
		image.setBinaryData("new data".getBytes());
		session.modifyImage(image);
	}
}
