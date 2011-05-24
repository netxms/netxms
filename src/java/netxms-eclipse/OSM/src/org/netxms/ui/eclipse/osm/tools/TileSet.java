/**
 * 
 */
package org.netxms.ui.eclipse.osm.tools;

import org.eclipse.swt.graphics.Image;

/**
 * Tile set
 *
 */
public class TileSet
{
	public Image[][] tiles;
	public int xOffset;
	public int yOffset;
	
	/**
	 * @param tiles
	 * @param xOffset
	 * @param yOffset
	 */
	public TileSet(Image[][] tiles, int xOffset, int yOffset)
	{
		super();
		this.tiles = tiles;
		this.xOffset = xOffset;
		this.yOffset = yOffset;
	}
}
