/**
 * 
 */
package org.netxms.ui.eclipse.osm.tools;

import org.eclipse.swt.graphics.Image;

/**
 * Tile object
 */
public class Tile
{
	int x;
	int y;
	private Image image;
	private boolean loaded;
	private boolean internalImage;
	
	/**
	 * @param image
	 * @param loaded
	 * @param x
	 * @param y
	 */
	public Tile(int x, int y, Image image, boolean loaded, boolean internalImage)
	{
		this.image = image;
		this.loaded = loaded;
		this.internalImage = internalImage;
		this.x = x;
		this.y = y;
	}

	/**
	 * Dispose tile
	 */
	public void dispose()
	{
		if ((image != null) && !internalImage)
			image.dispose();
	}

	/**
	 * @return the x
	 */
	public int getX()
	{
		return x;
	}

	/**
	 * @return the y
	 */
	public int getY()
	{
		return y;
	}

	/**
	 * @return the image
	 */
	public Image getImage()
	{
		return image;
	}

	/**
	 * @return the loaded
	 */
	public boolean isLoaded()
	{
		return loaded;
	}
}
