/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

import org.eclipse.swt.widgets.Event;

/**
 * The dispose listener.
 */
public interface IDisposeListener {

    /**
     * The method to be invoked when the target is disposed.
     * 
     * @param e
     *            the event
     */
    void disposed(Event e);
}