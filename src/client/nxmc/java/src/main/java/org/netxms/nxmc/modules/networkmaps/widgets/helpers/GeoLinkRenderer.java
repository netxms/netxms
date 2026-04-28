/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.LinkDataLocation;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Draws one {@link NetworkMapLink} onto the geographical canvas.
 *
 * The caller decides on the link's polyline (DIRECT → straight line; MANHATTAN
 * → synthesized L-shape; BENDPOINTS → user-authored geographical bendpoints
 * projected to pixel space) and passes the intermediate points (if any) via
 * {@code bendPointsPx}. This class is purely a pixel-space renderer — it draws
 * src → bends → dst as a polyline, falling through to a straight line when no
 * intermediate points are supplied.
 *
 * Color, width and style are resolved through {@link LinkStylingHelper} so the
 * behaviour matches the Zest-based graph canvas.
 */
public final class GeoLinkRenderer
{
   // Pill-shaped label colors (match ConnectorLabel defaults)
   private static final Color LABEL_FG = new Color(Display.getDefault(), new RGB(0, 0, 0));
   private static final Color LABEL_BG = new Color(Display.getDefault(), new RGB(240, 240, 240));
   private static final Color LABEL_BORDER = new Color(Display.getDefault(), new RGB(64, 64, 64));

   // Fallback line color when the styling helper returns no specific color
   // (e.g. an "unknown"-status link on a map with no custom default color).
   // Matches Zest's GraphConnection default of ColorConstants.lightGray, so a
   // graph-mode link and a geo-mode link painted from the same model end up
   // the same colour.
   private static final Color DEFAULT_LINE_COLOR = new Color(Display.getDefault(), new RGB(192, 192, 192));

   /**
    * Label positioning mirrors {@code MultiLabelConnectionLocator} used on the
    * Zest canvas in spirit: the link is divided into zones for connector,
    * OBJECT1, CENTER, OBJECT2 labels. On geo we use an absolute pixel offset
    * for the connector labels instead of a proportional fraction, because the
    * proportional approach pulls labels right against the pin tip on links
    * (especially short / high-zoom links) and the two connector labels can
    * collide. The OBJECT1, CENTER, OBJECT2 pills stay proportional (in their
    * three-segment zones) since they're driven by the link's
    * {@link org.netxms.client.maps.configs.LinkConfig#getLabelPosition()}.
    *
    * Fractional positions along the link (used for OBJECT1, CENTER, OBJECT2):
    *   OBJECT1 DCI : (1 + 3*p)/11   where p = labelPosition/100
    *   CENTER      : (4 + 3*p)/11
    *   OBJECT2 DCI : (7 + 3*p)/11
    */
   private static final int CONNECTOR_OFFSET_PX = 50;

   private GeoLinkRenderer()
   {
   }

   /**
    * Draw a link between two pins, including (if requested) the direction
    * arrow at the target end and labels for connector names, link name and
    * DCI values. Matches the visual set produced by the Zest-based
    * {@code MapLabelProvider}.
    *
    * @param gc drawing context (caller owns setup/teardown)
    * @param srcPx pixel position of the source pin's tip
    * @param dstPx pixel position of the destination pin's tip
    * @param link link model
    * @param session NetXMS session for resolving link status/interface objects
    * @param dciValueProvider DCI value provider (active thresholds lookup)
    * @param colors color cache for custom/script colors
    * @param defaultLinkColor map-level default color (may be null)
    * @param defaultLinkColorSource map-level default color source
    * @param defaultLinkWidth map-level default line width
    * @param defaultLinkStyle map-level default line style
    * @param bendPointsPx intermediate polyline vertices in pixel space (geo
    *                     bendpoints projected to pixels, or synthesized
    *                     Manhattan corners); null or empty draws a straight
    *                     line
    * @param showLinkDirection if true, draw a direction arrow at the target end
    * @param showLabels if false, skip connector names, link name and DCI labels
    * @param labelFont font used for all text labels on the link
    */
   public static void drawLink(GC gc, Point srcPx, Point dstPx, NetworkMapLink link,
         NXCSession session, LinkDciValueProvider dciValueProvider, ColorCache colors,
         Color defaultLinkColor, int defaultLinkColorSource,
         int defaultLinkWidth, int defaultLinkStyle,
         List<Point> bendPointsPx,
         boolean showLinkDirection,
         boolean showLabels,
         Font labelFont)
   {
      Color color = resolveLineColor(link, session, dciValueProvider, colors, defaultLinkColor, defaultLinkColorSource);
      drawLinkWithColor(gc, srcPx, dstPx, link, session, dciValueProvider, colors,
            color, defaultLinkWidth, defaultLinkStyle, bendPointsPx, showLinkDirection, showLabels, labelFont);
   }

   /**
    * Same as {@link #drawLink} but skips the color resolution — the caller
    * supplies the already-resolved line color (typically cached on the per-link
    * geometry). Avoids the {@link LinkStylingHelper#resolveLinkColor} second
    * call when the caller wants to render the line and then later render the
    * labels on top in a separate pass.
    */
   public static void drawLinkWithColor(GC gc, Point srcPx, Point dstPx, NetworkMapLink link,
         NXCSession session, LinkDciValueProvider dciValueProvider, ColorCache colors,
         Color color,
         int defaultLinkWidth, int defaultLinkStyle,
         List<Point> bendPointsPx,
         boolean showLinkDirection,
         boolean showLabels,
         Font labelFont)
   {
      int width = LinkStylingHelper.resolveLinkWidth(link, defaultLinkWidth);
      int style = LinkStylingHelper.resolveLinkStyle(link, defaultLinkStyle);

      int oldWidth = gc.getLineWidth();
      int oldStyle = gc.getLineStyle();
      Color oldFg = gc.getForeground();
      Color oldBg = gc.getBackground();
      Font oldFont = gc.getFont();

      gc.setForeground(color);
      if (width > 0)
         gc.setLineWidth(width);
      if (style >= 1 && style <= 5)
         gc.setLineStyle(style);

      // Polyline whenever the caller supplied intermediate points (geo
      // bendpoints or Manhattan corners); otherwise straight line.
      boolean useBendpoints = (bendPointsPx != null) && !bendPointsPx.isEmpty();
      if (useBendpoints)
      {
         int[] pts = new int[(bendPointsPx.size() + 2) * 2];
         int i = 0;
         pts[i++] = srcPx.x;
         pts[i++] = srcPx.y;
         for(Point bp : bendPointsPx)
         {
            pts[i++] = bp.x;
            pts[i++] = bp.y;
         }
         pts[i++] = dstPx.x;
         pts[i] = dstPx.y;
         gc.drawPolyline(pts);
      }
      else
      {
         gc.drawLine(srcPx.x, srcPx.y, dstPx.x, dstPx.y);
      }

      // Direction arrow at the target end. Set bg just for the fill, then restore.
      if (showLinkDirection)
      {
         Point arrowFrom = useBendpoints
               ? bendPointsPx.get(bendPointsPx.size() - 1)
               : srcPx;
         if (color != null)
            gc.setBackground(color);
         drawDirectionArrow(gc, arrowFrom, dstPx, width);
      }

      if (showLabels)
         drawLinkLabels(gc, srcPx, dstPx, link, session, dciValueProvider, colors, color, bendPointsPx, labelFont);

      gc.setLineWidth(oldWidth);
      gc.setLineStyle(oldStyle);
      gc.setForeground(oldFg);
      gc.setBackground(oldBg);
      if (oldFont != null)
         gc.setFont(oldFont);
   }

   /**
    * Draw a link's connector / DCI / link-name labels in pixel space.
    * Companion to {@link #drawLink} so the caller can render labels in a
    * separate pass — on the geographical canvas pins are drawn between the
    * polyline and the labels, and rendering labels last keeps short links'
    * middle labels from being obscured by the pin pills.
    *
    * <p>This method does not touch {@code gc}'s line width / line style; it
    * uses solid lines for the pill borders. It restores the font on exit.</p>
    *
    * @param lineColor the resolved link color, used for the special-case
    *                  proxy/tunnel CENTER label background
    */
   public static void drawLinkLabels(GC gc, Point srcPx, Point dstPx, NetworkMapLink link,
         NXCSession session, LinkDciValueProvider dciValueProvider, ColorCache colors,
         Color lineColor, List<Point> bendPointsPx, Font labelFont)
   {
      // Label rendering uses the SOLID line style regardless of the link's style
      int oldStyle = gc.getLineStyle();
      Font oldFont = gc.getFont();
      gc.setLineStyle(SWT.LINE_SOLID);
      if (labelFont != null)
         gc.setFont(labelFont);

      boolean useBendpoints = (bendPointsPx != null) && !bendPointsPx.isEmpty();

      // Build the polyline (src → bendpoints → dst) for proportional positioning.
      Point[] polyline;
      if (useBendpoints)
      {
         polyline = new Point[bendPointsPx.size() + 2];
         polyline[0] = srcPx;
         for(int i = 0; i < bendPointsPx.size(); i++)
            polyline[i + 1] = bendPointsPx.get(i);
         polyline[polyline.length - 1] = dstPx;
      }
      else
      {
         polyline = new Point[] { srcPx, dstPx };
      }

      double labelPosition = link.getConfig().getLabelPosition() / 100.0;
      double fracObj1   = (1.0 + 3.0 * labelPosition) / 11.0;
      double fracCenter = (4.0 + 3.0 * labelPosition) / 11.0;
      double fracObj2   = (7.0 + 3.0 * labelPosition) / 11.0;

      // Compute cumulative segment lengths once and reuse them for every
      // fractional lookup below — pointAtFraction's naive walk does a
      // Math.sqrt per segment, which adds up to 6 walks per link otherwise
      // (totalLength + five label positions).
      double[] cumDist = cumulativeLengths(polyline);
      double totalLength = cumDist[cumDist.length - 1];

      // Connector names: at a fixed pixel offset from each pin so there's
      // always breathing room from the pin pill, regardless of link length /
      // map zoom. On very short links the offset is clamped to a third of
      // the length so the two connector labels don't cross past each other.
      double connectorOffset = Math.min(CONNECTOR_OFFSET_PX, totalLength / 3.0);
      double connectorFracSrc = (totalLength > 0) ? (connectorOffset / totalLength) : 0;
      double connectorFracDst = (totalLength > 0) ? (1.0 - connectorOffset / totalLength) : 1.0;
      String srcLabel = connectorName(link, false, session);
      if (srcLabel != null)
         drawPillAt(gc, pointAtFraction(polyline, cumDist, connectorFracSrc), srcLabel, null, colors);
      String dstLabel = connectorName(link, true, session);
      if (dstLabel != null)
         drawPillAt(gc, pointAtFraction(polyline, cumDist, connectorFracDst), dstLabel, null, colors);

      // Center label: link name + CENTER DCI values.
      // For proxy / tunnel link types the graph canvas paints the CENTER label
      // with the link's line color as background (text auto-contrasted) — match it here.
      boolean hasName = link.hasName();
      boolean hasDci = link.hasDciData();
      Color centerBg = isProxyOrTunnel(link.getType()) ? lineColor : null;
      if (hasName || hasDci)
      {
         StringBuilder centerText = new StringBuilder();
         if (hasName)
            centerText.append(link.getName());
         if (hasDci)
         {
            String dci = dciValueProvider.getDciDataAsString(link, LinkDataLocation.CENTER);
            if ((dci != null) && !dci.isEmpty())
            {
               if (centerText.length() > 0)
                  centerText.append('\n');
               centerText.append(dci);
            }
         }
         if (centerText.length() > 0)
            drawPillAt(gc, pointAtFraction(polyline, cumDist, fracCenter), centerText.toString(), centerBg, colors);

         if (hasDci)
         {
            String obj1 = dciValueProvider.getDciDataAsString(link, LinkDataLocation.OBJECT1);
            if ((obj1 != null) && !obj1.isEmpty())
               drawPillAt(gc, pointAtFraction(polyline, cumDist, fracObj1), obj1, null, colors);
            String obj2 = dciValueProvider.getDciDataAsString(link, LinkDataLocation.OBJECT2);
            if ((obj2 != null) && !obj2.isEmpty())
               drawPillAt(gc, pointAtFraction(polyline, cumDist, fracObj2), obj2, null, colors);
         }
      }

      gc.setLineStyle(oldStyle);
      if (oldFont != null)
         gc.setFont(oldFont);
   }

   /**
    * Resolve the link's effective color for label rendering. Exposed so the
    * GeoNetworkMapViewer's second pass can pass the same color to
    * {@link #drawLinkLabels} as was used by the line itself (only meaningful
    * for proxy/tunnel CENTER labels). Substitutes the same lightGray fallback
    * used by {@link #drawLink} when the styling helper has no opinion.
    */
   public static Color resolveLineColor(NetworkMapLink link, NXCSession session,
         LinkDciValueProvider dciValueProvider, ColorCache colors,
         Color defaultLinkColor, int defaultLinkColorSource)
   {
      Color color = LinkStylingHelper.resolveLinkColor(link, session, dciValueProvider, colors,
            defaultLinkColor, defaultLinkColorSource);
      return (color != null) ? color : DEFAULT_LINE_COLOR;
   }

   /**
    * Resolve the connector name for one end of a link: prefer explicit
    * {@code connectorName1/2}, fall back to the interface object's name.
    */
   private static String connectorName(NetworkMapLink link, boolean second, NXCSession session)
   {
      String name = second ? link.getConnectorName2() : link.getConnectorName1();
      if ((name != null) && !name.isBlank())
         return name;
      long ifaceId = second ? link.getInterfaceId2() : link.getInterfaceId1();
      return (ifaceId > 0) ? session.getObjectName(ifaceId) : null;
   }

   /**
    * Draw a pill at the given pixel position. {@code background} is the fill
    * color; when {@code null}, the default light-grey fill is used and the
    * text is drawn in black. When non-null, the text color is auto-selected
    * for contrast against the supplied background.
    */
   private static void drawPillAt(GC gc, Point center, String text, Color background, ColorCache colors)
   {
      drawPillLabel(gc, center.x, center.y, text, background, colors);
   }

   /**
    * Link types whose CENTER label is painted with the link's own line color
    * as background (matches Zest-canvas behaviour for proxy / tunnel links).
    */
   private static boolean isProxyOrTunnel(int type)
   {
      return (type == NetworkMapLink.AGENT_TUNEL) || (type == NetworkMapLink.AGENT_PROXY)
            || (type == NetworkMapLink.ICMP_PROXY) || (type == NetworkMapLink.SNMP_PROXY)
            || (type == NetworkMapLink.SSH_PROXY) || (type == NetworkMapLink.ZONE_PROXY);
   }

   /**
    * Compute cumulative segment lengths for a polyline. {@code result[0]} is
    * 0, {@code result[i]} is the total path length from {@code polyline[0]}
    * to {@code polyline[i]}. The final entry is the total polyline length.
    */
   private static double[] cumulativeLengths(Point[] polyline)
   {
      double[] cum = new double[polyline.length];
      cum[0] = 0;
      for(int i = 1; i < polyline.length; i++)
      {
         double dx = polyline[i].x - polyline[i - 1].x;
         double dy = polyline[i].y - polyline[i - 1].y;
         cum[i] = cum[i - 1] + Math.sqrt(dx * dx + dy * dy);
      }
      return cum;
   }

   /**
    * Find the point on {@code polyline} at the given fractional position
    * along its total length, using the precomputed cumulative-distance array
    * {@code cum} (see {@link #cumulativeLengths}). Avoids re-walking +
    * Math.sqrt'ing every segment on each call — drawLinkLabels makes five
    * lookups per link, so doing this once instead of five times is the bulk
    * of the perf win.
    */
   private static Point pointAtFraction(Point[] polyline, double[] cum, double fraction)
   {
      double totalLength = cum[cum.length - 1];
      if ((totalLength < 0.001) || (fraction <= 0.0))
         return polyline[0];
      if (fraction >= 1.0)
         return polyline[polyline.length - 1];

      double target = totalLength * fraction;
      for(int i = 1; i < cum.length; i++)
      {
         if (cum[i] >= target)
         {
            double segLen = cum[i] - cum[i - 1];
            double t = (segLen < 0.001) ? 0 : (target - cum[i - 1]) / segLen;
            int x = polyline[i - 1].x + (int)Math.round((polyline[i].x - polyline[i - 1].x) * t);
            int y = polyline[i - 1].y + (int)Math.round((polyline[i].y - polyline[i - 1].y) * t);
            return new Point(x, y);
         }
      }
      return polyline[polyline.length - 1];
   }

   /**
    * Draw {@code text} inside a rounded, filled rectangle centered at
    * {@code (cx, cy)}. Matches the style of {@link ConnectorLabel} used on the
    * Zest-based graph canvas. With {@code background == null}: light-grey
    * fill, black text, dark border. With a custom {@code background}: filled
    * with that color, text auto-contrasted against it.
    */
   private static void drawPillLabel(GC gc, int cx, int cy, String text, Color background, ColorCache colors)
   {
      Point textSize = gc.textExtent(text);
      int padX = 4;
      int padY = 2;
      int w = textSize.x + padX * 2;
      int h = textSize.y + padY * 2;
      int x = cx - w / 2;
      int y = cy - h / 2;

      Color oldBg = gc.getBackground();
      Color oldFg = gc.getForeground();
      int oldLineWidth = gc.getLineWidth();
      int oldAntialias = gc.getAntialias();

      gc.setAntialias(SWT.ON);
      Color fill = (background != null) ? background : LABEL_BG;
      gc.setBackground(fill);
      gc.fillRoundRectangle(x, y, w, h, 8, 8);
      gc.setForeground(LABEL_BORDER);
      gc.setLineWidth(1);
      gc.drawRoundRectangle(x, y, w, h, 8, 8);
      Color textColor = (background != null) ? ColorConverter.selectTextColorByBackgroundColor(background, colors) : LABEL_FG;
      gc.setForeground(textColor);
      gc.drawText(text, x + padX, y + padY, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);

      gc.setBackground(oldBg);
      gc.setForeground(oldFg);
      gc.setLineWidth(oldLineWidth);
      gc.setAntialias(oldAntialias);
   }

   /**
    * Draw a filled triangular arrow pointing from {@code from} toward {@code to}.
    * The arrow's tip sits at {@code to}; the scale matches the current line width.
    */
   private static void drawDirectionArrow(GC gc, Point from, Point to, int lineWidth)
   {
      double dx = to.x - from.x;
      double dy = to.y - from.y;
      double len = Math.sqrt(dx * dx + dy * dy);
      if (len < 0.001)
         return;
      double ux = dx / len;
      double uy = dy / len;
      // Perpendicular unit vector
      double px = -uy;
      double py = ux;
      double scale = Math.max(lineWidth, 1) * 0.9 + 2;
      double arrowLength = scale * 2.3;
      double arrowHalfWidth = scale;
      int baseX = (int)(to.x - ux * arrowLength);
      int baseY = (int)(to.y - uy * arrowLength);
      int[] tri = new int[] {
         to.x, to.y,
         (int)(baseX + px * arrowHalfWidth), (int)(baseY + py * arrowHalfWidth),
         (int)(baseX - px * arrowHalfWidth), (int)(baseY - py * arrowHalfWidth)
      };
      gc.fillPolygon(tri);
   }

   /**
    * Return true when {@code point} lies inside the rectangle {@code (0,0,w,h)}.
    * Used by callers to apply the link-visibility rule: a link is drawn only
    * when at least one endpoint's pixel position is inside the viewport.
    */
   public static boolean isInViewport(Point point, int viewportWidth, int viewportHeight)
   {
      return (point.x >= 0) && (point.x < viewportWidth) && (point.y >= 0) && (point.y < viewportHeight);
   }

   /**
    * Convenience: is either endpoint visible in the viewport?
    */
   public static boolean hasVisibleEndpoint(Point srcPx, Point dstPx, int viewportWidth, int viewportHeight)
   {
      return isInViewport(srcPx, viewportWidth, viewportHeight) || isInViewport(dstPx, viewportWidth, viewportHeight);
   }
}
