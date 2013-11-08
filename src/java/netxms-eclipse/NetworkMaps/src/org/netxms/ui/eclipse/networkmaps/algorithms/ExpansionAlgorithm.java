/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.algorithms;

import java.util.Arrays;
import java.util.Comparator;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import org.eclipse.gef4.zest.layouts.LayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.dataStructures.DisplayIndependentPoint;
import org.eclipse.gef4.zest.layouts.interfaces.EntityLayout;
import org.eclipse.gef4.zest.layouts.interfaces.LayoutContext;

/**
 * Graph expansion algorithm for overlay elimination
 */
public class ExpansionAlgorithm implements LayoutAlgorithm
{
	private LayoutContext context;

	/* (non-Javadoc)
	 * @see org.eclipse.gef4.zest.layouts.LayoutAlgorithm#setLayoutContext(org.eclipse.gef4.zest.layouts.interfaces.LayoutContext)
	 */
	@Override
	public void setLayoutContext(LayoutContext context)
	{
		this.context = context;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.gef4.zest.layouts.LayoutAlgorithm#applyLayout(boolean)
	 */
	@Override
	public void applyLayout(boolean clean)
	{
		if (!clean)
			return;
		
		int intersections = 0;
		double scaleFactorX = 1.0;
		double scaleFactorY = 1.0;
		
		// Sort elements by X coordinate
		EntityLayout[] entities = context.getEntities();
		Arrays.sort(entities, new Comparator<EntityLayout>() {
			@Override
			public int compare(EntityLayout e1, EntityLayout e2)
			{
				return (int)Math.signum(e1.getLocation().x - e2.getLocation().x);
			}
		});
		
		// Scan elements from left to right
		final Set<EntityLayout> currentElements = new HashSet<EntityLayout>();
		for(EntityLayout e : entities) 
		{
			DisplayIndependentPoint currLoc = e.getLocation();
			final double currX = currLoc.x - e.getSize().width / 2;	// current sweep line location
			
			// remove elements passed by sweep line
			// and check remaining for intersection with current element
			Iterator<EntityLayout> it = currentElements.iterator();
			while(it.hasNext())
			{
				EntityLayout l = it.next();
				if (l.getLocation().x + l.getSize().width / 2 < currX)
				{
					it.remove();
				}
				else
				{
					double distanceY = Math.abs(currLoc.y - l.getLocation().y);
					if (distanceY < 1.0)
						distanceY = 1.0;
					double overlayY = (e.getSize().height / 2 + l.getSize().height / 2) - distanceY;
					if (overlayY >= 0)
					{
						// elements intersects
						intersections++;

						double distanceX = Math.abs(currLoc.x - l.getLocation().x);
						double overlayX = (e.getSize().width / 2 + l.getSize().width / 2) - distanceX;
						
						//diffX += 5;	// add minimal distance between elements
						//diffY += 5;
						
						if (overlayX < overlayY)
						{
							double psf = (distanceX + overlayX) / distanceX;
							if (psf > scaleFactorX)
							{
								scaleFactorX = psf;
							}
						}
						else
						{
							double psf = (distanceY + overlayY) / distanceY;
							if (psf > scaleFactorY)
							{
								scaleFactorY = psf;
							}
						}
					}
				}
				if ((scaleFactorX >= 8.0) && (scaleFactorY >= 8.0))
					break;
			}
			
			if ((scaleFactorX >= 8.0) && (scaleFactorY >= 8.0))
				break;
			currentElements.add(e);
		}
		
		if (intersections > 0)
		{
			if (scaleFactorX > 8.0)
				scaleFactorX = 8.0;
			if (scaleFactorY > 8.0)
				scaleFactorY = 8.0;
			for(EntityLayout e : entities) 
			{
				DisplayIndependentPoint p = e.getLocation();
				e.setLocation(p.x * scaleFactorX, p.y * scaleFactorY);
			}
		}
	}
}
