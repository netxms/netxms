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

import java.util.Objects;
import java.util.function.Consumer;

/**
 * Classes that implement this interface will be notified when hyperlinks are entered, exited and activated.
 *
 * @see org.netxms.nxmc.base.widgets.Hyperlink
 * @see org.netxms.nxmc.base.widgets.ImageHyperlink
 * @see org.netxms.nxmc.base.widgets.FormText
 */
public interface IHyperlinkListener
{
   /**
    * Sent when hyperlink is entered either by mouse entering the link client area, or keyboard focus switching to the hyperlink.
    *
    * @param e an event containing information about the hyperlink
    */
   void linkEntered(HyperlinkEvent e);

   /**
    * Sent when hyperlink is exited either by mouse exiting the link client area, or keyboard focus switching from the hyperlink.
    *
    * @param e an event containing information about the hyperlink
    */
   void linkExited(HyperlinkEvent e);

   /**
    * Sent when hyperlink is activated either by mouse click inside the link client area, or by pressing 'Enter' key while hyperlink
    * has keyboard focus.
    *
    * @param e an event containing information about the hyperlink
    */
   void linkActivated(HyperlinkEvent e);

   /**
    * Static helper method to create a <code>IHyperlinkListener</code> for the {@link #linkEntered(HyperlinkEvent)} method, given a
    * lambda expression or a method reference.
    *
    * @param consumer the consumer of the event
    * @return IHyperlinkListener
    * @since 3.9
    */
   static IHyperlinkListener linkEnteredAdapter(Consumer<HyperlinkEvent> consumer)
   {
      Objects.requireNonNull(consumer);
      return new HyperlinkAdapter() {
         @Override
         public void linkEntered(HyperlinkEvent e)
         {
            consumer.accept(e);
         }
      };
   }

   /**
    * Static helper method to create a <code>IHyperlinkListener</code> for the {@link #linkExited(HyperlinkEvent)} method, given a
    * lambda expression or a method reference.
    *
    * @param consumer the consumer of the event
    * @return IHyperlinkListener
    * @since 3.9
    */
   static IHyperlinkListener linkExitedAdapter(Consumer<HyperlinkEvent> consumer)
   {
      Objects.requireNonNull(consumer);
      return new HyperlinkAdapter() {
         @Override
         public void linkExited(HyperlinkEvent e)
         {
            consumer.accept(e);
         }
      };
   }

   /**
    * Static helper method to create a <code>IHyperlinkListener</code> for the {@link #linkActivated(HyperlinkEvent)} method, given
    * a lambda expression or a method reference.
    *
    * @param consumer the consumer of the event
    * @return IHyperlinkListener
    * @since 3.9
    */
   static IHyperlinkListener linkActivatedAdapter(Consumer<HyperlinkEvent> consumer)
   {
      Objects.requireNonNull(consumer);
      return new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            consumer.accept(e);
         }
      };
   }
}
