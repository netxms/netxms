/*******************************************************************************
 * Copyright (c) 2000, 2015 IBM Corporation and others.
 *
 * This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License 2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *     IBM Corporation - initial API and implementation
 *******************************************************************************/
package org.netxms.nxmc.base.widgets.events;

/**
 * This adapter class provides default implementations for the methods described by the <code>HyperlinkListener</code> interface.
 * <p>
 * Classes that wish to deal with <code>HyperlinkEvent</code> s can extend this class and override only the methods which they are
 * interested in.
 * </p>
 *
 * @see IHyperlinkListener
 * @see HyperlinkEvent
 */
public class HyperlinkAdapter implements IHyperlinkListener
{
   /**
    * Sent when the link is entered. The default behaviour is to do nothing.
    *
    * @param e the event
    */
   @Override
   public void linkEntered(HyperlinkEvent e)
   {
   }

   /**
    * Sent when the link is exited. The default behaviour is to do nothing.
    *
    * @param e the event
    */
   @Override
   public void linkExited(HyperlinkEvent e)
   {
   }

   /**
    * Sent when the link is activated. The default behaviour is to do nothing.
    *
    * @param e the event
    */
   @Override
   public void linkActivated(HyperlinkEvent e)
   {
   }
}
