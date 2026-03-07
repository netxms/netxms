/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.HopInfo;
import org.netxms.client.topology.NetworkPath;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.RoundedLabel;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.FontTools;
import org.xnap.commons.i18n.I18n;

/**
 * Vertical stepper/timeline visualization for network path hops.
 */
public class NetworkPathStepper extends Composite
{
   private static final int DOT_SIZE = 12;
   private static final int SPINE_WIDTH = 40;
   private static final int LINE_WIDTH = 2;

   private final I18n i18n = LocalizationHelper.getI18n(NetworkPathStepper.class);

   private ScrolledComposite scroller;
   private Composite content;
   private ColorCache colors;
   private Font nodeNameFont;
   private Font badgeFont;
   private Font detailFont;
   private NXCSession session;

   /**
    * Create network path stepper widget.
    *
    * @param parent parent composite
    */
   public NetworkPathStepper(Composite parent)
   {
      super(parent, SWT.NONE);

      session = Registry.getSession();
      colors = new ColorCache(this);

      Font defaultFont = JFaceResources.getDefaultFont();
      nodeNameFont = FontTools.createAdjustedFont(defaultFont, 0, SWT.BOLD);
      badgeFont = FontTools.createAdjustedFont(defaultFont, -2, SWT.BOLD);
      detailFont = FontTools.createAdjustedFont(defaultFont, -1, SWT.NORMAL);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      setLayout(layout);

      scroller = new ScrolledComposite(this, SWT.V_SCROLL);
      scroller.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(false);
      scroller.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      content = new Composite(scroller, SWT.NONE);
      GridLayout contentLayout = new GridLayout();
      contentLayout.marginHeight = 8;
      contentLayout.marginWidth = 8;
      contentLayout.verticalSpacing = 0;
      content.setLayout(contentLayout);
      content.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      scroller.setContent(content);
      scroller.addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            updateContentSize();
         }
      });

      addDisposeListener((e) -> {
         if (nodeNameFont != null)
            nodeNameFont.dispose();
         if (badgeFont != null)
            badgeFont.dispose();
         if (detailFont != null)
            detailFont.dispose();
      });
   }

   /**
    * Set network path to display.
    *
    * @param path network path (can be null to clear)
    */
   public void setNetworkPath(NetworkPath path)
   {
      for(org.eclipse.swt.widgets.Control c : content.getChildren())
         c.dispose();

      if (path == null)
      {
         updateContentSize();
         return;
      }

      List<HopInfo> hops = path.getPath();
      boolean isComplete = path.isComplete();

      List<HopInfo> displayHops = new ArrayList<>(hops);
      if (!isComplete)
         displayHops.add(new HopInfo(displayHops.size()));

      for(int i = 0; i < displayHops.size(); i++)
      {
         HopInfo hop = displayHops.get(i);
         boolean isLast = (i == displayHops.size() - 1);
         boolean isIncompleteMarker = (!isComplete && isLast);
         createHopRow(hop, isLast, isIncompleteMarker);
      }

      content.layout(true, true);
      updateContentSize();
   }

   /**
    * Create a single hop row in the stepper.
    */
   private void createHopRow(HopInfo hop, boolean isLast, boolean isIncompleteMarker)
   {
      Composite row = new Composite(content, SWT.NONE);
      row.setBackground(content.getBackground());
      row.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      GridLayout rowLayout = new GridLayout(2, false);
      rowLayout.marginHeight = 0;
      rowLayout.marginWidth = 0;
      rowLayout.horizontalSpacing = 8;
      rowLayout.verticalSpacing = 0;
      row.setLayout(rowLayout);

      RGB hopColor = getHopColor(hop.getType());
      boolean dashedLine = (hop.getType() == HopInfo.VPN) || (hop.getType() == HopInfo.PROXY);

      // Spine canvas
      Canvas spine = new Canvas(row, SWT.DOUBLE_BUFFERED);
      spine.setBackground(content.getBackground());
      GridData spineGd = new GridData(SWT.CENTER, SWT.FILL, false, true);
      spineGd.widthHint = SPINE_WIDTH;
      spine.setLayoutData(spineGd);

      spine.addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
            GC gc = e.gc;
            gc.setAntialias(SWT.ON);
            Point size = spine.getSize();
            int cx = size.x / 2;
            int dotY = DOT_SIZE / 2 + 4;

            Color dotColor = colors.create(hopColor);

            if (isIncompleteMarker)
            {
               // Hollow circle with dashed orange border
               gc.setForeground(dotColor);
               gc.setLineWidth(2);
               gc.setLineStyle(SWT.LINE_DASH);
               gc.drawOval(cx - DOT_SIZE / 2, dotY - DOT_SIZE / 2, DOT_SIZE, DOT_SIZE);
            }
            else
            {
               // Filled circle
               gc.setBackground(dotColor);
               gc.fillOval(cx - DOT_SIZE / 2, dotY - DOT_SIZE / 2, DOT_SIZE, DOT_SIZE);
            }

            // Connecting line to next hop
            if (!isLast)
            {
               gc.setForeground(dotColor);
               gc.setLineWidth(LINE_WIDTH);
               gc.setLineStyle(dashedLine ? SWT.LINE_DASH : SWT.LINE_SOLID);
               gc.drawLine(cx, dotY + DOT_SIZE / 2 + 2, cx, size.y);
            }
         }
      });

      // Content area
      Composite contentArea = new Composite(row, SWT.NONE);
      contentArea.setBackground(content.getBackground());
      contentArea.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      GridLayout contentLayout = new GridLayout();
      contentLayout.marginHeight = 4;
      contentLayout.marginWidth = 0;
      contentLayout.verticalSpacing = 2;
      contentArea.setLayout(contentLayout);

      if (isIncompleteMarker)
      {
         // Incomplete path indicator
         Label nameLabel = new Label(contentArea, SWT.NONE);
         nameLabel.setBackground(content.getBackground());
         nameLabel.setFont(nodeNameFont);
         nameLabel.setForeground(colors.create(getHopColor(HopInfo.PROXY))); // orange
         nameLabel.setText(i18n.tr("Path incomplete"));
         nameLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

         Label detailLabel = new Label(contentArea, SWT.NONE);
         detailLabel.setBackground(content.getBackground());
         detailLabel.setFont(detailFont);
         detailLabel.setForeground(colors.create(getHopColor(HopInfo.PROXY))); // orange
         detailLabel.setText(i18n.tr("Next hop could not be resolved"));
         detailLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }
      else
      {
         // Node name line
         String nodeName = (hop.getNodeId() > 0) ? session.getObjectName(hop.getNodeId()) : "??";
         Label nameLabel = new Label(contentArea, SWT.NONE);
         nameLabel.setBackground(content.getBackground());
         nameLabel.setFont(nodeNameFont);
         nameLabel.setText(nodeName + "  [" + hop.getNodeId() + "]");
         nameLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

         // Detail line with type badge
         Composite detailRow = new Composite(contentArea, SWT.NONE);
         detailRow.setBackground(content.getBackground());
         detailRow.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

         GridLayout detailLayout = new GridLayout(2, false);
         detailLayout.marginHeight = 0;
         detailLayout.marginWidth = 0;
         detailLayout.horizontalSpacing = 8;
         detailRow.setLayout(detailLayout);

         // Type badge
         RoundedLabel badge = new RoundedLabel(detailRow);
         badge.setFont(badgeFont);
         badge.setText(getTypeName(hop.getType()));
         badge.setLabelBackground(colors.create(hopColor));
         badge.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));

         // Detail text
         String detail = getDetailText(hop);
         if (!detail.isEmpty())
         {
            Label detailLabel = new Label(detailRow, SWT.NONE);
            detailLabel.setBackground(content.getBackground());
            detailLabel.setFont(detailFont);
            detailLabel.setForeground(getDisplay().getSystemColor(SWT.COLOR_DARK_GRAY));
            detailLabel.setText(detail);
            detailLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
         }
      }
   }

   /**
    * Get color for hop type.
    */
   private RGB getHopColor(int type)
   {
      switch(type)
      {
         case HopInfo.ROUTE:
            return new RGB(59, 130, 246);   // Blue
         case HopInfo.VPN:
            return new RGB(139, 92, 246);   // Purple
         case HopInfo.PROXY:
            return new RGB(245, 158, 11);   // Orange
         case HopInfo.L2_LINK:
            return new RGB(20, 184, 166);   // Teal
         case HopInfo.DESTINATION:
            return new RGB(34, 197, 94);    // Green
         default:
            return new RGB(156, 163, 175);  // Grey
      }
   }

   /**
    * Get display name for hop type.
    */
   private String getTypeName(int type)
   {
      switch(type)
      {
         case HopInfo.ROUTE:
            return "ROUTE";
         case HopInfo.VPN:
            return "VPN";
         case HopInfo.PROXY:
            return "PROXY";
         case HopInfo.L2_LINK:
            return "L2_LINK";
         case HopInfo.DESTINATION:
            return "DESTINATION";
         default:
            return "?";
      }
   }

   /**
    * Get detail text for a hop.
    */
   private String getDetailText(HopInfo hop)
   {
      StringBuilder sb = new StringBuilder();
      switch(hop.getType())
      {
         case HopInfo.ROUTE:
            if (!hop.getName().isEmpty())
               sb.append(hop.getName());
            if (hop.getNextHop() != null)
            {
               if (sb.length() > 0)
                  sb.append(" \u2014 ");
               sb.append("next hop ");
               sb.append(hop.getNextHop().getHostAddress());
            }
            break;
         case HopInfo.VPN:
            if (!hop.getName().isEmpty())
               sb.append(hop.getName());
            break;
         case HopInfo.PROXY:
            if (!hop.getName().isEmpty())
               sb.append(hop.getName());
            break;
         case HopInfo.L2_LINK:
            if (!hop.getName().isEmpty())
               sb.append(hop.getName());
            else
               sb.append("L2 link");
            break;
         case HopInfo.DESTINATION:
            sb.append(i18n.tr("Destination node"));
            break;
      }
      return sb.toString();
   }

   /**
    * Update scrolled content size.
    */
   private void updateContentSize()
   {
      Point size = content.computeSize(scroller.getClientArea().width, SWT.DEFAULT);
      content.setSize(size);
   }

   /**
    * Get CSV representation of the displayed path.
    *
    * @param path network path
    * @return CSV text
    */
   public static String toCSV(NetworkPath path)
   {
      StringBuilder sb = new StringBuilder();
      sb.append("Hop,Node,Node ID,Type,Details\n");
      NXCSession session = Registry.getSession();
      List<HopInfo> hops = path.getPath();
      for(int i = 0; i < hops.size(); i++)
      {
         HopInfo hop = hops.get(i);
         String nodeName = (hop.getNodeId() > 0) ? session.getObjectName(hop.getNodeId()) : "";
         sb.append(i + 1);
         sb.append(',');
         sb.append(csvEscape(nodeName));
         sb.append(',');
         sb.append(hop.getNodeId());
         sb.append(',');
         sb.append(getTypeNameStatic(hop.getType()));
         sb.append(',');
         sb.append(csvEscape(getDetailTextStatic(hop)));
         sb.append('\n');
      }
      return sb.toString();
   }

   /**
    * Escape value for CSV.
    */
   private static String csvEscape(String value)
   {
      if (value.contains(",") || value.contains("\"") || value.contains("\n"))
         return "\"" + value.replace("\"", "\"\"") + "\"";
      return value;
   }

   /**
    * Get type name (static version for CSV export).
    */
   private static String getTypeNameStatic(int type)
   {
      switch(type)
      {
         case HopInfo.ROUTE:
            return "ROUTE";
         case HopInfo.VPN:
            return "VPN";
         case HopInfo.PROXY:
            return "PROXY";
         case HopInfo.L2_LINK:
            return "L2_LINK";
         case HopInfo.DESTINATION:
            return "DUMMY";
         default:
            return "";
      }
   }

   /**
    * Get detail text (static version for CSV export).
    */
   private static String getDetailTextStatic(HopInfo hop)
   {
      StringBuilder sb = new StringBuilder();
      switch(hop.getType())
      {
         case HopInfo.ROUTE:
            if (!hop.getName().isEmpty())
               sb.append(hop.getName());
            if (hop.getNextHop() != null)
            {
               if (sb.length() > 0)
                  sb.append(" -- ");
               sb.append("next hop ");
               sb.append(hop.getNextHop().getHostAddress());
            }
            break;
         case HopInfo.VPN:
         case HopInfo.PROXY:
         case HopInfo.L2_LINK:
            if (!hop.getName().isEmpty())
               sb.append(hop.getName());
            break;
         case HopInfo.DESTINATION:
            sb.append("Intermediate hop");
            break;
      }
      return sb.toString();
   }
}
