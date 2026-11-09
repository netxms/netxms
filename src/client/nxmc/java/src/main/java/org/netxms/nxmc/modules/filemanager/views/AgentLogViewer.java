/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.nxmc.modules.filemanager.views;

import java.io.File;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.widgets.BaseFileViewer;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Viewer for the tail of agent's own log file
 */
public class AgentLogViewer extends AdHocObjectView
{
   private static final long SHOW_LIMIT = 1048576; // 1 MB

   private final I18n i18n = LocalizationHelper.getI18n(AgentLogViewer.class);

   private BaseFileViewer viewer;
   private Action actionCopy;
   private Action actionSelectAll;
   private Action actionFind;

   /**
    * Create agent log viewer for given node.
    *
    * @param node node object
    * @param contextId context object ID
    */
   public AgentLogViewer(Node node, long contextId)
   {
      super(LocalizationHelper.getI18n(AgentLogViewer.class).tr("Agent log - {0}", node.getObjectName()), ResourceManager.getImageDescriptor("icons/object-views/file-view.png"),
            "AgentLogViewer", node.getObjectId(), contextId, false);
   }

   /**
    * Create view for cloning or restore.
    */
   protected AgentLogViewer()
   {
      super(LocalizationHelper.getI18n(AgentLogViewer.class).tr("Agent log"), ResourceManager.getImageDescriptor("icons/object-views/file-view.png"), "AgentLogViewer", 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View origin)
   {
      super.postClone(origin);
      viewer.replaceContent(((AgentLogViewer)origin).viewer, true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      viewer = new BaseFileViewer(parent, SWT.NONE, this);
      viewer.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            actionCopy.setEnabled(viewer.canCopy());
         }
      });

      createActions();
      createPopupMenu();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCopy = new Action(i18n.tr("&Copy")) {
         @Override
         public void run()
         {
            viewer.copy();
         }
      };
      actionCopy.setEnabled(false);
      addKeyBinding("Ctrl+C", actionCopy);

      actionSelectAll = new Action(i18n.tr("Select &all")) {
         @Override
         public void run()
         {
            viewer.selectAll();
         }
      };
      addKeyBinding("Ctrl+A", actionSelectAll);

      actionFind = new Action(i18n.tr("&Find"), SharedIcons.FIND) {
         @Override
         public void run()
         {
            viewer.showSearchBar();
         }
      };
      addKeyBinding("Ctrl+F", actionFind);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionFind);
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));
      Menu menu = menuMgr.createContextMenu(viewer.getTextControl());
      viewer.getTextControl().setMenu(menu);
   }

   /**
    * Fill context menu
    *
    * @param manager menu manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionFind);
      manager.add(new Separator());
      manager.add(actionSelectAll);
      manager.add(actionCopy);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Downloading agent log"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            File file = session.downloadAgentLogFile(getObjectId(), SHOW_LIMIT);
            boolean truncated = file.length() >= SHOW_LIMIT;
            String text = BaseFileViewer.loadFile(file);
            file.delete();
            // Tail of truncated log starts at arbitrary byte, so first line is incomplete
            int lineEnd = truncated ? text.indexOf('\n') : -1;
            final String content = (lineEnd >= 0) ? text.substring(lineEnd + 1) : text;
            runInUIThread(() -> {
               if (viewer.isDisposed())
                  return;
               viewer.showContent(content, true);
               String nodeName = getObjectName();
               if (!nodeName.isEmpty())
                  setName(i18n.tr("Agent log - {0}", nodeName));
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot download agent log from node {0}", getObjectName());
         }
      }.start();
   }
}
