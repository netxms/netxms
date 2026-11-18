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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ITreeSelection;
import org.eclipse.jface.viewers.ITreeViewerListener;
import org.eclipse.jface.viewers.TreeExpansionEvent;
import org.eclipse.jface.viewers.TreePath;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;

/**
 * Tree viewer for data collection views. Group nodes ({@link DciGroup}) are excluded from reported selection, so actions operate
 * only on data collection elements. When viewer input is replaced on refresh, existing tree items are updated in place, so scroll
 * position and expansion state of groups are preserved.
 */
public class DciTreeViewer extends SortableTreeViewer
{
   /**
    * Create viewer.
    *
    * @param parent parent composite
    * @param names column names
    * @param widths default column widths
    * @param defaultSortingColumn index of default sorting column
    * @param defaultSortDir default sorting direction
    * @param style widget style
    * @param configPrefix preference store prefix for persisting column settings (may be null)
    */
   public DciTreeViewer(Composite parent, String[] names, int[] widths, int defaultSortingColumn, int defaultSortDir, int style, String configPrefix)
   {
      super(parent, names, widths, defaultSortingColumn, defaultSortDir, style, configPrefix);
      addDoubleClickListener((e) -> {
         Object element = getTree().getSelectionCount() == 1 ? getTree().getSelection()[0].getData() : null;
         if (element instanceof DciGroup)
         {
            boolean expand = !getExpandedState(element);
            setExpandedState(element, expand);
            if (expand)
               packAfterExpand(); // programmatic expansion does not notify tree listeners
         }
      });
      addTreeListener(new ITreeViewerListener() {
         @Override
         public void treeExpanded(TreeExpansionEvent event)
         {
            packAfterExpand();
         }

         @Override
         public void treeCollapsed(TreeExpansionEvent event)
         {
         }
      });
   }

   /**
    * Pack columns after group expansion if automatic resize is enabled. Indentation of group elements is not reflected in text
    * length used by automatic resize, so expanded group may need wider first column. Child items are created after expand event,
    * so pack is done asynchronously.
    */
   private void packAfterExpand()
   {
      if (!isAutoResizeEnabled())
         return;
      getControl().getDisplay().asyncExec(() -> {
         if (!getControl().isDisposed())
            packColumns();
      });
   }

   /**
    * @see org.eclipse.jface.viewers.AbstractTreeViewer#getSelection()
    */
   @Override
   public ISelection getSelection()
   {
      ISelection selection = super.getSelection();
      if (!(selection instanceof ITreeSelection) || selection.isEmpty())
         return selection;

      TreePath[] paths = ((ITreeSelection)selection).getPaths();
      List<TreePath> elementPaths = new ArrayList<>(paths.length);
      for(TreePath p : paths)
      {
         if (!(p.getLastSegment() instanceof DciGroup))
            elementPaths.add(p);
      }
      return (elementPaths.size() == paths.length) ? selection : new TreeSelection(elementPaths.toArray(new TreePath[elementPaths.size()]), getComparer());
   }

   /**
    * @see org.netxms.nxmc.base.widgets.SortableTreeViewer#inputChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void inputChanged(Object input, Object oldInput)
   {
      if ((oldInput == null) || (getTree().getItemCount() == 0))
      {
         super.inputChanged(input, oldInput);
         return;
      }

      // Default implementation re-creates all tree items, which resets scroll position and collapses groups. Refresh updates
      // existing items in place instead, keeping scroll position and expansion state of groups (groups with same key are equal).
      getControl().setData(input);
      refresh();
   }
}
