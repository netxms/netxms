/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import java.util.List;
import org.eclipse.draw2d.Connection;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.LineBorder;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Insets;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.PointList;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.maps.LinkDataDirection;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Connector label
 */
public class ConnectorLabel extends Label
{
	private static final Color DEFAULT_FOREGROUND_COLOR = new Color(Display.getCurrent(), 0, 0, 0);
   private static final Color DEFAULT_BACKGROUND_COLOR = new Color(Display.getCurrent(), 240, 240, 240);
   private static final Color DEFAULT_BORDER_COLOR = new Color(Display.getCurrent(), 64, 64, 64);
   private static final int ARROW_GAP = 2;

	private Color backgroundColor = null;
   private MapLabelProvider labelProvider;
   private List<LinkLabelLine> lines = null;
   private Dimension linesSize = null;

	/**
	 * Create connector label with text
	 *
	 * @param s label's text
	 */
	public ConnectorLabel(String s, MapLabelProvider labelProvider)
	{
		super(s);
		initLabel();
		this.labelProvider = labelProvider;
      setFont(labelProvider.getLabelFont());
      setBorder(new LineBorder(DEFAULT_BORDER_COLOR, 1, SWT.LINE_DOT));
	}

	/**
    * Create connector label with text and custom
    * background color
    *
    * @param s label`s text
    * @param backgroundColor label`s background color
    */
   public ConnectorLabel(String s, MapLabelProvider labelProvider, Color backgroundColor)
   {
      super(s);
      this.backgroundColor = backgroundColor;
      this.labelProvider = labelProvider;
      initLabel();
   }

   /**
    * Create connector label with lines that can have data direction marks
    *
    * @param lines label lines
    * @param labelProvider map label provider
    */
   public ConnectorLabel(List<LinkLabelLine> lines, MapLabelProvider labelProvider)
   {
      this(LinkLabelLine.join(lines), labelProvider);
      this.lines = lines;
   }

   /**
    * Create connector label with lines that can have data direction marks and custom background color
    *
    * @param lines label lines
    * @param labelProvider map label provider
    * @param backgroundColor label's background color
    */
   public ConnectorLabel(List<LinkLabelLine> lines, MapLabelProvider labelProvider, Color backgroundColor)
   {
      this(LinkLabelLine.join(lines), labelProvider, backgroundColor);
      this.lines = lines;
   }

	/**
	 * Create connector label with image and text
	 *
	 * @param s label's text
	 * @param i label's image
	 */
	public ConnectorLabel(String s, Image i)
	{
		super(s, i);
		initLabel();
	}

	/**
	 * Common initialization
	 */
	private void initLabel()
	{
		setForegroundColor(backgroundColor == null ? DEFAULT_FOREGROUND_COLOR :
		   ColorConverter.selectTextColorByBackgroundColor(backgroundColor, labelProvider.getColors()));
		setBackgroundColor(backgroundColor == null ? DEFAULT_BACKGROUND_COLOR : backgroundColor);
	}

   /**
    * @see org.eclipse.draw2d.Label#paintFigure(org.eclipse.draw2d.Graphics)
    */
	@Override
	protected void paintFigure(Graphics gc)
	{
      setFont(labelProvider.getLabelFont());

		Rectangle bounds = getBounds();

		gc.setBackgroundColor(backgroundColor == null ? DEFAULT_BACKGROUND_COLOR : backgroundColor);
		gc.setAntialias(SWT.ON);
		gc.fillRoundRectangle(bounds, 8, 8);

      if (lines != null)
      {
         paintLines(gc);
         return;
      }

		gc.translate(bounds.x, bounds.y);
		final Point pos = getTextLocation();
		gc.drawText(getSubStringText(), pos.x + 3, pos.y);
		gc.translate(-bounds.x, -bounds.y);
	}

   /**
    * Paint label lines. Data direction is shown as an arrow before the text, parallel to the first segment of the link:
    * forward direction points from the link's first element to the second one.
    *
    * @param gc graphics context
    */
   private void paintLines(Graphics gc)
   {
      double[] linkDirection = null;
      if (getParent() instanceof Connection)
      {
         PointList points = ((Connection)getParent()).getPoints();
         if (points.size() > 1)
         {
            Point p1 = points.getPoint(0);
            Point p2 = points.getPoint(1);
            linkDirection = DirectionArrow.unitVector(p1.x, p1.y, p2.x, p2.y);
         }
      }

      gc.setBackgroundColor(gc.getForegroundColor());
      gc.setLineStyle(SWT.LINE_SOLID);

      Rectangle bounds = getBounds();
      Insets insets = getInsets();
      int lineHeight = getLineHeight();
      int separatorWidth = getTextUtilities().getStringExtents(LinkLabelLine.SEGMENT_SEPARATOR, getFont()).width;
      int y = bounds.y + insets.top + 2;
      for(LinkLabelLine line : lines)
      {
         int x = bounds.x + insets.left + 3;
         boolean first = true;
         for(LinkLabelLine.Segment s : line.getSegments())
         {
            if (!first)
               x += separatorWidth;
            first = false;
            if (s.direction != LinkDataDirection.NONE)
            {
               if (linkDirection != null)
               {
                  double sign = (s.direction == LinkDataDirection.FORWARD) ? 1 : -1;
                  DirectionArrow arrow = new DirectionArrow(x + lineHeight / 2, y + lineHeight / 2, linkDirection[0] * sign, linkDirection[1] * sign, lineHeight - 2);
                  gc.setLineWidth(arrow.shaftWidth);
                  gc.drawLine(arrow.shaft[0], arrow.shaft[1], arrow.shaft[2], arrow.shaft[3]);
                  gc.fillPolygon(arrow.head);
               }
               x += lineHeight + ARROW_GAP;
            }
            gc.drawText(s.text, x, y);
            x += getTextUtilities().getStringExtents(s.text, getFont()).width;
         }
         y += lineHeight;
      }
   }

   /**
    * @return height of one label line
    */
   private int getLineHeight()
   {
      return getTextUtilities().getStringExtents("Wg", getFont()).height;
   }

   /**
    * Get size of label lines. Space reserved for direction arrow is a square with side equal to line height, so label size
    * does not depend on link direction.
    *
    * @return size of label lines
    */
   private Dimension getLinesSize()
   {
      if (linesSize == null)
      {
         int lineHeight = getLineHeight();
         int separatorWidth = getTextUtilities().getStringExtents(LinkLabelLine.SEGMENT_SEPARATOR, getFont()).width;
         int width = 0;
         for(LinkLabelLine line : lines)
         {
            int lineWidth = separatorWidth * (line.getSegments().size() - 1);
            for(LinkLabelLine.Segment s : line.getSegments())
            {
               if (s.direction != LinkDataDirection.NONE)
                  lineWidth += lineHeight + ARROW_GAP;
               lineWidth += getTextUtilities().getStringExtents(s.text, getFont()).width;
            }
            width = Math.max(width, lineWidth);
         }
         linesSize = new Dimension(width, lineHeight * lines.size());
      }
      return linesSize;
   }

   /**
    * @see org.eclipse.draw2d.Label#invalidate()
    */
   @Override
   public void invalidate()
   {
      linesSize = null;
      super.invalidate();
   }

   /**
    * @see org.eclipse.draw2d.Label#getPreferredSize(int, int)
    */
	@Override
	public Dimension getPreferredSize(int wHint, int hHint)
	{
      Dimension d = (lines != null) ? getLinesSize().getCopy() : calculateLabelSize(getTextSize());
		Insets insets = getInsets();
		d.expand(insets.getWidth(), insets.getHeight());
		if (getLayoutManager() != null)
			d.union(getLayoutManager().getPreferredSize(this, wHint,
					hHint));
		d.height += 4;
		d.width += 6;
		return d;
	}
}
