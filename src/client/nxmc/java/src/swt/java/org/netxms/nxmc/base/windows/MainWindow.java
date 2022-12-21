/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Layout;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.BrandingManager;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.Startup;
import org.netxms.nxmc.base.menus.HelpMenuManager;
import org.netxms.nxmc.base.menus.UserMenuManager;
import org.netxms.nxmc.base.preferencepages.Appearance;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.base.widgets.ServerClock;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.ExternalWebBrowser;
import org.netxms.nxmc.tools.FontTools;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Main window
 */
public class MainWindow extends Window implements MessageAreaHolder
{
   private static Logger logger = LoggerFactory.getLogger(MainWindow.class);
   private static I18n i18n = LocalizationHelper.getI18n(MainWindow.class);

   private Composite windowContent;
   private ToolBar mainMenu;
   private Composite headerArea;
   private MessageArea messageArea;
   private Composite perspectiveArea;
   private List<Perspective> perspectives;
   private Perspective currentPerspective;
   private Perspective pinboardPerspective;
   private boolean verticalLayout;
   private boolean showServerClock;
   private Composite serverClockArea;
   private ServerClock serverClock;
   private HeaderButton userMenuButton;
   private UserMenuManager userMenuManager;
   private HeaderButton helpMenuButton;
   private HelpMenuManager helpMenuManager;
   private Font headerFontBold;

   /**
    * @param parentShell
    */
   public MainWindow(Shell parentShell)
   {
      super(parentShell);
      PreferenceStore ps = PreferenceStore.getInstance();
      verticalLayout = ps.getAsBoolean("Appearance.VerticalLayout", true);
      showServerClock = ps.getAsBoolean("Appearance.ShowServerClock", false);
      userMenuManager = new UserMenuManager();
      helpMenuManager = new HelpMenuManager();
   }

   /**
    * @see org.eclipse.jface.window.ApplicationWindow#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell shell)
   {
      super.configureShell(shell);

      NXCSession session = Registry.getSession();
      shell.setText(String.format(i18n.tr("%s - %s"), BrandingManager.getClientProductName(), session.getUserName() + "@" + session.getServerAddress()));

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
            ps.set(PreferenceStore.serverProperty("MainWindow.CurrentPerspective"), (currentPerspective != null) ? currentPerspective.getId() : "(none)");
         }
      });

      shell.setImages(Startup.windowIcons);
   }

   /**
    * @see org.eclipse.jface.window.Window#getLayout()
    */
   @Override
   protected Layout getLayout()
   {
      return new FillLayout();
   }

   /**
    * @see org.eclipse.jface.window.Window#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      NXCSession session = Registry.getSession();

      Font perspectiveSwitcherFont = ThemeEngine.getFont("Window.PerspectiveSwitcher");
      Font headerFont = ThemeEngine.getFont("Window.Header");
      headerFontBold = FontTools.createAdjustedFont(headerFont, 2, SWT.BOLD);

      windowContent = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      layout.numColumns = verticalLayout ? 2 : 1;
      windowContent.setLayout(layout);

      // Header
      Color headerBackgroundColor = new Color(parent.getDisplay(), BrandingManager.getAppHeaderBackground());
      Color headerForegroundColor = ThemeEngine.getForegroundColor("Window.Header");

      headerArea = new Composite(windowContent, SWT.NONE);
      headerArea.setBackground(headerBackgroundColor);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = verticalLayout ? 2 : 1;
      headerArea.setLayoutData(gd);
      layout = new GridLayout();
      layout.marginWidth = 5;
      layout.marginHeight = 5;
      layout.numColumns = 14;
      headerArea.setLayout(layout);

      Label appLogo = new Label(headerArea, SWT.CENTER);
      appLogo.setBackground(headerBackgroundColor);
      appLogo.setImage(BrandingManager.getAppHeaderImage().createImage());
      gd = new GridData(SWT.LEFT, SWT.CENTER, false, false);
      gd.horizontalIndent = 8;
      appLogo.setLayoutData(gd);

      Label title = new Label(headerArea, SWT.LEFT);
      title.setBackground(headerBackgroundColor);
      title.setForeground(headerForegroundColor);
      title.setFont(headerFontBold);
      title.setText(BrandingManager.getProductName());

      Label filler = new Label(headerArea, SWT.CENTER);
      filler.setBackground(headerBackgroundColor);
      filler.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      serverClockArea = new Composite(headerArea, SWT.NONE);
      serverClockArea.setBackground(headerBackgroundColor);
      serverClockArea.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, true));
      serverClockArea.setLayout(new FillLayout());

      if (showServerClock)
      {
         createServerClockWidget();
      }

      new Spacer(headerArea, 32);

      Composite serverNameHolder = new Canvas(headerArea, SWT.NONE);
      FillLayout serverNameLayout = new FillLayout();
      serverNameLayout.marginHeight = 4;
      serverNameLayout.marginWidth = 4;
      serverNameHolder.setLayout(serverNameLayout);
      serverNameHolder.setBackground(headerBackgroundColor);

      Label serverName = new Label(serverNameHolder, SWT.CENTER);
      serverName.setBackground(headerBackgroundColor);
      serverName.setForeground(headerForegroundColor);
      serverName.setFont(headerFont);
      serverName.setText(session.getServerName());
      serverName.setToolTipText(i18n.tr("Server name"));
      RGB serverColor = ColorConverter.parseColorDefinition(session.getServerColor());
      if (serverColor != null)
      {
         final Color color = new Color(serverColor);
         serverName.setBackground(color);

         if (!ColorConverter.isDarkColor(serverColor))
         {
            serverName.setForeground(serverName.getDisplay().getSystemColor(SWT.COLOR_BLACK));
         }

         serverNameHolder.addPaintListener(new PaintListener() {
            @Override
            public void paintControl(PaintEvent e)
            {
               e.gc.setBackground(color);
               Rectangle r = serverNameHolder.getClientArea();
               e.gc.fillRoundRectangle(0, 0, r.width, r.height, 8, 8);
            }
         });
      }

      new Spacer(headerArea, 32);

      userMenuButton = new HeaderButton(headerArea, "icons/main-window/user.png", i18n.tr("User properties"), new Runnable() {
         @Override
         public void run()
         {
            Rectangle bounds = userMenuButton.getBounds();
            showMenu(userMenuManager, headerArea.toDisplay(new Point(bounds.x, bounds.y + bounds.height + 2)));
         }
      });

      Label userInfo = new Label(headerArea, SWT.LEFT);
      userInfo.setBackground(headerBackgroundColor);
      userInfo.setForeground(headerForegroundColor);
      userInfo.setFont(headerFont);
      userInfo.setText(session.getUserName() + "@" + session.getServerAddress());
      userInfo.setToolTipText(i18n.tr("Login name and server address"));

      new Spacer(headerArea, 32);

      new HeaderButton(headerArea, "icons/main-window/preferences.png", i18n.tr("Client preferences"), new Runnable() {
         @Override
         public void run()
         {
            showPreferences();
         }
      });

      helpMenuButton = new HeaderButton(headerArea, "icons/main-window/help.png", 
            BrandingManager.isExtendedHelpMenuEnabled() ? i18n.tr("Help") : i18n.tr("Open user manual"), new Runnable() {
         @Override
         public void run()
         {
            if (BrandingManager.isExtendedHelpMenuEnabled())
            {
               Rectangle bounds = helpMenuButton.getBounds();
               showMenu(helpMenuManager, headerArea.toDisplay(new Point(bounds.x, bounds.y + bounds.height + 2)));
            }
            else
            {
               ExternalWebBrowser.open(BrandingManager.getAdministratorGuideURL());
            }
         }
      });

      new HeaderButton(headerArea, "icons/main-window/about.png", i18n.tr("About NetXMS Management Client"), new Runnable() {
         @Override
         public void run()
         {
            BrandingManager.createAboutDialog(getShell()).open();
         }
      });

      new Spacer(headerArea, 8);

      // Perspective switcher
      Composite menuArea = new Composite(windowContent, SWT.NONE);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = verticalLayout ? 1 : 2;
      menuArea.setLayout(layout);
      gd = new GridData();
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
      mainMenu.setFont(perspectiveSwitcherFont);
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
      switchToPerspective(PreferenceStore.getInstance().getAsString(PreferenceStore.serverProperty("MainWindow.CurrentPerspective", session)));

      String motd = session.getMessageOfTheDay();
      if ((motd != null) && !motd.isEmpty())
         addMessage(MessageArea.INFORMATION, session.getMessageOfTheDay());

      getShell().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            headerFontBold.dispose();
         }
      });

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

      showServerClock = PreferenceStore.getInstance().getAsBoolean("Appearance.ShowServerClock", false);
      if (showServerClock && (serverClock == null))
      {
         createServerClockWidget();
         headerArea.layout(true);
      }
      else if (!showServerClock && (serverClock != null))
      {
         serverClock.dispose();
         serverClock = null;
         headerArea.layout(true);
      }
   }

   /**
    * Create server clock widget
    */
   private void createServerClockWidget()
   {
      serverClock = new ServerClock(serverClockArea, SWT.NONE);
      serverClock.setBackground(serverClockArea.getBackground());
      serverClock.setForeground(ThemeEngine.getForegroundColor("Window.Header"));
      serverClock.setFont(headerFontBold);
      serverClock.setDisplayFormatChangeListener(new Runnable() {
         @Override
         public void run()
         {
            headerArea.layout();
         }
      });
   }

   /**
    * Show menu at given location.
    *
    * @param location location to show menu at
    */
   private void showMenu(MenuManager menuManager, Point location)
   {
      Menu menu = menuManager.createContextMenu(getShell());
      menu.setLocation(location);
      menu.setVisible(true);
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

   /**
    * Spacer composite
    */
   private static class Spacer extends Composite
   {
      private int width;

      public Spacer(Composite parent, int width)
      {
         super(parent, SWT.NONE);
         this.width = width;
         setBackground(parent.getBackground());
         setLayoutData(new GridData(SWT.CENTER, SWT.FILL, false, true));
      }

      /**
       * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
       */
      @Override
      public Point computeSize(int wHint, int hHint, boolean changed)
      {
         return new Point(width, (hHint == SWT.DEFAULT) ? 20 : hHint);
      }
   }

   /**
    * Header button
    */
   private static class HeaderButton extends Canvas
   {
      private Image image;
      private boolean highlight = false;
      private boolean mouseDown = false;

      /**
       * Create header button.
       *
       * @param parent parent composite
       * @param imagePath path to image
       * @param handler selection handler
       */
      HeaderButton(Composite parent, String imagePath, String tooltip, Runnable handler)
      {
         super(parent, SWT.NONE);

         setToolTipText(tooltip);
         setBackground(parent.getBackground());
         final Color highlightColor = ThemeEngine.getBackgroundColor("Window.Header.Highlight");

         image = ResourceManager.getImage(imagePath);
         addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               image.dispose();
            }
         });

         addPaintListener(new PaintListener() {
            @Override
            public void paintControl(PaintEvent e)
            {
               if (highlight)
               {
                  Rectangle rect = getClientArea();
                  e.gc.setBackground(highlightColor);
                  e.gc.fillRoundRectangle(0, 0, rect.width, rect.height, 8, 8);
               }
               e.gc.drawImage(image, 2, 2);
            }
         });

         addMouseTrackListener(new MouseTrackListener() {
            @Override
            public void mouseEnter(MouseEvent e)
            {
               if (!highlight)
               {
                  highlight = true;
                  redraw();
               }
            }

            @Override
            public void mouseExit(MouseEvent e)
            {
               if (highlight)
               {
                  highlight = false;
                  redraw();
               }
               mouseDown = false;
            }

            @Override
            public void mouseHover(MouseEvent e)
            {
               if (!highlight)
               {
                  highlight = true;
                  redraw();
               }
            }
         });

         addMouseListener(new MouseAdapter() {
            @Override
            public void mouseUp(MouseEvent e)
            {
               if (mouseDown)
               {
                  mouseDown = false;
                  handler.run();
               }
            }

            @Override
            public void mouseDown(MouseEvent e)
            {
               mouseDown = true;
            }
         });

         addMouseMoveListener(new MouseMoveListener() {
            @Override
            public void mouseMove(MouseEvent e)
            {
               if (!mouseDown && !highlight)
                  return;

               Rectangle bounds = getBounds();
               if ((e.x < 0) || (e.y < 0) || (e.x > bounds.width) || (e.y > bounds.height))
               {
                  mouseDown = false;
                  if (highlight)
                  {
                     highlight = false;
                     redraw();
                  }
               }
            }
         });
      }

      /**
       * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
       */
      @Override
      public Point computeSize(int wHint, int hHint, boolean changed)
      {
         Rectangle r = image.getBounds();
         return new Point((wHint == SWT.DEFAULT) ? r.width + 4 : wHint, (hHint == SWT.DEFAULT) ? r.height + 4 : hHint);
      }
   }
}
