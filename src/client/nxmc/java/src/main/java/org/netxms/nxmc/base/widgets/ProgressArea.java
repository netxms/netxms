/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;

/**
 * Widget to display job progress inside a view
 */
public class ProgressArea extends Composite
{
   private Map<Integer, EmbeddedProgressMonitor> monitors = new HashMap<>();
   private Set<Integer> visibleMonitors = new HashSet<>();

   /**
    * Create progress area.
    *
    * @param parent parent composite
    * @param style style bits
    */
   public ProgressArea(Composite parent, int style)
   {
      super(parent, style);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      setLayout(layout);

      new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      addDisposeListener((e) -> monitors.clear());
   }

   /**
    * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      return visibleMonitors.isEmpty() ? new Point(0, 0) : super.computeSize(wHint, hHint, changed);
   }

   /**
    * Create progress monitor for given job.
    *
    * @param jobId ID of job to create monitor for
    * @return created progress monitor
    */
   public EmbeddedProgressMonitor createMonitor(final int jobId)
   {
      EmbeddedProgressMonitor monitor = new EmbeddedProgressMonitor(this, SWT.NONE, false);
      monitor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      monitors.put(jobId, monitor);
      getDisplay().timerExec(400, () -> {
         if (!monitor.isDisposed())
         {
            visibleMonitors.add(jobId);
            getParent().layout(true, true);
         }
      });
      return monitor;
   }

   /**
    * Remove progress monitor for given job
    *
    * @param jobId ID of job to remove monitor for
    */
   public void removeMonitor(int jobId)
   {
      EmbeddedProgressMonitor monitor = monitors.remove(jobId);
      if (monitor != null)
      {
         monitor.dispose();
         if (visibleMonitors.remove(jobId))
            getParent().layout(true, true);
      }
   }
}
