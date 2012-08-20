/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

import org.eclipse.swt.events.PaintListener;

/**
 * The paint listener to paint on plot area.
 */
public interface ICustomPaintListener extends PaintListener {

    /**
     * Gets the state indicating if painting behind series.
     *
     * @return True if painting behind series
     */
    boolean drawBehindSeries();
}
