/*******************************************************************************
 * Copyright (c) 2004, 2015 IBM Corporation and others.
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
package org.netxms.nxmc.base.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.accessibility.ACC;
import org.eclipse.swt.accessibility.Accessible;
import org.eclipse.swt.accessibility.AccessibleAdapter;
import org.eclipse.swt.accessibility.AccessibleControlAdapter;
import org.eclipse.swt.accessibility.AccessibleControlEvent;
import org.eclipse.swt.accessibility.AccessibleEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Hyperlink is a concrete implementation of the abstract base class that draws text in the client area. Text can be wrapped and
 * underlined. Hyperlink is typically added to the hyperlink group so that certain properties are managed for all the hyperlinks
 * that belong to it.
 * <p>
 * Hyperlink can be extended.
 * </p>
 * <dl>
 * <dt><b>Styles:</b></dt>
 * <dd>SWT.WRAP</dd>
 * </dl>
 */
public class Hyperlink extends AbstractHyperlink
{
   private String text;
   private static final String ELLIPSIS = "..."; //$NON-NLS-1$
   private boolean underlined;
   // The tooltip is used for two purposes - the application can set
   // a tooltip or the tooltip can be used to display the full text when the
   // the text has been truncated due to the label being too short.
   // The appToolTip stores the tooltip set by the application. Control.tooltiptext
   // contains whatever tooltip is currently being displayed.
   private String appToolTipText;

   /**
    * Creates a new hyperlink control in the provided parent.
    *
    * @param parent the control parent
    * @param style the widget style
    */
   public Hyperlink(Composite parent, int style)
   {
      super(parent, style);
      initAccessible();
   }

   protected void initAccessible()
   {
      Accessible accessible = getAccessible();
      accessible.addAccessibleListener(new AccessibleAdapter() {
         @Override
         public void getName(AccessibleEvent e)
         {
            e.result = getText();
            if (e.result == null)
               getHelp(e);
         }

         @Override
         public void getHelp(AccessibleEvent e)
         {
            e.result = getToolTipText();
         }
      });
      accessible.addAccessibleControlListener(new AccessibleControlAdapter() {
         @Override
         public void getChildAtPoint(AccessibleControlEvent e)
         {
            Point pt = toControl(new Point(e.x, e.y));
            e.childID = (getBounds().contains(pt)) ? ACC.CHILDID_SELF : ACC.CHILDID_NONE;
         }

         @Override
         public void getLocation(AccessibleControlEvent e)
         {
            Rectangle location = getBounds();
            Point pt = toDisplay(new Point(location.x, location.y));
            e.x = pt.x;
            e.y = pt.y;
            e.width = location.width;
            e.height = location.height;
         }

         @Override
         public void getChildCount(AccessibleControlEvent e)
         {
            e.detail = 0;
         }

         @Override
         public void getRole(AccessibleControlEvent e)
         {
            e.detail = ACC.ROLE_LINK;
         }

         @Override
         public void getDefaultAction(AccessibleControlEvent e)
         {
            e.result = SWT.getMessage("SWT_Press"); //$NON-NLS-1$
         }

         @Override
         public void getState(AccessibleControlEvent e)
         {
            int state = ACC.STATE_NORMAL;
            if (Hyperlink.this.getSelection())
               state = ACC.STATE_SELECTED | ACC.STATE_FOCUSED;
            e.detail = state;
         }
      });
   }

   /**
    * Sets the underlined state. It is not necessary to call this method when in a hyperlink group.
    *
    * @param underlined if <samp>true </samp>, a line will be drawn below the text for each wrapped line.
    */
   public void setUnderlined(boolean underlined)
   {
      this.underlined = underlined;
      redraw();
   }

   /**
    * Returns the underline state of the hyperlink.
    *
    * @return <samp>true </samp> if text is underlined, <samp>false </samp> otherwise.
    */
   public boolean isUnderlined()
   {
      return underlined;
   }

   /**
    * Overrides the parent by incorporating the margin.
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      checkWidget();
      int innerWidth = wHint;
      if (innerWidth != SWT.DEFAULT)
      {
         innerWidth -= marginWidth * 2;
         if (innerWidth < 0)
            innerWidth = 0;
      }
      int innerHeight = hHint;
      if (innerHeight != SWT.DEFAULT)
      {
         innerHeight -= marginHeight * 2;
         if (innerHeight < 0)
            innerHeight = 0;
      }
      Point textSize = computeTextSize(innerWidth, innerHeight);
      int textWidth = textSize.x + 2 * marginWidth;
      int textHeight = textSize.y + 2 * marginHeight;
      if (wHint != SWT.DEFAULT)
      {
         textWidth = wHint;
      }
      if (hHint != SWT.DEFAULT)
      {
         textHeight = hHint;
      }
      Rectangle trim = computeTrim(0, 0, textWidth, textHeight);
      return new Point(trim.width, trim.height);
   }

   /**
    * Returns the current hyperlink text.
    *
    * @return hyperlink text
    */
   @Override
   public String getText()
   {
      return text;
   }

   @Override
   public String getToolTipText()
   {
      checkWidget();
      return appToolTipText;
   }

   @Override
   public void setToolTipText(String string)
   {
      super.setToolTipText(string);
      appToolTipText = super.getToolTipText();
   }

   /**
    * Sets the text of this hyperlink.
    *
    * @param text the hyperlink text
    */
   public void setText(String text)
   {
      if (text != null)
         this.text = text;
      else
         this.text = ""; //$NON-NLS-1$
      redraw();
   }

   /**
    * Paints the hyperlink text.
    *
    * @param gc graphic context
    */
   @Override
   protected void paintHyperlink(GC gc)
   {
      Rectangle carea = getClientArea();
      Rectangle bounds = new Rectangle(marginWidth, marginHeight, carea.width - marginWidth - marginWidth,
            carea.height - marginHeight - marginHeight);
      paintText(gc, bounds);
   }

   /**
    * Paints the hyperlink text in provided bounding rectangle.
    *
    * @param gc graphic context
    * @param bounds the bounding rectangle in which to paint the text
    */
   protected void paintText(GC gc, Rectangle bounds)
   {
      gc.setFont(getFont());
      Color fg = isEnabled() ? getForeground()
            : new Color(gc.getDevice(), ColorConverter.blend(getBackground().getRGB(), getForeground().getRGB(), 70));
      try
      {
         gc.setForeground(fg);
         if ((getStyle() & SWT.WRAP) != 0)
         {
            WidgetHelper.paintWrapText(gc, text, bounds, underlined);
         }
         else
         {
            Point totalSize = computeTextSize(SWT.DEFAULT, SWT.DEFAULT);
            boolean shortenText = false;
            if (bounds.width < totalSize.x)
            {
               // shorten
               shortenText = true;
            }
            int textWidth = Math.min(bounds.width, totalSize.x);
            int textHeight = totalSize.y;
            String textToDraw = getText();
            if (shortenText)
            {
               textToDraw = shortenText(gc, getText(), bounds.width);
               if (appToolTipText == null)
               {
                  super.setToolTipText(getText());
               }
            }
            else
            {
               super.setToolTipText(appToolTipText);
            }
            gc.drawText(textToDraw, bounds.x, bounds.y, true);
            if (underlined)
            {
               int descent = 2; // FIXME: what is correct value? gc.getFontMetrics().getDescent();
               int lineY = bounds.y + textHeight - descent + 1;
               gc.drawLine(bounds.x, lineY, bounds.x + textWidth, lineY);
            }
         }
      }
      finally
      {
         if (!isEnabled() && fg != null)
            fg.dispose();
      }
   }

   protected String shortenText(GC gc, String t, int width)
   {
      if (t == null)
         return null;
      int w = gc.textExtent(ELLIPSIS).x;
      if (width <= w)
         return t;
      int l = t.length();
      int max = l / 2;
      int min = 0;
      int mid = (max + min) / 2 - 1;
      if (mid <= 0)
         return t;
      while(min < mid && mid < max)
      {
         String s1 = t.substring(0, mid);
         String s2 = t.substring(l - mid, l);
         int l1 = gc.textExtent(s1).x;
         int l2 = gc.textExtent(s2).x;
         if (l1 + w + l2 > width)
         {
            max = mid;
            mid = (max + min) / 2;
         }
         else if (l1 + w + l2 < width)
         {
            min = mid;
            mid = (max + min) / 2;
         }
         else
         {
            min = max;
         }
      }
      if (mid == 0)
         return t;
      return t.substring(0, mid) + ELLIPSIS + t.substring(l - mid, l);
   }

   protected Point computeTextSize(int wHint, int hHint)
   {
      Point extent;
      GC gc = new GC(this);
      gc.setFont(getFont());
      if ((getStyle() & SWT.WRAP) != 0 && wHint != SWT.DEFAULT)
      {
         extent = WidgetHelper.computeWrapSize(gc, getText(), wHint);
      }
      else
      {
         extent = gc.textExtent(getText());
         if ((getStyle() & SWT.WRAP) == 0 && wHint != SWT.DEFAULT)
            extent.x = wHint;
      }
      gc.dispose();
      return extent;
   }
}
