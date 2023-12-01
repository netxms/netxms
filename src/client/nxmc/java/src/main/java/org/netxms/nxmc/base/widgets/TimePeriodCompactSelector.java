/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import java.util.HashSet;
import java.util.Set;
import org.apache.commons.lang3.SystemUtils;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.List;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.TimePeriod;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Compact selector for time period. Displays time period description with drop down button to open full size selector.
 */
public class TimePeriodCompactSelector extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(TimePeriodSelector.class);
   
   private static final TimePeriod[] PRESETS = {
      new TimePeriod(TimeFrameType.BACK_FROM_NOW, 1, TimeUnit.HOUR, null, null),
      new TimePeriod(TimeFrameType.BACK_FROM_NOW, 4, TimeUnit.HOUR, null, null),
      new TimePeriod(TimeFrameType.BACK_FROM_NOW, 12, TimeUnit.HOUR, null, null),
      new TimePeriod(TimeFrameType.BACK_FROM_NOW, 1, TimeUnit.DAY, null, null),
      new TimePeriod(TimeFrameType.BACK_FROM_NOW, 7, TimeUnit.DAY, null, null),
      new TimePeriod(TimeFrameType.BACK_FROM_NOW, 30, TimeUnit.DAY, null, null),
      new TimePeriod(TimeFrameType.BACK_FROM_NOW, 90, TimeUnit.DAY, null, null)
   };

   private TimePeriod timePeriod;
   private Button description;
   private Shell selectorShell;
   private Set<SelectionListener> selectionListeners = new HashSet<>();

   /**
    * Create selector widget.
    *
    * @param parent parent composite
    * @param style widget style
    */
   public TimePeriodCompactSelector(Composite parent, int style)
   {
      this(parent, style, new TimePeriod(TimeFrameType.BACK_FROM_NOW, 1, TimeUnit.HOUR, null, null));
   }

   /**
    * Create selector widget.
    *
    * @param parent parent composite
    * @param style widget style
    * @param initialTimePeriod initial time period to display
    */
   public TimePeriodCompactSelector(Composite parent, int style, TimePeriod initialTimePeriod)
   {
      super(parent, style);

      timePeriod = initialTimePeriod;

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      setLayout(layout);

      description = new Button(this, SWT.PUSH | SWT.FLAT);
      description.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      description.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      description.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if (selectorShell == null)
            {
               showSelector();
            }
            else
            {
               selectorShell.dispose();
            }
         }
      });

      updateDescription();
   }

   /**
    * Show selector window
    */
   private void showSelector()
   {
      selectorShell = new Shell(getShell(), SWT.NO_TRIM);
      selectorShell.setLayout(new FillLayout());

      selectorShell.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            selectorShell = null;
            updateDescription();
         }
      });

      Composite content = new Composite(selectorShell, SWT.BORDER);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      content.setLayout(layout);

      Label presetsLabel = new Label(content, SWT.NONE);
      presetsLabel.setText(i18n.tr("Presets"));

      TimePeriodSelector selector = new TimePeriodSelector(content, SWT.VERTICAL, timePeriod);
      selector.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true, 1, 2));

      List presetList = new List(content, SWT.BORDER | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.widthHint = 200;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      presetList.setLayoutData(gd);

      for(TimePeriod p : PRESETS)
         presetList.add(describeTimePeriod(p));

      presetList.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            selector.setTimePeriod(PRESETS[presetList.getSelectionIndex()]);
         }
      });
      presetList.addMouseListener(new MouseAdapter() {
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            timePeriod = PRESETS[presetList.getSelectionIndex()];
            selectorShell.dispose();
            fireSelectionListeners();
         }
      });

      Composite buttonBar = new Composite(content, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      buttonBar.setLayout(layout);
      gd = new GridData(SWT.RIGHT, SWT.FILL, true, false, 2, 1);
      gd.verticalIndent = 6;
      buttonBar.setLayoutData(gd);

      Button buttonApply, buttonCancel;
      if (Registry.IS_WEB_CLIENT || SystemUtils.IS_OS_WINDOWS)
      {
         buttonApply = new Button(buttonBar, SWT.PUSH | SWT.DEFAULT);
         buttonCancel = new Button(buttonBar, SWT.PUSH);
      }
      else
      {
         buttonCancel = new Button(buttonBar, SWT.PUSH);
         buttonApply = new Button(buttonBar, SWT.PUSH | SWT.DEFAULT);
      }
      selectorShell.setDefaultButton(buttonApply);

      buttonApply.setText(i18n.tr("&Apply"));
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonApply.setLayoutData(gd);
      buttonApply.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            timePeriod = selector.getTimePeriod();
            selectorShell.dispose();
            fireSelectionListeners();
         }
      });

      buttonCancel.setText(i18n.tr("Cancel"));
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonCancel.setLayoutData(gd);
      buttonCancel.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            selectorShell.dispose();
         }
      });

      selectorShell.pack();

      Point shellSize = selectorShell.getSize();
      Rectangle displayBounds = getDisplay().getBounds();
      Point l = getLocation();
      l.y += getSize().y + 1;
      l = getDisplay().map(getParent(), null, l);
      if (l.x + shellSize.x >= displayBounds.width)
         l.x = displayBounds.width - shellSize.x - 1;
      if (l.y + shellSize.y >= displayBounds.height)
         l.y = displayBounds.height - shellSize.y - 1;
      selectorShell.setLocation(l);
      
      updateDescription();
      selectorShell.open();
   }

   /**
    * Update description field
    */
   private void updateDescription()
   {
      if (description.isDisposed())
         return;
      description.setText(describeTimePeriod(timePeriod) + ((selectorShell != null) ? "  \u25b4" : "  \u25be"));
      getParent().layout();
   }

   /**
    * Describe given time period.
    *
    * @param p time period to describe
    * @return time period description
    */
   private String describeTimePeriod(TimePeriod p)
   {
      StringBuilder sb = new StringBuilder();
      if (p.isBackFromNow())
      {
         sb.append(i18n.tr("Last "));
         if (p.getTimeRange() == 1)
         {
            switch(p.getTimeUnit())
            {
               case DAY:
                  sb.append(i18n.tr("day"));
                  break;
               case HOUR:
                  sb.append(i18n.tr("hour"));
                  break;
               case MINUTE:
                  sb.append(i18n.tr("minute"));
                  break;
            }
         }
         else if ((p.getTimeRange() == 7) && (p.getTimeUnit() == TimeUnit.DAY))
         {
            sb.append(i18n.tr(" week"));
         }
         else
         {
            sb.append(p.getTimeRange());
            switch(p.getTimeUnit())
            {
               case DAY:
                  sb.append(i18n.tr(" days"));
                  break;
               case HOUR:
                  sb.append(i18n.tr(" hours"));
                  break;
               case MINUTE:
                  sb.append(i18n.tr(" minutes"));
                  break;
            }
         }
      }
      else
      {
         sb.append(DateFormatFactory.getDateTimeFormat().format(p.getPeriodStart()));
         sb.append(" \u2013 ");
         sb.append(DateFormatFactory.getDateTimeFormat().format(p.getPeriodEnd()));
      }
      return sb.toString();
   }

   /**
    * @return the timePeriod
    */
   public TimePeriod getTimePeriod()
   {
      return timePeriod;
   }

   /**
    * @param timePeriod the timePeriod to set
    */
   public void setTimePeriod(TimePeriod timePeriod)
   {
      this.timePeriod = timePeriod;
      updateDescription();
   }

   /**
    * Add selection listener.
    *
    * @param listener selection listener
    */
   public void addSelectionListener(SelectionListener listener)
   {
      selectionListeners.add(listener);
   }

   /**
    * Remove selection listener.
    *
    * @param listener selection listener
    */
   public void removeSelectionListener(SelectionListener listener)
   {
      selectionListeners.remove(listener);
   }

   /**
    * Fire selection listeners
    */
   private void fireSelectionListeners()
   {
      Event e = new Event();
      e.display = getDisplay();
      e.doit = true;
      e.widget = this;
      SelectionEvent se = new SelectionEvent(e);
      for(SelectionListener l : selectionListeners)
         l.widgetSelected(se);
   }
}
