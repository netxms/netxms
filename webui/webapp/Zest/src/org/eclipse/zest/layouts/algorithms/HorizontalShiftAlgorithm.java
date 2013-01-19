/*******************************************************************************
 * Copyright (c) 2005-2010 The Chisel Group and others. All rights reserved. This
 * program and the accompanying materials are made available under the terms of
 * the Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: The Chisel Group - initial API and implementation
 *               Mateusz Matela 
 *               Ian Bull
 ******************************************************************************/
package org.eclipse.zest.layouts.algorithms;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;

import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentDimension;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentRectangle;
import org.eclipse.zest.layouts.interfaces.EntityLayout;
import org.eclipse.zest.layouts.interfaces.LayoutContext;

/**
 * This layout shifts overlapping nodes to the right.
 * 
 * @author Ian Bull
 */
public class HorizontalShiftAlgorithm implements LayoutAlgorithm {

	private static final double DELTA = 10;

	private static final double VSPACING = 16;

	private LayoutContext context;

	public void applyLayout(boolean clean) {
		if (!clean)
			return;
		ArrayList rowsList = new ArrayList();
		EntityLayout[] entities = context.getEntities();

		for (int i = 0; i < entities.length; i++) {
			addToRowList(entities[i], rowsList);
		}

		Collections.sort(rowsList, new Comparator() {
			public int compare(Object o1, Object o2) {
				List a0 = (List) o1;
				List a1 = (List) o2;
				EntityLayout entity0 = (EntityLayout) a0.get(0);
				EntityLayout entity1 = (EntityLayout) a1.get(0);
				return (int) (entity0.getLocation().y - entity1.getLocation().y);
			}
		});

		Comparator entityComparator = new Comparator() {
			public int compare(Object o1, Object o2) {
				return (int) (((EntityLayout) o1).getLocation().y - ((EntityLayout) o2)
						.getLocation().y);
			}
		};
		DisplayIndependentRectangle bounds = context.getBounds();
		int heightSoFar = 0;

		for (Iterator iterator = rowsList.iterator(); iterator.hasNext();) {
			List currentRow = (List) iterator.next();
			Collections.sort(currentRow, entityComparator);

			int i = 0;
			int width = (int) (bounds.width / 2 - currentRow.size() * 75);

			heightSoFar += ((EntityLayout) currentRow.get(0)).getSize().height
					+ VSPACING;
			for (Iterator iterator2 = currentRow.iterator(); iterator2
					.hasNext();) {
				EntityLayout entity = (EntityLayout) iterator2.next();
				DisplayIndependentDimension size = entity.getSize();
				entity.setLocation(width + 10 * ++i + size.width / 2,
						heightSoFar + size.height / 2);
				width += size.width;
			}
		}
	}

	public void setLayoutContext(LayoutContext context) {
		this.context = context;
	}

	private void addToRowList(EntityLayout entity, ArrayList rowsList) {
		double layoutY = entity.getLocation().y;

		for (Iterator iterator = rowsList.iterator(); iterator.hasNext();) {
			List currentRow = (List) iterator.next();
			EntityLayout currentRowEntity = (EntityLayout) currentRow.get(0);
			double currentRowY = currentRowEntity.getLocation().y;
			if (layoutY >= currentRowY - DELTA
					&& layoutY <= currentRowY + DELTA) {
				currentRow.add(entity);
				return;
			}
		}
		List newRow = new ArrayList();
		newRow.add(entity);
		rowsList.add(newRow);
	}
}
