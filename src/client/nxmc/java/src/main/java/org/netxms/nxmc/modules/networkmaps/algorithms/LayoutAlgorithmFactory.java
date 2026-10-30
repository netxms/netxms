/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.algorithms;

import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.gef4.zest.layouts.LayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.ForceDirectedLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.RadialLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.TreeLayoutAlgorithm;
import org.netxms.client.maps.MapLayoutAlgorithm;

/**
 * Creates layout algorithm implementations for map layout settings. Dashboard map elements lay out sparse vertical tree themselves.
 */
public final class LayoutAlgorithmFactory
{
   /**
    * Create layout algorithm.
    *
    * @param alg map layout setting
    * @return layout algorithm
    */
   public static LayoutAlgorithm create(MapLayoutAlgorithm alg)
   {
      switch(alg)
      {
         case SPRING:
            return new ForceDirectedLayoutAlgorithm();
         case RADIAL:
            return new RadialLayoutAlgorithm();
         case HTREE:
            return new TreeLayoutAlgorithm(TreeLayoutAlgorithm.LEFT_RIGHT);
         case VTREE:
            return new TreeLayoutAlgorithm(TreeLayoutAlgorithm.TOP_DOWN);
         case SPARSE_VTREE:
            return new TreeLayoutAlgorithm(TreeLayoutAlgorithm.TOP_DOWN, new Dimension(100, 100));
         default:
            return new ManualLayout();
      }
   }
}
