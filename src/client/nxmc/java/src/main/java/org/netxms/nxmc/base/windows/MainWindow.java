/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.nxmc.base.windows;

import java.util.List;
import org.eclipse.jface.window.ApplicationWindow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Main window
 */
public class MainWindow extends ApplicationWindow
{
   private static Logger logger = LoggerFactory.getLogger(MainWindow.class);

   private Composite windowContent;
   private ToolBar mainMenu;
   private ToolBar toolsMenu;
   private Composite perspectiveArea;
   private List<Perspective> perspectives;
   private Perspective currentPerspective;
   private Perspective pinboardPerspective;

   /**
    * @param parentShell
    */
   public MainWindow(Shell parentShell)
   {
      super(parentShell);
      addStatusLine();
   }

   /**
    * @see org.eclipse.jface.window.ApplicationWindow#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell shell)
   {
      super.configureShell(shell);
      shell.setText("NetXMS Management Console");

      PreferenceStore ps = PreferenceStore.getInstance();
      shell.setSize(ps.getAsPoint("MainWindow.Size", 600, 400));
      shell.setLocation(ps.getAsPoint("MainWindow.Location", 100, 100));
      shell.setMaximized(ps.getAsBoolean("MainWindow.Maximized", true));

      shell.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            PreferenceStore ps = PreferenceStore.getInstance();
            ps.set("MainWindow.Maximized", getShell().getMaximized());
            ps.set("MainWindow.Size", getShell().getSize());
            ps.set("MainWindow.Location", getShell().getLocation());
         }
      });
   }

   /**
    * @see org.eclipse.jface.window.Window#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Font font = ThemeEngine.getFont("TopMenu");

      windowContent = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      windowContent.setLayout(layout);

      mainMenu = new ToolBar(windowContent, SWT.FLAT | SWT.WRAP | SWT.RIGHT);
      mainMenu.setFont(font);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      mainMenu.setLayoutData(gd);

      toolsMenu = new ToolBar(windowContent, SWT.FLAT | SWT.WRAP | SWT.RIGHT);
      toolsMenu.setFont(font);

      ToolItem userMenu = new ToolItem(toolsMenu, SWT.PUSH);
      userMenu.setImage(ResourceManager.getImage("icons/user-menu.png"));
      NXCSession session = Registry.getSession();
      userMenu.setText(session.getUserName() + "@" + session.getServerName());

      Label separator = new Label(windowContent, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      separator.setLayoutData(gd);

      perspectiveArea = new Composite(windowContent, SWT.NONE);
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      perspectiveArea.setLayoutData(gd);

      perspectiveArea.addControlListener(new ControlListener() {
         @Override
         public void controlResized(ControlEvent e)
         {
            resizePerspectiveAreaContent();
         }

         @Override
         public void controlMoved(ControlEvent e)
         {
         }
      });

      setupPerspectiveSwitcher();

      return windowContent;
   }

   /**
    * Resize content of perspective area
    */
   private void resizePerspectiveAreaContent()
   {
      for(Control c : perspectiveArea.getChildren())
      {
         if (c.isVisible())
         {
            c.setSize(perspectiveArea.getSize());
            break;
         }
      }
   }

   /**
    * Setup perspective switcher
    */
   private void setupPerspectiveSwitcher()
   {
      ToolItem appMenu = new ToolItem(mainMenu, SWT.PUSH);
      appMenu.setImage(ResourceManager.getImage("icons/app-menu.png"));

      perspectives = Registry.getInstance().getPerspectives();
      for(final Perspective p : perspectives)
      {
         ToolItem item = new ToolItem(mainMenu, SWT.PUSH);
         item.setText(p.getName());
         item.setImage(p.getImage());
         item.addSelectionListener(new SelectionListener() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               switchToPerspective(p);
            }

            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               widgetSelected(e);
            }
         });
         if (p.getId().equals("Pinboard"))
            pinboardPerspective = p;
      }
   }

   /**
    * Switch to given perspective.
    *
    * @param p perspective to switch to
    */
   private void switchToPerspective(Perspective p)
   {
      logger.debug("Switching to perspective " + p.getName());
      if (currentPerspective != null)
         currentPerspective.hide();
      currentPerspective = p;
      currentPerspective.show(perspectiveArea);
      resizePerspectiveAreaContent();
   }

   /**
    * Switch to given perspective.
    *
    * @param id perspective ID
    */
   public void switchToPerspective(String id)
   {
      for(Perspective p : perspectives)
         if (p.getId().equals(id))
         {
            switchToPerspective(p);
            break;
         }
   }

   /**
    * Pin given view (add it to pinboard perspective).
    *
    * @param view view to pin
    */
   public void pinView(View view)
   {
      logger.debug("Request to pin view with ID=" + view.getId());
      view.globalizeId();
      pinboardPerspective.addMainView(view, true);
   }
}
