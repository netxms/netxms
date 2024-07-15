/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.List;
import java.util.UUID;
import org.junit.jupiter.api.Test;
import org.netxms.client.LibraryImage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;

/**
 * Tests for image library
 */
public class ImageLibraryTest extends AbstractSessionTest
{
   @Test
	public void testGetLibrary() throws Exception
	{
		final NXCSession session = connectAndLogin();

		final List<LibraryImage> library = session.getImageLibrary();
		assertTrue(library.size() > 1);
		final LibraryImage image1 = library.get(0);
		//assertEquals("ATM", image1.getName());
		//assertEquals("1ddb76a3-a05f-4a42-acda-22021768feaf", image1.getGuid().toString());
      assertFalse(image1.isComplete());
		assertEquals(TestConstants.IMAGE_CATEGORY, image1.getCategory());

		for(LibraryImage i : library)
			System.out.println(i.toString());
		
		session.disconnect();
	}
   @Test
	public void testGetLibraryCategory() throws Exception
	{
		final NXCSession session = connectAndLogin();

		final List<LibraryImage> library = session.getImageLibrary("Network Objects");
		assertTrue(library.size() > 1);
		
		session.disconnect();
	}
   @Test
	public void testGetImage() throws Exception
	{
		final NXCSession session = connectAndLogin();

		final LibraryImage image = session.getImage(UUID.fromString("1ddb76a3-a05f-4a42-acda-22021768feaf")); // ATM
		assertEquals("1ddb76a3-a05f-4a42-acda-22021768feaf", image.getGuid().toString());
		assertEquals("ATM", image.getName());
		assertEquals("Network Objects", image.getCategory());
      assertTrue(image.isProtected());
      assertTrue(image.isComplete());
		assertEquals("image/png", image.getMimeType());
		assertEquals(860, image.getBinaryData().length);

		session.disconnect();
	}
   @Test
	public void testGetImageParallel() throws Exception
	{
		Runnable r = new Runnable() {
			@Override
			public void run()
			{
				try
				{
					final NXCSession session = connectAndLogin();
					System.out.println(Thread.currentThread().getName() + ": connected");
	
					final LibraryImage image = session.getImage(UUID.fromString("1ddb76a3-a05f-4a42-acda-22021768feaf")); // ATM
					assertEquals("1ddb76a3-a05f-4a42-acda-22021768feaf", image.getGuid().toString());
					assertEquals("ATM", image.getName());
					assertEquals("Network Objects", image.getCategory());
               assertTrue(image.isProtected());
               assertTrue(image.isComplete());
					assertEquals("image/png", image.getMimeType());
					assertEquals(2718, image.getBinaryData().length);
					System.out.println(Thread.currentThread().getName() + ": download " + image.getBinaryData().length);

					session.disconnect();

					System.out.println(Thread.currentThread().getName() + ": stopped");
				}
				catch(Exception e)
				{
					e.printStackTrace();
				}
			}
		};

		Thread[] t = new Thread[16];
		for(int i = 0; i < t.length; i++)
		{
			t[i] = new Thread(null, r, "Worker " + i);
			t[i].start();
		}
		for(int i = 0; i < t.length; i++)
			t[i].join();
	}

   @Test
	public void testDeleteStockImage() throws Exception
	{
		final NXCSession session = connectAndLogin();

		try
		{
         session.deleteImage(UUID.fromString("1ddb76a3-a05f-4a42-acda-22021768feaf"));
			assertTrue(false);
		}
		catch(NXCException e)
		{
         assertEquals(RCC.PROTECTED_IMAGE, e.getErrorCode());
		}

		final LibraryImage image = session.getImage(UUID.fromString("1ddb76a3-a05f-4a42-acda-22021768feaf")); // ATM
		assertEquals("1ddb76a3-a05f-4a42-acda-22021768feaf", image.getGuid().toString());
		assertEquals("ATM", image.getName());
		assertEquals("Network Objects", image.getCategory());
      assertTrue(image.isProtected());
      assertTrue(image.isComplete());
		assertEquals("image/png", image.getMimeType());
		assertEquals(860, image.getBinaryData().length);

		session.disconnect();
	}

   @Test
	public void testCreateImage() throws Exception
	{
		final NXCSession session = connectAndLogin();

		final LibraryImage image = new LibraryImage();
		image.setName("testCreateImage");
		image.setCategory("category");
      image.setBinaryData("data".getBytes(), "image/unknown");
		final LibraryImage createdImage = session.createImage(image, null);

		assertNotNull(createdImage.getGuid());
		System.out.println("Assigned GUID: " + createdImage.getGuid().toString());

		session.deleteImage(createdImage);
		
		session.disconnect();
	}

   @Test
	public void testModifyImage() throws Exception
	{
		final NXCSession session = connectAndLogin();

      final LibraryImage image = new LibraryImage(UUID.fromString("c263037f-021d-41b8-ac73-b5fd64ee3a85"), "testModifyImage", "category", "image/unknown");
      image.setBinaryData("new data".getBytes(), "image/unknown");
		session.modifyImage(image, null);

		session.deleteImage(image);
		
		session.disconnect();
	}
}
