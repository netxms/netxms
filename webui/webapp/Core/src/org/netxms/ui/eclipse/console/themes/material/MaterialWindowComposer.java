/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Raden Solutions
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
package org.netxms.ui.eclipse.console.themes.material;

import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.window.ApplicationWindow;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.ui.interactiondesign.IWindowComposer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.netxms.ui.eclipse.console.Activator;

/**
 * Window composer for "Material" theme
 */
public class MaterialWindowComposer implements IWindowComposer
{
   private Shell shell;
   private ApplicationWindow window;
   private Label logo;
   private Composite header;
   private Composite menuBar;
   
   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.interactiondesign.IWindowComposer#createWindowContents(org.eclipse.swt.widgets.Shell, org.eclipse.ui.application.IWorkbenchWindowConfigurer)
    */
   @Override
   public Composite createWindowContents(Shell shell, IWorkbenchWindowConfigurer configurer)
   {
      this.shell = shell;
      window = (ApplicationWindow)PlatformUI.getWorkbench().getActiveWorkbenchWindow();
      
      shell.setLayout(new FormLayout());
      
      createLogo();
      createHeader();
      if (configurer.getShowMenuBar())
         createMenuBar();
      
      Composite page = new Composite(shell, SWT.NONE);
      FormData fd = new FormData();
      fd.top = new FormAttachment(configurer.getShowMenuBar() ? menuBar : header, 0, SWT.BOTTOM);
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      page.setLayoutData(fd);
      page.setLayout(new FillLayout());
      
      return page;
   }
   
   /**
    * Create logo
    */
   private void createLogo()
   {
      logo = new Label(shell, SWT.NONE);
      final Image image = Activator.getImageDescriptor("icons/logo.png").createImage();
      logo.setImage(image);
      logo.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent event)
         {
            image.dispose();
         }
      });

      FormData fd = new FormData();
      fd.top = new FormAttachment(0, 0);
      fd.left = new FormAttachment(0, 0);
      logo.setLayoutData(fd);
   }
   
   /**
    * Create header
    */
   private void createHeader()
   {
      header = new Composite(shell, SWT.NONE);
      header.setData(RWT.CUSTOM_VARIANT, "header"); //$NON-NLS-1$
      FormData fd = new FormData();
      fd.top = new FormAttachment(0, 0);
      fd.left = new FormAttachment(logo, 0, SWT.RIGHT);
      fd.right = new FormAttachment(100, 0);
      header.setLayoutData(fd);
      header.setLayout(new FillLayout());
      
      new Label(header, SWT.NONE).setText("HEADER");
   }
   
   /**
    * Create menu bar
    */
   private void createMenuBar()
   {
      menuBar = new Composite(shell, SWT.NONE);
      menuBar.setData(RWT.CUSTOM_VARIANT, "menuBar"); //$NON-NLS-1$
      FormData fd = new FormData();
      fd.top = new FormAttachment(header, 0, SWT.BOTTOM);
      fd.left = new FormAttachment(logo, 0, SWT.RIGHT);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(logo, 0, SWT.BOTTOM);
      menuBar.setLayoutData(fd);
      
      RowLayout layout = new RowLayout();
      layout.marginLeft = 30;
      layout.marginRight = 0;
      layout.marginTop = 3;
      menuBar.setLayout(layout);
      
      MenuManager manager = window.getMenuBarManager();
      manager.fill(menuBar);
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.interactiondesign.IWindowComposer#postWindowOpen(org.eclipse.ui.application.IWorkbenchWindowConfigurer)
    */
   @Override
   public void postWindowOpen(IWorkbenchWindowConfigurer configurer)
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.interactiondesign.IWindowComposer#preWindowOpen(org.eclipse.ui.application.IWorkbenchWindowConfigurer)
    */
   @Override
   public void preWindowOpen(IWorkbenchWindowConfigurer configurer)
   {
      configurer.setShowCoolBar(false);
   }
}
