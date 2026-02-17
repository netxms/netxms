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
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveSeparator;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.resources.ThemeEngine;

/**
 * Custom perspective switcher sidebar widget. Supports expanded (icon + text) and collapsed (icon only) modes with section grouping
 * and scrolling.
 */
public class PerspectiveSwitcher extends Composite
{
   private static final int EXPANDED_WIDTH = 200;
   private static final int COLLAPSED_WIDTH = 48;
   private static final int ICON_SIZE_EXPANDED = 20;
   private static final int ICON_SIZE_COLLAPSED = 24;
   private static final int ITEM_HEIGHT = 36;
   private static final int SECTION_HEADER_HEIGHT = 28;
   private static final int SECTION_SPACING = 16;
   private static final int ACCENT_WIDTH = 3;

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
   private Font mainFont;
   private Font sectionHeaderFont;

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

      backgroundColor = ThemeEngine.getBackgroundColor("Window.PerspectiveSwitcher");
      foregroundColor = ThemeEngine.getForegroundColor("Window.PerspectiveSwitcher");
      selectionBackground = ThemeEngine.getBackgroundColor("Window.PerspectiveSwitcher.Selection");
      selectionForeground = ThemeEngine.getForegroundColor("Window.PerspectiveSwitcher.Selection");
      hoverBackground = ThemeEngine.getBackgroundColor("Window.PerspectiveSwitcher.Hover");
      sectionHeaderForeground = ThemeEngine.getForegroundColor("Window.PerspectiveSwitcher.SectionHeader");
      mainFont = ThemeEngine.getFont("Window.PerspectiveSwitcher");
      sectionHeaderFont = ThemeEngine.getFont("Window.PerspectiveSwitcher.SectionHeader");

      buildSections(perspectives);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      setLayout(layout);
      setBackground(backgroundColor);

      createContent();
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
      scroller.getVerticalBar().setIncrement(ITEM_HEIGHT);

      scrollContent = new Composite(scroller, SWT.NONE);
      GridLayout scrollLayout = new GridLayout();
      scrollLayout.marginWidth = 0;
      scrollLayout.marginHeight = 4;
      scrollLayout.verticalSpacing = 0;
      scrollContent.setLayout(scrollLayout);
      scrollContent.setBackground(backgroundColor);

      boolean firstSection = true;
      for(String sectionName : sectionOrder)
      {
         List<Perspective> sectionPerspectives = sections.get(sectionName);

         if (!firstSection)
         {
            if (expanded && !sectionName.isEmpty())
            {
               // Add spacing before section header
               Label spacer = new Label(scrollContent, SWT.NONE);
               spacer.setBackground(backgroundColor);
               GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
               gd.heightHint = SECTION_SPACING;
               spacer.setLayoutData(gd);
            }
            else
            {
               // Collapsed mode or unnamed section: thin gap
               Label spacer = new Label(scrollContent, SWT.NONE);
               spacer.setBackground(backgroundColor);
               GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
               gd.heightHint = SECTION_SPACING;
               spacer.setLayoutData(gd);
            }
         }
         firstSection = false;

         // Section header (only in expanded mode with named sections)
         if (expanded && !sectionName.isEmpty())
         {
            Label header = new Label(scrollContent, SWT.NONE);
            header.setText(sectionName.toUpperCase());
            header.setFont(sectionHeaderFont);
            header.setForeground(sectionHeaderForeground);
            header.setBackground(backgroundColor);
            GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
            gd.heightHint = SECTION_HEADER_HEIGHT;
            gd.horizontalIndent = ACCENT_WIDTH + 8;
            header.setLayoutData(gd);
         }

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
      toggleLabel.setText(expanded ? "\u00AB" : "\u00BB");
      toggleLabel.setFont(mainFont);
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
    * Inner composite representing a single perspective item in the sidebar.
    */
   private class PerspectiveItemComposite extends Composite
   {
      private Perspective perspective;
      private Label accentBar;
      private SVGCanvas iconLabel;
      private Label textLabel;
      private boolean selected = false;
      private boolean hovered = false;

      PerspectiveItemComposite(Composite parent, Perspective perspective)
      {
         super(parent, SWT.NONE);
         this.perspective = perspective;

         setBackground(backgroundColor);

         GridLayout layout = new GridLayout();
         layout.marginWidth = 0;
         layout.marginHeight = 0;
         layout.marginTop = 2;
         layout.marginBottom = 2;
         layout.horizontalSpacing = 0;
         layout.verticalSpacing = 0;

         if (expanded)
         {
            layout.numColumns = 3; // accent + icon + text
         }
         else
         {
            layout.numColumns = 1; // icon only (centered)
         }
         setLayout(layout);

         int iconSize = expanded ? ICON_SIZE_EXPANDED : ICON_SIZE_COLLAPSED;
         if (expanded)
         {
            // Accent bar
            accentBar = new Label(this, SWT.NONE);
            accentBar.setBackground(backgroundColor);
            GridData gd = new GridData(SWT.LEFT, SWT.FILL, false, true);
            gd.widthHint = ACCENT_WIDTH;
            gd.heightHint = ITEM_HEIGHT - 4;
            accentBar.setLayoutData(gd);

            // Icon
            iconLabel = new SVGCanvas(this, SWT.NONE);
            gd = new GridData(SWT.CENTER, SWT.CENTER, false, false);
            gd.widthHint = iconSize + 8;
            gd.heightHint = ITEM_HEIGHT - 6;
            gd.horizontalIndent = 4;
            iconLabel.setLayoutData(gd);

            // Text
            textLabel = new Label(this, SWT.NONE);
            textLabel.setText(perspective.getName());
            textLabel.setFont(mainFont);
            textLabel.setForeground(foregroundColor);
            textLabel.setBackground(backgroundColor);
            gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
            gd.horizontalIndent = 6;
            textLabel.setLayoutData(gd);
         }
         else
         {
            // Icon only, centered
            iconLabel = new SVGCanvas(this, SWT.NONE);
            GridData gd = new GridData(SWT.CENTER, SWT.CENTER, true, false);
            gd.widthHint = iconSize + 8;
            gd.heightHint = ITEM_HEIGHT - 4;
            iconLabel.setLayoutData(gd);
         }

         iconLabel.setBackground(backgroundColor);
         iconLabel.setDefaultColor(foregroundColor);
         iconLabel.setImage(perspective.getImage());

         // Tooltip
         KeyStroke shortcut = perspective.getKeyboardShortcut();
         String tooltip = (shortcut != null) ? perspective.getName() + "\t" + shortcut.toString() : perspective.getName();
         setToolTipText(tooltip);
         iconLabel.setToolTipText(tooltip);
         if (textLabel != null)
            textLabel.setToolTipText(tooltip);

         // Mouse listeners on all child controls
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
         iconLabel.addMouseListener(clickListener);
         iconLabel.addMouseTrackListener(hoverListener);
         if (textLabel != null)
         {
            textLabel.addMouseListener(clickListener);
            textLabel.addMouseTrackListener(hoverListener);
         }
         if (accentBar != null)
         {
            accentBar.addMouseListener(clickListener);
            accentBar.addMouseTrackListener(hoverListener);
         }

         setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
         iconLabel.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
         if (textLabel != null)
            textLabel.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
         if (accentBar != null)
            accentBar.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
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
         applyColors();
      }

      /**
       * Set hover state.
       */
      private void setHovered(boolean hovered)
      {
         if (this.hovered == hovered)
            return;

         this.hovered = hovered;
         applyColors();
      }

      /**
       * Apply colors based on current state.
       */
      private void applyColors()
      {
         Color bg;
         Color fg;

         if (selected)
         {
            bg = selectionBackground;
            fg = selectionForeground;
         }
         else if (hovered)
         {
            bg = hoverBackground;
            fg = foregroundColor;
         }
         else
         {
            bg = backgroundColor;
            fg = foregroundColor;
         }

         setBackground(bg);
         iconLabel.setBackground(bg);
         if (textLabel != null)
         {
            textLabel.setBackground(bg);
            textLabel.setForeground(fg);
         }
         if (accentBar != null)
         {
            accentBar.setBackground(selected ? selectionForeground : bg);
         }
      }
   }
}
