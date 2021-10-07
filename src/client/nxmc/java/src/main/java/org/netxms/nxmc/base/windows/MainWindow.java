/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.window.ApplicationWindow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.preferencepages.Appearance;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Main window
 */
public class MainWindow extends ApplicationWindow implements MessageAreaHolder
{
   private static Logger logger = LoggerFactory.getLogger(MainWindow.class);
   private static I18n i18n = LocalizationHelper.getI18n(MainWindow.class);

   private Composite windowContent;
   private ToolBar mainMenu;
   private ToolBar toolsMenu;
   private MessageArea messageArea;
   private Composite perspectiveArea;
   private List<Perspective> perspectives;
   private Perspective currentPerspective;
   private Perspective pinboardPerspective;
   private boolean verticalLayout;

   /**
    * @param parentShell
    */
   public MainWindow(Shell parentShell)
   {
      super(parentShell);
      addStatusLine();
      verticalLayout = PreferenceStore.getInstance().getAsBoolean("Appearance.VerticalLayout", true);
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
            ps.set("MainWindow.CurrentPerspective", (currentPerspective != null) ? currentPerspective.getId() : "(none)");
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
      layout.verticalSpacing = 0;
      layout.numColumns = verticalLayout ? 2 : 1;
      windowContent.setLayout(layout);

      Composite menuArea = new Composite(windowContent, SWT.NONE);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = verticalLayout ? 1 : 2;
      menuArea.setLayout(layout);
      GridData gd = new GridData();
      if (verticalLayout)
      {
         gd.grabExcessVerticalSpace = true;
         gd.verticalAlignment = SWT.FILL;
         gd.verticalSpan = 2;
      }
      else
      {
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.FILL;
      }
      menuArea.setLayoutData(gd);

      mainMenu = new ToolBar(menuArea, SWT.FLAT | SWT.WRAP | SWT.RIGHT | (verticalLayout ? SWT.VERTICAL : SWT.HORIZONTAL));
      mainMenu.setFont(font);
      gd = new GridData();
      if (verticalLayout)
      {
         gd.grabExcessVerticalSpace = true;
         gd.verticalAlignment = SWT.FILL;
      }
      else
      {
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.FILL;
      }
      mainMenu.setLayoutData(gd);

      toolsMenu = new ToolBar(menuArea, SWT.FLAT | SWT.WRAP | SWT.RIGHT | (verticalLayout ? SWT.VERTICAL : SWT.HORIZONTAL));
      toolsMenu.setFont(font);

      ToolItem userMenu = new ToolItem(toolsMenu, SWT.PUSH);
      userMenu.setImage(ResourceManager.getImage("icons/user-menu.png"));
      NXCSession session = Registry.getSession();
      if (verticalLayout)
         userMenu.setToolTipText(session.getUserName() + "@" + session.getServerName());
      else
         userMenu.setText(session.getUserName() + "@" + session.getServerName());

      ToolItem appPreferencesMenu = new ToolItem(toolsMenu, SWT.PUSH);
      appPreferencesMenu.setImage(ResourceManager.getImage("icons/preferences.png"));
      appPreferencesMenu.setToolTipText("Console preferences");
      appPreferencesMenu.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            showPreferences();
         }
      });

      messageArea = new MessageArea(windowContent, SWT.NONE);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      messageArea.setLayoutData(gd);

      perspectiveArea = new Composite(windowContent, SWT.NONE);
      gd = new GridData();
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

      final Display display = parent.getDisplay();
      display.addFilter(SWT.KeyDown, new Listener() {
         @Override
         public void handleEvent(Event e)
         {
            if (getShell() == display.getActiveShell()) // Only process keystrokes directed to this window
               processKeyDownEvent(e.stateMask, e.keyCode);
         }
      });

      switchToPerspective("Pinboard");
      switchToPerspective(PreferenceStore.getInstance().getAsString("MainWindow.CurrentPerspective"));

      String motd = session.getMessageOfTheDay();
      if ((motd != null) && !motd.isEmpty())
         addMessage(MessageArea.INFORMATION, session.getMessageOfTheDay());

      return windowContent;
   }

   /**
    * Process key down event
    *
    * @param e event to process
    */
   private void processKeyDownEvent(int stateMask, int keyCode)
   {
      if ((keyCode == SWT.SHIFT) || (keyCode == SWT.CTRL) || (keyCode == SWT.SHIFT) || (keyCode == SWT.ALT) || (keyCode == SWT.ALT_GR) || (keyCode == SWT.COMMAND))
         return; // Ignore key down on modifier keys

      KeyStroke ks = new KeyStroke(stateMask, keyCode);
      boolean processed = false;
      for(final Perspective p : perspectives)
      {
         if (ks.equals(p.getKeyboardShortcut()))
         {
            processed = true;
            getShell().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  switchToPerspective(p);
               }
            });
            break;
         }
      }
      if (!processed && (currentPerspective != null))
         currentPerspective.processKeyStroke(ks);
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
      perspectives = Registry.getInstance().getPerspectives();
      for(final Perspective p : perspectives)
      {
         p.bindToWindow(this);
         ToolItem item = new ToolItem(mainMenu, SWT.RADIO);
         item.setData("PerspectiveId", p.getId());
         item.setImage(p.getImage());
         if (!verticalLayout)
            item.setText(p.getName());
         KeyStroke shortcut = p.getKeyboardShortcut();
         item.setToolTipText((shortcut != null) ? p.getName() + "\t" + shortcut.toString() : p.getName());
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
      for(ToolItem item : mainMenu.getItems())
      {
         Object id = item.getData("PerspectiveId");
         if (id != null)
         {
            // This is perspective switcher item, set selection according to current perspective
            item.setSelection(p.getId().equals(id));
         }
      }
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
      logger.debug("Request to pin view with GlobalID=" + view.getGlobalId());
      pinboardPerspective.addMainView(view, false, true);
   }

   /**
    * Show console preferences
    */
   private void showPreferences()
   {
      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("appearance", new Appearance()));

      PreferenceDialog dlg = new PreferenceDialog(getShell(), pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText(i18n.tr("Console Preferences"));
         }
      };
      dlg.setBlockOnOpen(true);
      dlg.open();
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String)
    */
   @Override
   public int addMessage(int level, String text)
   {
      return messageArea.addMessage(level, text);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String, boolean)
    */
   @Override
   public int addMessage(int level, String text, boolean sticky)
   {
      return messageArea.addMessage(level, text);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#deleteMessage(int)
    */
   @Override
   public void deleteMessage(int id)
   {
      messageArea.deleteMessage(id);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#clearMessages()
    */
   @Override
   public void clearMessages()
   {
      messageArea.clearMessages();
   }
}
