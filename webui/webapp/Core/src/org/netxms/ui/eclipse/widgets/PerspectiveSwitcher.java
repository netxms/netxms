/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.widgets;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CBanner;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.IPerspectiveDescriptor;
import org.eclipse.ui.IPerspectiveListener;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PlatformUI;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Custom perspective switcher
 */
public class PerspectiveSwitcher extends Composite
{
   private Label currentPerspective;
   private ToolBar toolbar;

   /**
    * @param window
    */
   public PerspectiveSwitcher(IWorkbenchWindow window)
   {
      super((Composite)window.getShell().getChildren()[0], SWT.NONE);
      ((CBanner)window.getShell().getChildren()[0]).setRight(this);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 10;
      setLayout(layout);

      currentPerspective = new Label(this, SWT.RIGHT);
      currentPerspective.setFont(JFaceResources.getBannerFont());
      IPerspectiveDescriptor p = window.getActivePage().getPerspective();
      if (p != null)
         currentPerspective.setText(p.getLabel());

      Menu menu = new Menu(currentPerspective);
      MenuItem mi = new MenuItem(menu, SWT.PUSH);
      mi.setText("&Reset perspective");
      mi.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            window.getActivePage().resetPerspective();
         }
      });
      currentPerspective.setMenu(menu);

      toolbar = new ToolBar(this, SWT.FLAT);

      ToolItem selectorButton = new ToolItem(toolbar, SWT.PUSH);
      selectorButton.setImage(Activator.getImageDescriptor("icons/perspectives-menu.png").createImage());
      final MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener(new IMenuListener() {
         @Override
         public void menuAboutToShow(IMenuManager manager)
         {
            for(final IPerspectiveDescriptor p : getVisiblePerspectives())
            {
               manager.add(new Action(p.getLabel(), p.getImageDescriptor()) {
                  @Override
                  public void run()
                  {
                     window.getActivePage().setPerspective(p);
                  }
               });
            }
         }
      });
      manager.createContextMenu(toolbar);
      selectorButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            manager.getMenu().setVisible(true);
         }
      });

      window.addPerspectiveListener(new IPerspectiveListener() {
         @Override
         public void perspectiveChanged(IWorkbenchPage page, IPerspectiveDescriptor perspective, String changeId)
         {
         }

         @Override
         public void perspectiveActivated(IWorkbenchPage page, IPerspectiveDescriptor perspective)
         {
            currentPerspective.setText(perspective.getLabel());
            window.getShell().layout(true, true);
         }
      });
   }

   /**
    * Get list of perspectives that should be visible, in correct order for display.
    *
    * @return list of perspectives that should be visible
    */
   public static List<IPerspectiveDescriptor> getVisiblePerspectives()
   {
      List<IPerspectiveDescriptor> visiblePerspectives = new ArrayList<IPerspectiveDescriptor>();

      List<String> enabledPerspectives;
      String v = ConsoleSharedData.getSession().getClientConfigurationHint("PerspectiveSwitcher.EnabledPerspectives");
      if ((v != null) && !v.isEmpty())
      {
         String[] parts = v.split(",");
         enabledPerspectives = new ArrayList<String>(parts.length);
         for(String s : parts)
         {
            if (!s.isBlank())
               enabledPerspectives.add(s.trim().toLowerCase());
         }
      }
      else
      {
         enabledPerspectives = null;
      }

      List<String> disabledPerspectives;
      v = ConsoleSharedData.getSession().getClientConfigurationHint("PerspectiveSwitcher.DisabledPerspectives");
      if ((v != null) && !v.isEmpty())
      {
         String[] parts = v.split(",");
         disabledPerspectives = new ArrayList<String>(parts.length);
         for(String s : parts)
         {
            if (!s.isBlank())
               disabledPerspectives.add(s.trim().toLowerCase());
         }
      }
      else
      {
         disabledPerspectives = null;
      }

      IPerspectiveDescriptor[] perspectives = PlatformUI.getWorkbench().getPerspectiveRegistry().getPerspectives();
      if (enabledPerspectives != null)
      {
         Arrays.sort(perspectives, new Comparator<IPerspectiveDescriptor>() {
            @Override
            public int compare(IPerspectiveDescriptor p1, IPerspectiveDescriptor p2)
            {
               return enabledPerspectives.indexOf(p1.getLabel().toLowerCase()) - enabledPerspectives.indexOf(p2.getLabel().toLowerCase());
            }
         });
      }

      String customizationId = ConsoleSharedData.getSession().getClientConfigurationHint("CustomizationId");

      for(final IPerspectiveDescriptor p : perspectives)
      {
         if (p.getLabel().startsWith("<") || p.getId().equals("org.netxms.ui.eclipse.console.SwitcherPerspective"))
            continue;
         if ((enabledPerspectives != null) && !enabledPerspectives.contains(p.getLabel().toLowerCase()))
            continue;
         if ((disabledPerspectives != null) && disabledPerspectives.contains(p.getLabel().toLowerCase()))
            continue;
         if (p.getId().startsWith("com.radensolutions.netxms.ui.perspective.custom.") &&
               ((customizationId == null) || !p.getId().startsWith("com.radensolutions.netxms.ui.perspective.custom." + customizationId + ".")))
            continue;
         visiblePerspectives.add(p);
      }

      return visiblePerspectives;
   }
}
