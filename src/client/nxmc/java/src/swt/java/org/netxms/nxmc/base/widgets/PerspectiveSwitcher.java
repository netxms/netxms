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
package org.netxms.nxmc.base.widgets;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseTrackAdapter;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.BrandingManager;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveSeparator;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.FontTools;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Custom perspective switcher sidebar widget. Each perspective is shown as an icon with its name centered underneath. Supports
 * expanded (icon + text) and collapsed (icon only) modes with section grouping and scrolling. The selected (or hovered) item is
 * highlighted with a rounded filled rectangle.
 */
public class PerspectiveSwitcher extends Composite
{
   private static final int EXPANDED_WIDTH = 112;
   private static final int COLLAPSED_WIDTH = 48;
   private static final int ICON_SIZE_EXPANDED = 22;
   private static final int ICON_SIZE_COLLAPSED = 22;
   private static final int ITEM_HEIGHT_EXPANDED = 60;
   private static final int ITEM_HEIGHT_COLLAPSED = 44;
   private static final int ITEM_PADDING_TOP = 6;
   private static final int TEXT_GAP = 3;
   private static final int SECTION_SPACING = 16;
   private static final int HIGHLIGHT_MARGIN_H = 6;
   private static final int HIGHLIGHT_MARGIN_V = 3;
   private static final int HIGHLIGHT_COLLAPSED_SIZE = 36;
   private static final int HIGHLIGHT_RADIUS = 12;

   private boolean expanded;
   private Consumer<Perspective> switchCallback;
   private Perspective selectedPerspective;

   private ScrolledComposite scroller;
   private Composite scrollContent;
   private Composite bottomArea;

   private List<Perspective> perspectives;
   private Perspective pinboardPerspective;
   private Map<String, List<Perspective>> sections;
   private List<String> sectionOrder;

   private List<PerspectiveItemComposite> allItems = new ArrayList<>();
   private PerspectiveItemComposite pinboardItem;

   private Color backgroundColor;
   private Color foregroundColor;
   private Color selectionBackground;
   private Color selectionForeground;
   private Color hoverBackground;
   private Color sectionHeaderForeground;
   private Font itemFont;
   private Font toggleFont;

   /**
    * Create perspective switcher.
    *
    * @param parent parent composite
    * @param style SWT style
    * @param perspectives list of perspectives to display
    * @param switchCallback callback invoked when user selects a perspective
    */
   public PerspectiveSwitcher(Composite parent, int style, List<Perspective> perspectives, Consumer<Perspective> switchCallback)
   {
      super(parent, style);
      this.switchCallback = switchCallback;
      this.expanded = PreferenceStore.getInstance().getAsBoolean("PerspectiveSwitcher.Expanded", true);

      backgroundColor = new Color(getDisplay(), BrandingManager.getPerspectiveSwitcherBackground());
      foregroundColor = ThemeEngine.getForegroundColor("Window.PerspectiveSwitcher");
      selectionBackground = new Color(getDisplay(), BrandingManager.getPerspectiveSwitcherSelectionBackground());
      selectionForeground = ThemeEngine.getForegroundColor("Window.PerspectiveSwitcher.Selection");
      hoverBackground = ThemeEngine.getBackgroundColor("Window.PerspectiveSwitcher.Hover");
      sectionHeaderForeground = ThemeEngine.getForegroundColor("Window.PerspectiveSwitcher.SectionHeader");
      itemFont = FontTools.createAdjustedFont(getFont(), -1);
      toggleFont = FontTools.createAdjustedFont(getFont(), 2);

      buildSections(perspectives);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      setLayout(layout);
      setBackground(backgroundColor);

      createContent();

      addDisposeListener((e) -> {
         itemFont.dispose();
         toggleFont.dispose();
         backgroundColor.dispose();
         selectionBackground.dispose();
      });
   }

   /**
    * Build sections from perspective list. Perspectives with same getSectionName() are grouped together. Perspectives with null
    * section name and pinboard get special treatment.
    */
   private void buildSections(List<Perspective> allPerspectives)
   {
      this.perspectives = new ArrayList<>();
      this.pinboardPerspective = null;
      this.sections = new LinkedHashMap<>();
      this.sectionOrder = new ArrayList<>();

      for(Perspective p : allPerspectives)
      {
         if (p instanceof PerspectiveSeparator)
            continue;

         if ("pinboard".equals(p.getId()))
         {
            pinboardPerspective = p;
            continue;
         }

         perspectives.add(p);

         String section = p.getSectionName();
         if (section == null)
            section = "";

         if (!sections.containsKey(section))
         {
            sections.put(section, new ArrayList<>());
            sectionOrder.add(section);
         }
         sections.get(section).add(p);
      }
   }

   /**
    * Create all child widgets.
    */
   private void createContent()
   {
      // Scrollable area for main sections
      scroller = new ScrolledComposite(this, SWT.V_SCROLL);
      scroller.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(false);
      scroller.setBackground(backgroundColor);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, expanded ? ITEM_HEIGHT_EXPANDED : ITEM_HEIGHT_COLLAPSED);

      scrollContent = new Composite(scroller, SWT.NONE);
      GridLayout scrollLayout = new GridLayout();
      scrollLayout.marginWidth = 0;
      scrollLayout.marginHeight = 4;
      scrollLayout.marginTop = 4;
      scrollLayout.verticalSpacing = 0;
      scrollContent.setLayout(scrollLayout);
      scrollContent.setBackground(backgroundColor);

      boolean firstSection = true;
      for(String sectionName : sectionOrder)
      {
         List<Perspective> sectionPerspectives = sections.get(sectionName);

         if (!firstSection)
         {
            // Spacing between sections (replaces section header)
            Label spacer = new Label(scrollContent, SWT.NONE);
            spacer.setBackground(backgroundColor);
            GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
            gd.heightHint = SECTION_SPACING;
            spacer.setLayoutData(gd);
         }
         firstSection = false;

         // Perspective items
         for(Perspective p : sectionPerspectives)
         {
            PerspectiveItemComposite item = new PerspectiveItemComposite(scrollContent, p);
            item.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
            allItems.add(item);
         }
      }

      scroller.setContent(scrollContent);
      scroller.addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            updateScrolledContentSize();
         }
      });

      // Divider
      Label divider = new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL);
      divider.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Bottom area (pinboard + toggle)
      bottomArea = new Composite(this, SWT.NONE);
      GridLayout bottomLayout = new GridLayout();
      bottomLayout.marginWidth = 0;
      bottomLayout.marginHeight = 4;
      bottomLayout.verticalSpacing = 0;
      bottomArea.setLayout(bottomLayout);
      bottomArea.setLayoutData(new GridData(SWT.FILL, SWT.END, true, false));
      bottomArea.setBackground(backgroundColor);

      if (pinboardPerspective != null)
      {
         pinboardItem = new PerspectiveItemComposite(bottomArea, pinboardPerspective);
         pinboardItem.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }

      // Toggle button
      Composite toggleButton = new Composite(bottomArea, SWT.NONE);
      GridLayout toggleLayout = new GridLayout();
      toggleLayout.marginWidth = 0;
      toggleLayout.marginHeight = 2;
      toggleButton.setLayout(toggleLayout);
      toggleButton.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      toggleButton.setBackground(backgroundColor);

      Label toggleLabel = new Label(toggleButton, SWT.CENTER);
      toggleLabel.setText(expanded ? "«" : "»");
      toggleLabel.setFont(toggleFont);
      toggleLabel.setForeground(sectionHeaderForeground);
      toggleLabel.setBackground(backgroundColor);
      toggleLabel.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, false));
      toggleLabel.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      toggleLabel.addMouseListener(new MouseAdapter() {
         @Override
         public void mouseUp(MouseEvent e)
         {
            toggle();
         }
      });
      toggleButton.addMouseListener(new MouseAdapter() {
         @Override
         public void mouseUp(MouseEvent e)
         {
            toggle();
         }
      });

      updateScrolledContentSize();
   }

   /**
    * Update scrolled content size after layout changes.
    */
   private void updateScrolledContentSize()
   {
      if (scrollContent != null && !scrollContent.isDisposed())
      {
         int width = expanded ? EXPANDED_WIDTH : COLLAPSED_WIDTH;
         Point size = scrollContent.computeSize(width, SWT.DEFAULT);
         scrollContent.setSize(size);
         scroller.setMinSize(size);
      }
   }

   /**
    * Toggle between expanded and collapsed modes.
    */
   public void toggle()
   {
      expanded = !expanded;
      PreferenceStore.getInstance().set("PerspectiveSwitcher.Expanded", expanded);
      rebuild();
   }

   /**
    * Rebuild all content after mode change.
    */
   private void rebuild()
   {
      allItems.clear();
      pinboardItem = null;

      // Dispose existing children
      scroller.dispose();
      bottomArea.dispose();
      for(org.eclipse.swt.widgets.Control c : getChildren())
      {
         c.dispose();
      }

      createContent();
      updateSelection();
      layout(true, true);

      // Notify parent to relayout since our width changed
      getParent().layout(true, true);
   }

   /**
    * Set the selected perspective and update visual state.
    *
    * @param p perspective to mark as selected
    */
   public void setSelectedPerspective(Perspective p)
   {
      this.selectedPerspective = p;
      updateSelection();
   }

   /**
    * Update visual selection state on all items.
    */
   private void updateSelection()
   {
      for(PerspectiveItemComposite item : allItems)
      {
         item.updateSelection(selectedPerspective);
      }
      if (pinboardItem != null)
      {
         pinboardItem.updateSelection(selectedPerspective);
      }
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      int width = expanded ? EXPANDED_WIDTH : COLLAPSED_WIDTH;
      Point size = super.computeSize(width, hHint, changed);
      size.x = width;
      return size;
   }

   /**
    * Inner widget representing a single perspective item in the sidebar. Paints its own rounded selection/hover highlight and the
    * perspective name (centered, up to two lines) below the icon.
    */
   private class PerspectiveItemComposite extends Canvas
   {
      private Perspective perspective;
      private SVGCanvas icon;
      private String[] textLines;
      private boolean selected = false;
      private boolean hovered = false;

      PerspectiveItemComposite(Composite parent, Perspective perspective)
      {
         super(parent, SWT.DOUBLE_BUFFERED);
         this.perspective = perspective;

         setBackground(backgroundColor);

         icon = new SVGCanvas(this, SWT.NONE);
         icon.setBackground(backgroundColor);
         icon.setDefaultColor(foregroundColor);
         icon.setImageResource(perspective.getImagePath());

         if (expanded)
            textLines = splitText(perspective.getName());

         // Tooltip
         KeyStroke shortcut = perspective.getKeyboardShortcut();
         String tooltip = (shortcut != null) ? perspective.getName() + "\t" + shortcut.toString() : perspective.getName();
         setToolTipText(tooltip);
         icon.setToolTipText(tooltip);

         addPaintListener((e) -> paint(e.gc));
         addListener(SWT.Resize, (e) -> layoutIcon());

         MouseAdapter clickListener = new MouseAdapter() {
            @Override
            public void mouseUp(MouseEvent e)
            {
               switchCallback.accept(PerspectiveItemComposite.this.perspective);
            }
         };
         MouseTrackAdapter hoverListener = new MouseTrackAdapter() {
            @Override
            public void mouseEnter(MouseEvent e)
            {
               if (!selected)
                  setHovered(true);
            }

            @Override
            public void mouseExit(MouseEvent e)
            {
               setHovered(false);
            }
         };

         addMouseListener(clickListener);
         addMouseTrackListener(hoverListener);
         icon.addMouseListener(clickListener);
         icon.addMouseTrackListener(hoverListener);

         setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
         icon.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      }

      /**
       * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
       */
      @Override
      public Point computeSize(int wHint, int hHint, boolean changed)
      {
         int width = (wHint != SWT.DEFAULT) ? wHint : (expanded ? EXPANDED_WIDTH : COLLAPSED_WIDTH);
         return new Point(width, expanded ? ITEM_HEIGHT_EXPANDED : ITEM_HEIGHT_COLLAPSED);
      }

      /**
       * Position the icon within the item.
       */
      private void layoutIcon()
      {
         Point size = getSize();
         int iconSize = expanded ? ICON_SIZE_EXPANDED : ICON_SIZE_COLLAPSED;
         int x = (size.x - iconSize) / 2;
         int y = expanded ? ITEM_PADDING_TOP : (size.y - iconSize) / 2;
         icon.setBounds(x, y, iconSize, iconSize);
      }

      /**
       * Paint highlight and text.
       */
      private void paint(GC gc)
      {
         Point size = getSize();

         gc.setBackground(backgroundColor);
         gc.fillRectangle(0, 0, size.x, size.y);

         if (selected || hovered)
         {
            gc.setAntialias(SWT.ON);
            gc.setBackground(selected ? selectionBackground : hoverBackground);
            if (expanded)
            {
               gc.fillRoundRectangle(HIGHLIGHT_MARGIN_H, HIGHLIGHT_MARGIN_V, size.x - 2 * HIGHLIGHT_MARGIN_H,
                     size.y - 2 * HIGHLIGHT_MARGIN_V, HIGHLIGHT_RADIUS, HIGHLIGHT_RADIUS);
            }
            else
            {
               int x = (size.x - HIGHLIGHT_COLLAPSED_SIZE) / 2;
               int y = (size.y - HIGHLIGHT_COLLAPSED_SIZE) / 2;
               gc.fillRoundRectangle(x, y, HIGHLIGHT_COLLAPSED_SIZE, HIGHLIGHT_COLLAPSED_SIZE, HIGHLIGHT_RADIUS, HIGHLIGHT_RADIUS);
            }
         }

         if (expanded && (textLines != null))
         {
            gc.setFont(itemFont);
            gc.setForeground(selected ? selectionForeground : foregroundColor);
            int lineHeight = gc.getFontMetrics().getHeight();
            int y = ITEM_PADDING_TOP + ICON_SIZE_EXPANDED + TEXT_GAP;
            for(String line : textLines)
            {
               Point extent = gc.textExtent(line);
               gc.drawText(line, (size.x - extent.x) / 2, y, SWT.DRAW_TRANSPARENT);
               y += lineHeight;
            }
         }
      }

      /**
       * Split perspective name into at most two centered lines that fit the item width.
       */
      private String[] splitText(String text)
      {
         GC gc = new GC(this);
         gc.setFont(itemFont);
         int maxWidth = EXPANDED_WIDTH - 2 * HIGHLIGHT_MARGIN_H - 4;
         if (gc.textExtent(text).x <= maxWidth)
         {
            gc.dispose();
            return new String[] { text };
         }

         String[] words = text.split(" ");
         if (words.length < 2)
         {
            gc.dispose();
            return new String[] { text };
         }

         StringBuilder line1 = new StringBuilder();
         int i = 0;
         for(; i < words.length; i++)
         {
            String candidate = (line1.length() == 0) ? words[i] : line1 + " " + words[i];
            if ((gc.textExtent(candidate).x > maxWidth) && (line1.length() > 0))
               break;
            line1.setLength(0);
            line1.append(candidate);
         }
         gc.dispose();

         if (i >= words.length)
            return new String[] { line1.toString() };

         StringBuilder line2 = new StringBuilder();
         for(; i < words.length; i++)
         {
            if (line2.length() > 0)
               line2.append(' ');
            line2.append(words[i]);
         }
         return new String[] { line1.toString(), line2.toString() };
      }

      /**
       * Update selection state.
       */
      void updateSelection(Perspective currentPerspective)
      {
         boolean isSelected = (currentPerspective != null) && currentPerspective.getId().equals(perspective.getId());
         if (this.selected == isSelected)
            return;

         this.selected = isSelected;
         if (isSelected)
            this.hovered = false;
         applyState();
      }

      /**
       * Set hover state.
       */
      private void setHovered(boolean hovered)
      {
         if (this.hovered == hovered)
            return;

         this.hovered = hovered;
         applyState();
      }

      /**
       * Apply visual state (icon color/background and repaint) based on current selection and hover state.
       */
      private void applyState()
      {
         Color highlight = selected ? selectionBackground : (hovered ? hoverBackground : backgroundColor);
         icon.setBackground(highlight);
         icon.setDefaultColor(selected ? selectionForeground : foregroundColor);
         icon.redraw();
         redraw();
      }
   }
}
