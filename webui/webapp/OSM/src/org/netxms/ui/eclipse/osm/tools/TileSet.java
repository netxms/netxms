/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.osm.tools;

/**
 * Tile set
 */
public class TileSet
{
	public Tile[][] tiles;
	public int xOffset;
	public int yOffset;
	public int zoom;
	public int missingTiles;
	
	/**
	 * @param tiles
	 * @param xOffset
	 * @param yOffset
	 */
	public TileSet(Tile[][] tiles, int xOffset, int yOffset, int zoom)
	{
		this.tiles = tiles;
		this.xOffset = xOffset;
		this.yOffset = yOffset;
		this.zoom = zoom;
		missingTiles = 0;
		for(int i = 0; i < tiles.length; i++)
			for(int j = 0; j < tiles[i].length; j++)
				if (!tiles[i][j].isLoaded())
					missingTiles++;
	}
	
	/**
	 * Dispose resources allocated for tile set
	 */
	public void dispose()
	{
		if (tiles != null)
		{
			for(int i = 0; i < tiles.length; i++)
				for(int j = 0; j < tiles[i].length; j++)
					tiles[i][j].dispose();
		}
	}
}
