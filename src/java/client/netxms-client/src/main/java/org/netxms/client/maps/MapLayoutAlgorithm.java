package org.netxms.client.maps;

import java.util.HashMap;
import java.util.Map;
import org.netxms.base.Logger;

public enum MapLayoutAlgorithm
{
	MANUAL(0x7FFF), SPRING(0), RADIAL(1), HTREE(2), VTREE(3), SPARSE_VTREE(4);

	private int value;
	private static Map<Integer, MapLayoutAlgorithm> lookupTable = new HashMap<Integer, MapLayoutAlgorithm>();

	static
	{
		for(MapLayoutAlgorithm element : MapLayoutAlgorithm.values())
		{
			lookupTable.put(element.value, element);
		}
	}

	private MapLayoutAlgorithm(int value)
	{
		this.value = value;
	}

	public int getValue()
	{
		return value;
	}

	public static MapLayoutAlgorithm getByValue(int value)
	{
		final MapLayoutAlgorithm layout = lookupTable.get(value);
		if (layout == null)
		{
			Logger.warning("LayoutAlgorithm", "Unknown layout alghoritm: " + value);
			return SPRING; // fallback
		}
		return layout;
	}
}
