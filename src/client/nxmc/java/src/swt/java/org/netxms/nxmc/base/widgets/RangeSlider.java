/*******************************************************************************
 * Copyright (c) 2011 Laurent CARON.
 * This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License 2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-2.0/
 * 
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors: Laurent CARON (laurent.caron@gmail.com) - initial API and
 * implementation Darrel Karisch - Fix bug #62
 *******************************************************************************/
package org.netxms.nxmc.base.widgets;

import java.text.Format;
import org.eclipse.swt.SWT;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Resource;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Widget;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Instances of this class provide a slider with two thumbs to control lower and upper integer values.
 * <p>
 * <dl>
 * <dt><b>Styles:</b></dt>
 * <dd>BORDER</dd>
 * <dd>HORIZONTAL</dd>
 * <dd>VERTICAL</dd>
 * <dd>CONTROL - Allow key and mouse manipulation to control both lower and upper value thumbs simultaneously</dd>
 * <dd>ON - Indicates that selection listeners are notified continuously during thumb drag events. Otherwise, notification occurs
 * only after the drag event terminates.</dd>
 * <dd>HIGH - Indicates high quality tick marks are generated dynamically to a factor of the pageIncrement or increment. Otherwise,
 * tick marks divide the scale evenly into ten parts.</dd>
 * <dd>SMOOTH - Indicates mouse manipulation of upper and lower values are computed smoothly from the exact mouse cursor position
 * disregarding the increment value. Otherwise, values are constrained to an incremental value.</dd>
 * <dt><b>Events:</b></dt>
 * <dd>Selection</dd>
 * </dl>
 * </p>
 * <p>
 * Note: Styles HORIZONTAL and VERTICAL are mutually exclusive.
 * </p>
 */
public class RangeSlider extends Canvas
{

   private static final byte NONE = 0;
   private static final byte UPPER = 1 << 0;
   private static final byte LOWER = 1 << 1;
   private static final byte BOTH = UPPER | LOWER;

   private int minimum;
   private int maximum;
   private int lowerValue;
   private int upperValue;
   private final Image slider, sliderHover, sliderDrag, sliderSelected;
   private final Image vSlider, vSliderHover, vSliderDrag, vSliderSelected;
   private int orientation;
   private int increment;
   private int pageIncrement;
   private byte selectedElement, priorSelectedElement;
   private boolean dragInProgress;
   private boolean upperHover, lowerHover;
   private int previousUpperValue, previousLowerValue;
   private int startDragUpperValue, startDragLowerValue;
   private Point startDragPoint;
   private boolean hasFocus;
   private final boolean isSmooth;
   private final boolean isFullSelection;
   private final boolean isHighQuality;
   private final boolean isOn;
   private int tickDivisions = 10, stride = 1;
   private float tickFactor;
   private Format toolTipFormatter;
   private String clientToolTipText;
   private Color selectionColor;

   /**
    * Constructs a new instance of this class given its parent and a style value describing its behavior and appearance.
    * <p>
    * The style value is either one of the style constants defined in class <code>SWT</code> which is applicable to instances of
    * this class, or must be built by <em>bitwise OR</em>'ing together (that is, using the <code>int</code> "|" operator) two or
    * more of those <code>SWT</code> style constants. The class description lists the style constants that are applicable to the
    * class. Style bits are also inherited from superclasses.
    * </p>
    *
    * @param parent a composite control which will be the parent of the new instance (cannot be null)
    * @param style the style of control to construct. Default style is HORIZONTAL
    *
    * @exception IllegalArgumentException
    *               <ul>
    *               <li>ERROR_NULL_ARGUMENT - if the parent is null</li>
    *               </ul>
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the parent</li>
    *               </ul>
    * @see SWT#HORIZONTAL
    * @see SWT#VERTICAL
    * @see Widget#getStyle
    *
    */
   public RangeSlider(final Composite parent, final int style)
   {
      super(parent, SWT.DOUBLE_BUFFERED | ((style & SWT.BORDER) == SWT.BORDER ? SWT.BORDER : SWT.NONE));
      minimum = lowerValue = 0;
      maximum = upperValue = 100;
      increment = 1;
      pageIncrement = 10;
      slider = ResourceManager.getImage("images/slider-normal.png");
      sliderHover = ResourceManager.getImage("images/slider-hover.png");
      sliderDrag = ResourceManager.getImage("images/slider-drag.png");
      sliderSelected = ResourceManager.getImage("images/slider-selected.png");

      vSlider = ResourceManager.getImage("images/h-slider-normal.png");
      vSliderHover = ResourceManager.getImage("images/h-slider-hover.png");
      vSliderDrag = ResourceManager.getImage("images/h-slider-drag.png");
      vSliderSelected = ResourceManager.getImage("images/h-slider-selected.png");

      if ((style & SWT.VERTICAL) == SWT.VERTICAL)
      {
         orientation = SWT.VERTICAL;
      }
      else
      {
         orientation = SWT.HORIZONTAL;
      }
      isSmooth = (style & SWT.SMOOTH) == SWT.SMOOTH;
      isFullSelection = (style & SWT.CONTROL) == SWT.CONTROL;
      isHighQuality = (style & SWT.HIGH) == SWT.HIGH;
      isOn = (style & SWT.ON) == SWT.ON;
      selectedElement = isFullSelection ? BOTH : LOWER;

      selectionColor = getDisplay().getSystemColor(SWT.COLOR_LIST_SELECTION);

      addListener(SWT.Dispose, event -> {
         safeDispose(slider);
         safeDispose(sliderHover);
         safeDispose(sliderDrag);
         safeDispose(sliderSelected);

         safeDispose(vSlider);
         safeDispose(vSliderHover);
         safeDispose(vSliderDrag);
         safeDispose(vSliderSelected);
      });
      addMouseListeners();
      addListener(SWT.Resize, event -> {
         if (isHighQuality)
         {
            setTickFactors();
         }
         else
         {
            tickFactor = ((orientation == SWT.HORIZONTAL ? getClientArea().width : getClientArea().height) - 20f) / tickDivisions;
         }
      });
      addListener(SWT.FocusIn, e -> {
         hasFocus = true;
         redraw();
      });
      addListener(SWT.FocusOut, e -> {
         hasFocus = false;
         redraw();
      });
      addListener(SWT.KeyDown, event -> {
         handleKeyDown(event);
      });
      addPaintListener(event -> {
         drawWidget(event);
      });
   }

   /**
    * Dispose resource.
    *
    * @param resource resource to dispose
    */
   private static void safeDispose(final Resource resource)
   {
      if (resource != null && !resource.isDisposed())
         resource.dispose();
   }

   /**
    * reset the tickDivisions, tickFactor, and stride values according to current size. if the current size is less than 40, no tick
    * marks are produced.
    */
   protected void setTickFactors()
   {
      final int breadth = orientation == SWT.HORIZONTAL ? getClientArea().width : getClientArea().height;
      if (breadth > 40)
      {
         setTickFactors(breadth - 20f);
      }
      else
      {
         tickDivisions = 0;
      }
   }

   /**
    * reset the tickDivisions, tickFactor, and stride values according to the input breadth. we generate at least ten divisions, but
    * fewer if the ticks are too compact. some factor of pageIncrement is the preferred tick division. if too few ticks are
    * generated from pageIncrement then increment is used. if too few ticks are generated from increment some smaller value is used
    * if style is SMOOTH.
    */
   protected void setTickFactors(float breadth)
   {
      final int fullRange = maximum - minimum;
      int division = pageIncrement;
      tickDivisions = fullRange / division;
      float limit = Math.max(10f, breadth / 20);
      while(tickDivisions < limit && division > 2 && division % 2 == 0)
      {
         division >>= 1;
         tickDivisions = fullRange / division;
      }
      if (tickDivisions < limit)
      {
         division = increment;
         tickDivisions = fullRange / division;
         while(tickDivisions < limit && division > 2 && division % 2 == 0)
         {
            division >>= 1;
            tickDivisions = fullRange / division;
         }
      }
      if (tickDivisions < limit && isSmooth)
      {
         division = 1;
         tickDivisions = fullRange;
      }
      stride = 1;
      if (tickDivisions > 0)
      {
         final int range = fullRange - fullRange % division;
         final float tickRange = breadth * range / fullRange;
         final float pixelSize = tickRange / range;
         limit = Math.max(10f, tickRange / 20);
         while(tickDivisions / stride > limit)
         {
            stride <<= 1;
         }
         tickFactor = pixelSize * range / tickDivisions;
         if (range != fullRange)
         {
            ++tickDivisions;
         }
      }
   }

   @Override
   public int getStyle()
   {
      return super.getStyle() | orientation | (isSmooth ? SWT.SMOOTH : SWT.NONE) | //
            (isOn ? SWT.ON : SWT.NONE) | //
            (isFullSelection ? SWT.CONTROL : SWT.NONE) | //
            (isHighQuality ? SWT.HIGH : SWT.NONE);
   }

   /**
    * Add the mouse listeners (mouse up, mouse down, mouse move, mouse wheel)
    */
   private void addMouseListeners()
   {
      addListener(SWT.MouseDown, event -> {
         handleMouseDown(event);
      });

      addListener(SWT.MouseUp, event -> {
         handleMouseUp(event);
      });

      addListener(SWT.MouseMove, event -> {
         handleMouseMove(event);
      });

      addListener(SWT.MouseWheel, event -> {
         handleMouseWheel(event);
      });

      addListener(SWT.MouseHover, event -> {
         handleMouseHover(event);
      });

      addListener(SWT.MouseDoubleClick, event -> {
         handleMouseDoubleClick(event);
      });
   }

   /**
    * Code executed when the mouse is down
    *
    * @param e event
    */
   private void handleMouseDown(final Event e)
   {
      selectKnobs(e);
      if (e.count == 1)
      {
         priorSelectedElement = selectedElement;
      }
      if (upperHover || lowerHover)
      {
         selectedElement = isFullSelection && lowerHover && upperHover ? BOTH : lowerHover ? LOWER : upperHover ? UPPER : selectedElement;
         dragInProgress = true;
         startDragLowerValue = previousLowerValue = lowerValue;
         startDragUpperValue = previousUpperValue = upperValue;
         startDragPoint = new Point(e.x, e.y);
      }
   }

   /**
    * Code executed when the mouse is up
    *
    * @param e event
    */
   private void handleMouseUp(final Event e)
   {
      if (dragInProgress)
      {
         startDragPoint = null;
         validateNewValues(e);
         dragInProgress = false;
         super.setToolTipText(clientToolTipText);
      }
   }

   /**
    * invoke selection listeners if either upper or lower value has changed. if listeners reject the change, restore the previous
    * values. redraw if either upper or lower value has changed.
    *
    * @param e event
    */
   private void validateNewValues(final Event e)
   {
      if (upperValue != previousUpperValue || lowerValue != previousLowerValue)
      {
         if (!fireSelectionListeners(e))
         {
            upperValue = previousUpperValue;
            lowerValue = previousLowerValue;
         }
         previousUpperValue = upperValue;
         previousLowerValue = lowerValue;
         redraw();
      }
   }

   /**
    * Fire selection listeners for this control.
    *
    * @param sourceEvent
    * @return
    */
   private boolean fireSelectionListeners(final Event sourceEvent)
   {
      Listener[] listeners = getListeners(SWT.Selection);
      for(final Listener listener : listeners)
      {
         final Event event = new Event();

         event.button = sourceEvent == null ? 1 : sourceEvent.button;
         event.display = getDisplay();
         event.item = null;
         event.widget = this;
         event.data = sourceEvent == null ? null : sourceEvent.data;
         event.time = sourceEvent == null ? 0 : sourceEvent.time;
         event.x = sourceEvent == null ? 0 : sourceEvent.x;
         event.y = sourceEvent == null ? 0 : sourceEvent.y;
         event.type = SWT.Selection;

         listener.handleEvent(event);
         if (!event.doit)
            return false;
      }
      return true;
   }

   /**
    * Code executed when the mouse pointer is moving
    *
    * @param e event
    */
   private void handleMouseMove(final Event e)
   {
      if (!dragInProgress)
      {
         final boolean wasUpper = upperHover;
         final boolean wasLower = lowerHover;
         selectKnobs(e);
         if (wasUpper != upperHover || wasLower != lowerHover)
         {
            redraw();
         }
      }
      else
      { // dragInProgress
         final int x = e.x, y = e.y;
         if (orientation == SWT.HORIZONTAL)
         {
            if (selectedElement == BOTH)
            {
               final int diff = (int)((startDragPoint.x - x) / computePixelSizeForHorizontalSlider()) + minimum;
               int newUpper = startDragUpperValue - diff;
               int newLower = startDragLowerValue - diff;
               if (newUpper > maximum)
               {
                  newUpper = maximum;
                  newLower = maximum - (startDragUpperValue - startDragLowerValue);
               }
               else if (newLower < minimum)
               {
                  newLower = minimum;
                  newUpper = minimum + startDragUpperValue - startDragLowerValue;
               }
               upperValue = newUpper;
               lowerValue = newLower;
               if (!isSmooth)
               {
                  lowerValue = (int)(Math.ceil(lowerValue / increment) * increment) - increment;
                  upperValue = (int)(Math.ceil(upperValue / increment) * increment) - increment;
               }
               handleToolTip(lowerValue, upperValue);
            }
            else if ((selectedElement & UPPER) != 0)
            {
               upperValue = (int)Math.round((x - 9d) / computePixelSizeForHorizontalSlider()) + minimum;
               if (!isSmooth)
               {
                  upperValue = (int)(Math.ceil(upperValue / increment) * increment) - increment;
               }
               checkUpperValue();
               handleToolTip(upperValue);
            }
            else
            {
               lowerValue = (int)Math.round((x - 9d) / computePixelSizeForHorizontalSlider()) + minimum;
               if (!isSmooth)
               {
                  lowerValue = (int)(Math.ceil(lowerValue / increment) * increment) - increment;
               }
               checkLowerValue();
               handleToolTip(lowerValue);
            }
         }
         else
         {
            if (selectedElement == BOTH)
            {
               final int diff = (int)((startDragPoint.y - y) / computePixelSizeForVerticalSlider()) + minimum;
               int newUpper = startDragUpperValue - diff;
               int newLower = startDragLowerValue - diff;
               if (newUpper > maximum)
               {
                  newUpper = maximum;
                  newLower = maximum - (startDragUpperValue - startDragLowerValue);
               }
               else if (newLower < minimum)
               {
                  newLower = minimum;
                  newUpper = minimum + startDragUpperValue - startDragLowerValue;
               }
               upperValue = newUpper;
               lowerValue = newLower;
               if (!isSmooth)
               {
                  lowerValue = (int)(Math.ceil(lowerValue / increment) * increment) - increment;
                  upperValue = (int)(Math.ceil(upperValue / increment) * increment) - increment;
               }
               handleToolTip(lowerValue, upperValue);
            }
            else if ((selectedElement & UPPER) != 0)
            {
               upperValue = (int)Math.round((y - 9d) / computePixelSizeForVerticalSlider()) + minimum;
               if (!isSmooth)
               {
                  upperValue = (int)(Math.ceil(upperValue / increment) * increment) - increment;
               }
               checkUpperValue();
               handleToolTip(upperValue);
            }
            else
            {
               lowerValue = (int)Math.round((y - 9d) / computePixelSizeForVerticalSlider()) + minimum;
               if (!isSmooth)
               {
                  lowerValue = (int)(Math.ceil(lowerValue / increment) * increment) - increment;
               }
               checkLowerValue();
               handleToolTip(lowerValue);
            }
         }
         if (isOn)
         {
            validateNewValues(e);
         }
         else
         {
            redraw();
         }
      }
   }

   /**
    * determine whether the input coordinate is within the scale bounds and between the current upper and lower values.
    *
    * @param x
    * @param y
    * @return
    */
   private boolean isBetweenKnobs(int x, int y)
   {
      return orientation == SWT.HORIZONTAL ? x < coordUpper.x && x > coordLower.x && y >= 9 && y <= 9 + getClientArea().height - 20 : //
            y < coordUpper.y && y > coordLower.y && x >= 9 && x <= 9 + getClientArea().width - 20;
   }

   /**
    * set the upperHover and lowerHover values according to the coordinates of the input event, the key modifier state, and whether
    * the style allows selection of both knobs.
    *
    * @param e
    */
   private void selectKnobs(final Event e)
   {
      if (coordLower == null)
      {
         return;
      }
      final Image img = orientation == SWT.HORIZONTAL ? slider : vSlider;
      final int x = e.x, y = e.y;
      lowerHover = x >= coordLower.x && x <= coordLower.x + img.getBounds().width && y >= coordLower.y && y <= coordLower.y + img.getBounds().height;
      upperHover = ((e.stateMask & (SWT.CTRL | SWT.SHIFT)) != 0 || !lowerHover) && //
            x >= coordUpper.x && x <= coordUpper.x + img.getBounds().width && //
            y >= coordUpper.y && y <= coordUpper.y + img.getBounds().height;
      lowerHover &= (e.stateMask & SWT.CTRL) != 0 || !upperHover;
      if (!lowerHover && !upperHover && isFullSelection && isBetweenKnobs(x, y))
      {
         lowerHover = upperHover = true;
      }
   }

   /**
    * if the input coordinate is within the scale bounds, return the corresponding scale value of the coordinate. otherwise return
    * -1.
    *
    * @param x x coordinate value
    * @param y y coordinate value
    * @return
    */
   private int getCursorValue(int x, int y)
   {
      int value = -1;
      final Rectangle clientArea = getClientArea();
      if (orientation == SWT.HORIZONTAL)
      {
         if (x < 9 + clientArea.width - 20 && x >= 9 && y >= 9 && y <= 9 + clientArea.height - 20)
         {
            value = (int)Math.round((x - 9d) / computePixelSizeForHorizontalSlider()) + minimum;
         }
      }
      else if (y < 9 + clientArea.height - 20 && y >= 9 && x >= 9 && x <= 9 + clientArea.width - 20)
      {
         value = (int)Math.round((y - 9d) / computePixelSizeForVerticalSlider()) + minimum;
      }
      return value;
   }

   /**
    * Code executed when the mouse double click
    *
    * @param e event
    */
   private void handleMouseDoubleClick(final Event e)
   {
      final int value = getCursorValue(e.x, e.y);
      if (value >= 0)
      {
         selectedElement = priorSelectedElement;
         if (value > upperValue)
         {
            if (selectedElement == BOTH)
            {
               lowerValue += value - upperValue;
               upperValue = value;
            }
            else if ((selectedElement & UPPER) != 0)
            {
               upperValue = value;
            }
            else if ((selectedElement & LOWER) != 0)
            {
               final int diff = upperValue - lowerValue;
               if (value + diff > maximum)
               {
                  upperValue = maximum;
                  lowerValue = maximum - diff;
               }
               else
               {
                  upperValue = value + diff;
                  lowerValue = value;
               }
            }
         }
         else if (value < lowerValue)
         {
            if (selectedElement == BOTH)
            {
               upperValue += value - lowerValue;
               lowerValue = value;
            }
            else if ((selectedElement & LOWER) != 0)
            {
               lowerValue = value;
            }
            else if ((selectedElement & UPPER) != 0)
            {
               final int diff = upperValue - lowerValue;
               if (value - diff < minimum)
               {
                  lowerValue = minimum;
                  upperValue = minimum + diff;
               }
               else
               {
                  upperValue = value;
                  lowerValue = value - diff;
               }
            }
         }
         else if (value > lowerValue && value < upperValue && selectedElement != BOTH)
         {
            if ((selectedElement & LOWER) != 0)
            {
               lowerValue = value;
            }
            else if ((selectedElement & UPPER) != 0)
            {
               upperValue = value;
            }
         }
         validateNewValues(e);
      }
   }

   private StringBuffer toolTip;
   private Point coordUpper;
   private Point coordLower;

   /**
    * set the tooltip if a toolTipFormatter is present. either one or two values are accepted.
    *
    * @param values
    */
   private void handleToolTip(int... values)
   {
      if (toolTipFormatter != null)
      {
         try
         {
            if (values.length == 1)
            {
               toolTip.setLength(0);
               toolTipFormatter.format(values[0], toolTip, null);
               super.setToolTipText(toolTip.toString());
            }
            else if (values.length == 2)
            {
               toolTip.setLength(0);
               toolTipFormatter.format(values[0], toolTip, null);
               toolTip.append(" \u2194 "); // LEFT RIGHT ARROW
               toolTipFormatter.format(values[1], toolTip, null);
               super.setToolTipText(toolTip.toString());
            }
         }
         catch(final IllegalArgumentException ex)
         {
            super.setToolTipText(clientToolTipText);
         }
      }
   }

   /**
    * Code executed on mouse hover
    *
    * @param e event
    */
   private void handleMouseHover(final Event e)
   {
      if (!dragInProgress && toolTipFormatter != null)
      {
         final int value = getCursorValue(e.x, e.y);
         if (value >= 0)
         {
            try
            {
               toolTip.setLength(0);
               toolTipFormatter.format(value, toolTip, null);
               super.setToolTipText(toolTip.toString());
            }
            catch(final IllegalArgumentException ex)
            {
               super.setToolTipText(clientToolTipText);
            }
         }
         else
         {
            super.setToolTipText(clientToolTipText);
         }
      }
   }

   /**
    * a formatter for displaying a tool tip when hovering over the scale and during thumb modification events. The
    * {@link Format#format(Object, StringBuffer, java.text.FieldPosition)} method is invoked to retrieve the text for the tooltip
    * where the input {@code Object} is an {@code Integer} with a value within the minimum and maximum.
    *
    * @param formatter
    */
   public void setToolTipFormatter(Format formatter)
   {
      toolTip = formatter != null ? new StringBuffer() : null;
      toolTipFormatter = formatter;
   }

   @Override
   public void setToolTipText(String string)
   {
      super.setToolTipText(clientToolTipText = string);
   }

   /**
    * Code executed when the mouse wheel is activated
    *
    * @param e event
    */
   private void handleMouseWheel(final Event e)
   {
      if (selectedElement == NONE || dragInProgress)
      {
         e.doit = false; // we are consuming this event
         return;
      }
      previousLowerValue = lowerValue;
      previousUpperValue = upperValue;
      final int amount = increment * ((e.stateMask & SWT.SHIFT) != 0 ? 10 : (e.stateMask & SWT.CTRL) != 0 ? 2 : 1);
      if (selectedElement == BOTH)
      {
         int newLower = lowerValue + e.count * amount;
         int newUpper = upperValue + e.count * amount;
         if (newUpper > maximum)
         {
            newUpper = maximum;
            newLower = maximum - (upperValue - lowerValue);
         }
         else if (newLower < minimum)
         {
            newLower = minimum;
            newUpper = minimum + upperValue - lowerValue;
         }
         upperValue = newUpper;
         lowerValue = newLower;
      }
      else if ((selectedElement & LOWER) != 0)
      {
         lowerValue += e.count * amount;
         checkLowerValue();
      }
      else
      {
         upperValue += e.count * amount;
         checkUpperValue();
      }
      validateNewValues(e);
      e.doit = false; // we are consuming this event
   }

   /**
    * Check if the lower value is in ranges
    */
   private void checkLowerValue()
   {
      if (lowerValue < minimum)
      {
         lowerValue = minimum;
      }
      if (lowerValue > maximum)
      {
         lowerValue = maximum;
      }
      if (lowerValue > upperValue)
      {
         lowerValue = upperValue;
      }
   }

   /**
    * Check if the upper value is in ranges
    */
   private void checkUpperValue()
   {
      if (upperValue < minimum)
      {
         upperValue = minimum;
      }
      if (upperValue > maximum)
      {
         upperValue = maximum;
      }
      if (upperValue < lowerValue)
      {
         upperValue = lowerValue;
      }
   }

   /**
    * Draws the widget
    *
    * @param e paint event
    */
   private void drawWidget(final PaintEvent e)
   {
      final Rectangle rect = getClientArea();
      if (rect.width == 0 || rect.height == 0)
      {
         return;
      }
      e.gc.setAdvanced(true);
      e.gc.setAntialias(SWT.ON);
      if (orientation == SWT.HORIZONTAL)
      {
         drawHorizontalRangeSlider(e.gc);
      }
      else
      {
         drawVerticalRangeSlider(e.gc);
      }

   }

   /**
    * Draw the range slider (horizontal)
    *
    * @param gc graphic context
    */
   private void drawHorizontalRangeSlider(final GC gc)
   {
      drawBackgroundHorizontal(gc);
      drawBarsHorizontal(gc);
      if (lowerHover || (selectedElement & LOWER) != 0)
      {
         coordUpper = drawHorizontalKnob(gc, upperValue, true);
         coordLower = drawHorizontalKnob(gc, lowerValue, false);
      }
      else
      {
         coordLower = drawHorizontalKnob(gc, lowerValue, false);
         coordUpper = drawHorizontalKnob(gc, upperValue, true);
      }
   }

   /**
    * Draw the background
    *
    * @param gc graphic context
    */
   private void drawBackgroundHorizontal(final GC gc)
   {
      final Rectangle clientArea = getClientArea();

      gc.setBackground(getBackground());
      gc.fillRectangle(clientArea);

      final float pixelSize = computePixelSizeForHorizontalSlider();
      final int startX = (int)(pixelSize * lowerValue);
      final int endX = (int)(pixelSize * upperValue);
      if (isEnabled())
      {
         gc.setBackground(selectionColor);
      }
      else
      {
         gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_GRAY));
      }
      gc.fillRectangle(12 + startX, 9, endX - startX - 6, clientArea.height - 20);

      if (isEnabled())
      {
         gc.setForeground(getForeground());
      }
      else
      {
         gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_GRAY));
      }
      gc.drawRoundRectangle(9, 9, clientArea.width - 20, clientArea.height - 20, 3, 3);
   }

   /**
    * @return how many pixels corresponds to 1 point of value
    */
   private float computePixelSizeForHorizontalSlider()
   {
      return (getClientArea().width - 20f) / (maximum - minimum);
   }

   /**
    * Draw the bars
    *
    * @param gc graphic context
    */
   private void drawBarsHorizontal(final GC gc)
   {
      final Rectangle clientArea = getClientArea();
      if (isEnabled())
      {
         gc.setForeground(getForeground());
      }
      else
      {
         gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_GRAY));
      }
      for(int i = stride; i < tickDivisions; i += stride)
      {
         final int x = (int)(9 + tickFactor * i);
         gc.drawLine(x, 4, x, 7);
         gc.drawLine(x, clientArea.height - 6, x, clientArea.height - 9);
      }
   }

   /**
    * Draws an horizontal knob
    *
    * @param gc graphic context
    * @param value corresponding value
    * @param upper if <code>true</code>, draws the upper knob. If <code>false</code>, draws the lower knob
    * @return the coordinate of the upper left corner of the knob
    */
   private Point drawHorizontalKnob(final GC gc, final int value, final boolean upper)
   {
      final float pixelSize = computePixelSizeForHorizontalSlider();
      final int x = (int)(pixelSize * value);
      Image image;
      if (upper)
      {
         if (upperHover)
         {
            image = dragInProgress || (selectedElement & UPPER) != 0 ? sliderDrag : sliderHover;
         }
         else if ((selectedElement & UPPER) != 0 && !lowerHover)
         {
            image = hasFocus ? sliderSelected : sliderHover;
         }
         else
         {
            image = slider;
         }
      }
      else
      {
         if (lowerHover)
         {
            image = dragInProgress || (selectedElement & LOWER) != 0 ? sliderDrag : sliderHover;
         }
         else if ((selectedElement & LOWER) != 0 && !upperHover)
         {
            image = hasFocus ? sliderSelected : sliderHover;
         }
         else
         {
            image = slider;
         }
      }
      if (isEnabled())
      {
         gc.drawImage(image, x + 5, getClientArea().height / 2 - slider.getBounds().height / 2);
      }
      else
      {
         final Image temp = new Image(getDisplay(), image, SWT.IMAGE_DISABLE);
         gc.drawImage(temp, x + 5, getClientArea().height / 2 - slider.getBounds().height / 2);
         temp.dispose();
      }
      return new Point(x + 5, getClientArea().height / 2 - slider.getBounds().height / 2);
   }

   /**
    * Draw the range slider (vertical)
    *
    * @param gc graphic context
    */
   private void drawVerticalRangeSlider(final GC gc)
   {
      drawBackgroundVertical(gc);
      drawBarsVertical(gc);
      if (lowerHover || (selectedElement & LOWER) != 0)
      {
         coordUpper = drawVerticalKnob(gc, upperValue, true);
         coordLower = drawVerticalKnob(gc, lowerValue, false);
      }
      else
      {
         coordLower = drawVerticalKnob(gc, lowerValue, false);
         coordUpper = drawVerticalKnob(gc, upperValue, true);
      }
   }

   /**
    * Draws the background
    *
    * @param gc graphic context
    */
   private void drawBackgroundVertical(final GC gc)
   {
      final Rectangle clientArea = getClientArea();
      gc.setBackground(getBackground());
      gc.fillRectangle(clientArea);

      final float pixelSize = computePixelSizeForVerticalSlider();
      final int startY = (int)(pixelSize * lowerValue);
      final int endY = (int)(pixelSize * upperValue);
      if (isEnabled())
      {
         gc.setBackground(selectionColor);
      }
      else
      {
         gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_GRAY));
      }
      gc.fillRectangle(9, 12 + startY, clientArea.width - 20, endY - startY - 6);

      if (isEnabled())
      {
         gc.setForeground(getForeground());
      }
      else
      {
         gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_GRAY));
      }
      gc.drawRoundRectangle(9, 9, clientArea.width - 20, clientArea.height - 20, 3, 3);
   }

   /**
    * @return how many pixels corresponds to 1 point of value
    */
   private float computePixelSizeForVerticalSlider()
   {
      return (getClientArea().height - 20f) / (maximum - minimum);
   }

   /**
    * Draws the bars
    *
    * @param gc graphic context
    */
   private void drawBarsVertical(final GC gc)
   {
      final Rectangle clientArea = getClientArea();
      if (isEnabled())
      {
         gc.setForeground(getForeground());
      }
      else
      {
         gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_GRAY));
      }
      for(int i = stride; i < tickDivisions; i += stride)
      {
         final int y = (int)(9 + tickFactor * i);
         gc.drawLine(4, y, 7, y);
         gc.drawLine(clientArea.width - 6, y, clientArea.width - 9, y);
      }
   }

   /**
    * Draws a vertical knob
    *
    * @param gc graphic context
    * @param value corresponding value
    * @param upper if <code>true</code>, draws the upper knob. If <code>false</code>, draws the lower knob
    * @return the coordinate of the upper left corner of the knob
    */
   private Point drawVerticalKnob(final GC gc, final int value, final boolean upper)
   {
      final float pixelSize = computePixelSizeForVerticalSlider();
      final int y = (int)(pixelSize * value);

      Image image;
      if (upper)
      {
         if (upperHover)
         {
            image = dragInProgress || (selectedElement & UPPER) != 0 ? vSliderDrag : vSliderHover;
         }
         else if ((selectedElement & UPPER) != 0 && !lowerHover)
         {
            image = hasFocus ? vSliderSelected : vSliderHover;
         }
         else
         {
            image = vSlider;
         }
      }
      else
      {
         if (lowerHover)
         {
            image = dragInProgress || (selectedElement & LOWER) != 0 ? vSliderDrag : vSliderHover;
         }
         else if ((selectedElement & LOWER) != 0 && !upperHover)
         {
            image = hasFocus ? vSliderSelected : vSliderHover;
         }
         else
         {
            image = vSlider;
         }
      }

      if (isEnabled())
      {
         gc.drawImage(image, getClientArea().width / 2 - 8, y + 4);
      }
      else
      {
         final Image temp = new Image(getDisplay(), image, SWT.IMAGE_DISABLE);
         gc.drawImage(temp, getClientArea().width / 2 - 8, y + 4);
         temp.dispose();
      }
      return new Point(getClientArea().width / 2 - 8, y + 4);
   }

   /**
    * move the cursor location by the input delta values.
    *
    * @param xDelta
    * @param yDelta
    */
   private void moveCursorPosition(int xDelta, int yDelta)
   {
      final Point cursorPosition = getDisplay().getCursorLocation();
      cursorPosition.x += xDelta;
      cursorPosition.y += yDelta;
      getDisplay().setCursorLocation(cursorPosition);
   }

   /**
    * Code executed when a key is typed
    *
    * @param event event
    */
   private void handleKeyDown(final Event event)
   {
      // TODO consider API for setting accelerator values
      int accelerator = (event.stateMask & SWT.SHIFT) != 0 ? 10 : (event.stateMask & SWT.CTRL) != 0 ? 2 : 1;
      if (dragInProgress)
      {
         switch(event.keyCode)
         {
            case SWT.ESC:
               startDragPoint = null;
               upperValue = startDragUpperValue;
               lowerValue = startDragLowerValue;
               validateNewValues(event);
               dragInProgress = false;
               if (!isOn)
               {
                  redraw();
               }
               event.doit = false;
               break;
            case SWT.ARROW_UP:
               accelerator = orientation == SWT.HORIZONTAL ? -accelerator : accelerator;
            case SWT.ARROW_LEFT:
               if (orientation == SWT.VERTICAL)
               {
                  moveCursorPosition(0, -accelerator);
               }
               else
               {
                  moveCursorPosition(-accelerator, 0);
               }
               event.doit = false;
               break;
            case SWT.ARROW_DOWN:
               accelerator = orientation == SWT.HORIZONTAL ? -accelerator : accelerator;
            case SWT.ARROW_RIGHT:
               if (orientation == SWT.VERTICAL)
               {
                  moveCursorPosition(0, accelerator);
               }
               else
               {
                  moveCursorPosition(accelerator, 0);
               }
               event.doit = false;
               break;
         }
         return;
      }
      previousLowerValue = lowerValue;
      previousUpperValue = upperValue;

      if (selectedElement == NONE)
      {
         selectedElement = LOWER;
      }
      switch(event.keyCode)
      {
         case SWT.HOME:
            if (selectedElement == BOTH)
            {
               if ((event.stateMask & SWT.SHIFT) != 0)
               {
                  lowerValue = maximum - (upperValue - lowerValue);
                  upperValue = maximum;
               }
               else
               {
                  upperValue = minimum + upperValue - lowerValue;
                  lowerValue = minimum;
               }
            }
            else if ((selectedElement & UPPER) != 0)
            {
               upperValue = maximum;
            }
            else
            {
               lowerValue = minimum;
            }
            break;
         case SWT.END:
            if (selectedElement == BOTH)
            {
               if ((event.stateMask & SWT.SHIFT) != 0)
               {
                  upperValue = minimum + upperValue - lowerValue;
                  lowerValue = minimum;
               }
               else
               {
                  lowerValue = maximum - (upperValue - lowerValue);
                  upperValue = maximum;
               }
            }
            else if ((selectedElement & UPPER) != 0)
            {
               upperValue = lowerValue;
            }
            else
            {
               lowerValue = upperValue;
            }
            break;
         case SWT.PAGE_UP:
            accelerator = orientation == SWT.HORIZONTAL ? -accelerator : accelerator;
            if (selectedElement == BOTH)
            {
               translateValues(pageIncrement * -accelerator);
            }
            else if ((selectedElement & UPPER) != 0)
            {
               upperValue -= pageIncrement * accelerator;
            }
            else
            {
               lowerValue -= pageIncrement * accelerator;
            }
            break;
         case SWT.PAGE_DOWN:
            accelerator = orientation == SWT.HORIZONTAL ? -accelerator : accelerator;
            if (selectedElement == BOTH)
            {
               translateValues(pageIncrement * accelerator);
            }
            else if ((selectedElement & UPPER) != 0)
            {
               upperValue += pageIncrement * accelerator;
            }
            else
            {
               lowerValue += pageIncrement * accelerator;
            }
            break;
         case SWT.ARROW_DOWN:
            accelerator = orientation == SWT.HORIZONTAL ? -accelerator : accelerator;
         case SWT.ARROW_RIGHT:
            if (selectedElement == BOTH)
            {
               translateValues(accelerator * increment);
            }
            else if ((selectedElement & UPPER) != 0)
            {
               upperValue += accelerator * increment;
            }
            else
            {
               lowerValue += accelerator * increment;
            }
            break;
         case SWT.ARROW_UP:
            accelerator = orientation == SWT.HORIZONTAL ? -accelerator : accelerator;
         case SWT.ARROW_LEFT:
            if (selectedElement == BOTH)
            {
               translateValues(-accelerator * increment);
            }
            else if ((selectedElement & UPPER) != 0)
            {
               upperValue -= accelerator * increment;
            }
            else
            {
               lowerValue -= accelerator * increment;
            }
            break;
         case SWT.TAB:
            final boolean next = (event.stateMask & SWT.SHIFT) == 0;
            if (next && (selectedElement & LOWER) != 0)
            {
               selectedElement = isFullSelection && selectedElement == LOWER ? BOTH : UPPER;
               redraw();
            }
            else if (!next && (selectedElement & UPPER) != 0)
            {
               selectedElement = isFullSelection && selectedElement == UPPER ? BOTH : LOWER;
               redraw();
            }
            else
            {
               traverse(next ? SWT.TRAVERSE_TAB_NEXT : SWT.TRAVERSE_TAB_PREVIOUS);
            }
            return;
      }
      if (previousLowerValue != lowerValue || previousUpperValue != upperValue)
      {
         if (selectedElement == BOTH)
         {
            checkLowerValue();
            checkUpperValue();
         }
         else if ((selectedElement & UPPER) != 0)
         {
            checkUpperValue();
         }
         else
         {
            checkLowerValue();
         }
         validateNewValues(event);
      }
   }

   /**
    * translate both the upper and lower values by the input amount. The updated values are constrained to be within the minimum and
    * maximum. The difference between upper and lower values is retained.
    *
    * @param amount
    */
   private void translateValues(int amount)
   {
      int newLower = lowerValue + amount;
      int newUpper = upperValue + amount;
      if (newUpper > maximum)
      {
         newUpper = maximum;
         newLower = maximum - (upperValue - lowerValue);
      }
      else if (newLower < minimum)
      {
         newLower = minimum;
         newUpper = minimum + upperValue - lowerValue;
      }
      upperValue = newUpper;
      lowerValue = newLower;
   }

   /**
    * Adds the listener to the collection of listeners who will be notified when the user changes the receiver's value, by sending
    * it one of the messages defined in the <code>SelectionListener</code> interface.
    * <p>
    * <code>widgetSelected</code> is called when the user changes the receiver's value. <code>widgetDefaultSelected</code> is not
    * called.
    * </p>
    *
    * @param listener the listener which should be notified
    *
    * @exception IllegalArgumentException
    *               <ul>
    *               <li>ERROR_NULL_ARGUMENT - if the listener is null</li>
    *               </ul>
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    *
    * @see SelectionListener
    * @see #removeSelectionListener
    */
   public void addSelectionListener(final SelectionListener listener)
   {
      addTypedListener(listener, SWT.Selection);
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(final int wHint, final int hHint, final boolean changed)
   {
      final int width, height;
      checkWidget();
      if (orientation == SWT.HORIZONTAL)
      {
         if (wHint < 100)
         {
            width = 100;
         }
         else
         {
            width = wHint;
         }

         if (hHint < 30)
         {
            height = 30;
         }
         else
         {
            height = hHint;
         }
      }
      else
      {
         if (wHint < 30)
         {
            width = 30;
         }
         else
         {
            width = wHint;
         }

         if (hHint < 100)
         {
            height = 100;
         }
         else
         {
            height = hHint;
         }
      }

      return new Point(width, height);
   }

   /**
    * Returns the amount that the selected receiver's value will be modified by when the up/down (or right/left) arrows are pressed.
    *
    * @return the increment
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public int getIncrement()
   {
      checkWidget();
      return increment;
   }

   /**
    * Returns the 'lower selection', which is the lower receiver's position.
    *
    * @return the selection
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public int getLowerValue()
   {
      checkWidget();
      return lowerValue;
   }

   /**
    * Returns the maximum value which the receiver will allow.
    *
    * @return the maximum
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public int getMaximum()
   {
      checkWidget();
      return maximum;
   }

   /**
    * Returns the minimum value which the receiver will allow.
    *
    * @return the minimum
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public int getMinimum()
   {
      checkWidget();
      return minimum;
   }

   /**
    * Returns the amount that the selected receiver's value will be modified by when the page increment/decrement areas are
    * selected.
    *
    * @return the page increment
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public int getPageIncrement()
   {
      checkWidget();
      return pageIncrement;
   }

   /**
    * Returns the 'selection', which is an array where the first element is the lower selection, and the second element is the upper
    * selection
    *
    * @return the selection
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public int[] getSelection()
   {
      checkWidget();
      final int[] selection = new int[2];
      selection[0] = lowerValue;
      selection[1] = upperValue;
      return selection;
   }

   /**
    * Returns the 'upper selection', which is the upper receiver's position.
    *
    * @return the selection
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public int getUpperValue()
   {
      checkWidget();
      return upperValue;
   }

   /**
    * Removes the listener from the collection of listeners who will be notified when the user changes the receiver's value.
    *
    * @param listener the listener which should no longer be notified
    *
    * @exception IllegalArgumentException
    *               <ul>
    *               <li>ERROR_NULL_ARGUMENT - if the listener is null</li>
    *               </ul>
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    *
    * @see SelectionListener
    * @see #addSelectionListener
    */
   public void removeSelectionListener(final SelectionListener listener)
   {
      removeTypedListener(SWT.Selection, listener);
   }

   /**
    * Sets the amount that the selected receiver's value will be modified by when the up/down (or right/left) arrows are pressed to
    * the argument, which must be at least one.
    *
    * @param increment the new increment (must be greater than zero)
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public void setIncrement(final int increment)
   {
      checkWidget();
      this.increment = increment;
      if (isHighQuality)
      {
         setTickFactors();
      }
      redraw();
   }

   /**
    * Sets the 'lower selection', which is the receiver's lower value, to the input argument which must be less than or equal to the
    * current 'upper selection' and greater or equal to the minimum. If either condition fails, no action is taken.
    *
    * @param value the new lower selection
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    * @see #getUpperValue()
    * @see #getMinimum()
    * @see #setSelection(int, int)
    */
   public void setLowerValue(final int value)
   {
      setSelection(value, upperValue);
   }

   /**
    * Sets the maximum value that the receiver will allow. This new value will be ignored if it is not greater than the receiver's
    * current minimum value. If the new maximum is applied then the receiver's selection value will be adjusted if necessary to fall
    * within its new range.
    *
    * @param value the new maximum, which must be greater than the current minimum
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    * @see #setExtrema(int, int)
    */
   public void setMaximum(final int value)
   {
      setExtrema(minimum, value);
   }

   /**
    * Sets the minimum value that the receiver will allow. This new value will be ignored if it is negative or is not less than the
    * receiver's current maximum value. If the new minimum is applied then the receiver's selection value will be adjusted if
    * necessary to fall within its new range.
    *
    * @param value the new minimum, which must be nonnegative and less than the current maximum
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    * @see #setExtrema(int, int)
    */
   public void setMinimum(final int value)
   {
      setExtrema(value, maximum);
   }

   /**
    * Sets the minimum and maximum values that the receiver will allow. The new values will be ignored if either are negative or the
    * min value is not less than the max. The receiver's selection values will be adjusted if necessary to fall within the new
    * range.
    *
    * @param min the new minimum, which must be nonnegative and less than the max
    * @param max the new maximum, which must be greater than the min
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public void setExtrema(final int min, final int max)
   {
      checkWidget();
      if (min >= 0 && min < max && (min != minimum || max != maximum))
      {
         minimum = min;
         maximum = max;
         if (lowerValue < minimum)
         {
            lowerValue = minimum;
         }
         else if (lowerValue > maximum)
         {
            lowerValue = maximum;
         }
         if (upperValue < minimum)
         {
            upperValue = minimum;
         }
         else if (upperValue > maximum)
         {
            upperValue = maximum;
         }
         if (isHighQuality)
         {
            setTickFactors();
         }
         redraw();
      }
   }

   /**
    * Sets the amount that the receiver's value will be modified by when the page increment/decrement areas are selected to the
    * argument, which must be at least one.
    *
    * @param pageIncrement the page increment (must be greater than zero)
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public void setPageIncrement(final int pageIncrement)
   {
      checkWidget();
      this.pageIncrement = pageIncrement;
      if (isHighQuality)
      {
         setTickFactors();
      }
      redraw();
   }

   /**
    * Sets the 'selection', which is the receiver's value. The lower value must be less than or equal to the upper value.
    * Additionally, both values must be inclusively between the slider minimum and maximum. If either condition fails, no action is
    * taken.
    *
    * @param value the new selection (first value is lower value, second value is upper value)
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    */
   public void setSelection(final int[] values)
   {
      if (values.length == 2)
      {
         setSelection(values[0], values[1]);
      }
   }

   /**
    * Sets the 'selection', which is the receiver's value. The lower value must be less than or equal to the upper value.
    * Additionally, both values must be inclusively between the slider minimum and maximum. If either condition fails, no action is
    * taken.
    *
    * @param lowerValue the new lower selection
    * @param upperValue the new upper selection
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    * @see #getMinimum()
    * @see #getMaximum()
    */
   public void setSelection(final int lowerValue, final int upperValue)
   {
      checkWidget();
      if (lowerValue <= upperValue && lowerValue >= minimum && upperValue <= maximum && (this.lowerValue != lowerValue || this.upperValue != upperValue))
      {
         this.lowerValue = lowerValue;
         this.upperValue = upperValue;
         redraw();
      }
   }

   /**
    * Sets the 'upper selection', which is the upper receiver's value, to the input argument which must be greater than or equal to
    * the current 'lower selection' and less or equal to the maximum. If either condition fails, no action is taken.
    *
    * @param value the new upper selection
    *
    * @exception SWTException
    *               <ul>
    *               <li>ERROR_WIDGET_DISPOSED - if the receiver has been disposed</li>
    *               <li>ERROR_THREAD_INVALID_ACCESS - if not called from the thread that created the receiver</li>
    *               </ul>
    * @see #getLowerValue()
    * @see #getMaximum()
    * @see #setSelection(int, int)
    */
   public void setUpperValue(final int value)
   {
      setSelection(lowerValue, value);
   }
}
