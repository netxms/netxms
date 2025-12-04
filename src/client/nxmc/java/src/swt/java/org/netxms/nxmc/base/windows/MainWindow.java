/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import java.lang.reflect.Constructor;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.ShellAdapter;
import org.eclipse.swt.events.ShellEvent;
import org.eclipse.swt.events.ShellListener;
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
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.Severity;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.BrandingManager;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.Startup;
import org.netxms.nxmc.base.UIElementFilter;
import org.netxms.nxmc.base.UIElementFilter.ElementType;
import org.netxms.nxmc.base.menus.HelpMenuManager;
import org.netxms.nxmc.base.menus.UserMenuManager;
import org.netxms.nxmc.base.preferencepages.AppearancePage;
import org.netxms.nxmc.base.preferencepages.HttpProxyPage;
import org.netxms.nxmc.base.preferencepages.LanguagePage;
import org.netxms.nxmc.base.preferencepages.RegionalSettingsPage;
import org.netxms.nxmc.base.preferencepages.ThemesPage;
import org.netxms.nxmc.base.views.NonRestorableView;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveSeparator;
import org.netxms.nxmc.base.views.PinLocation;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewFolder;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.base.widgets.RoundedLabel;
import org.netxms.nxmc.base.widgets.ServerClock;
import org.netxms.nxmc.base.widgets.Spacer;
import org.netxms.nxmc.base.widgets.WelcomePage;
import org.netxms.nxmc.keyboard.KeyBindingManager;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.preferencepages.AlarmPreferences;
import org.netxms.nxmc.modules.alarms.preferencepages.AlarmSounds;
import org.netxms.nxmc.modules.networkmaps.preferencepage.GeneralMapPreferences;
import org.netxms.nxmc.modules.objects.ObjectsPerspective;
import org.netxms.nxmc.modules.objects.preferencepages.ObjectsPreferences;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.ExternalWebBrowser;
import org.netxms.nxmc.tools.FontTools;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Main window
 */
public class MainWindow extends Window implements MessageAreaHolder
{
   private static final Logger logger = LoggerFactory.getLogger(MainWindow.class);
   private static final I18n i18n = LocalizationHelper.getI18n(MainWindow.class);

   private Composite windowContent;
   private ToolBar mainMenu;
   private Composite headerArea;
   private MessageArea messageArea;
   private Composite mainArea;
   private Composite perspectiveArea;
   private List<Perspective> perspectives;
   private Perspective currentPerspective;
   private Perspective pinboardPerspective;
   private SashForm verticalSplitArea;
   private SashForm horizontalSplitArea;
   private ViewFolder leftPinArea;
   private ViewFolder rightPinArea;
   private ViewFolder bottomPinArea;
   private boolean verticalLayout;
   private boolean showServerClock;
   private Composite serverClockArea;
   private ServerClock serverClock;
   private RoundedLabel objectsOutOfSyncIndicator;
   private HeaderButton userMenuButton;
   private UserMenuManager userMenuManager;
   private HeaderButton helpMenuButton;
   private HelpMenuManager helpMenuManager;
   private Font headerFontBold;
   private Font clockFont;
   private Action actionToggleFullScreen;
   private KeyBindingManager keyBindingManager;
   private List<Runnable> postOpenRunnables = new ArrayList<>();

   /**
    * @param parentShell
    */
   public MainWindow()
   {
      super((Shell)null);
      PreferenceStore ps = PreferenceStore.getInstance();
      verticalLayout = ps.getAsBoolean("Appearance.VerticalLayout", true);
      showServerClock = ps.getAsBoolean("Appearance.ShowServerClock", false);
      userMenuManager = new UserMenuManager();
      helpMenuManager = new HelpMenuManager();
      keyBindingManager = new KeyBindingManager();
   }

   /**
    * @see org.eclipse.jface.window.ApplicationWindow#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell shell)
   {
      super.configureShell(shell);

      final NXCSession session = Registry.getSession();
      shell.setText(String.format(i18n.tr("%s - %s"), BrandingManager.getClientProductName(), session.getUserName() + "@" + session.getServerAddress()));

      final SessionListener sessionListener = (n) -> processSessionNotification(n);
      session.addListener(sessionListener);

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
            for(Perspective p : perspectives)
            {
               if (!p.isInitialized())
                  continue;
               Memento m = new Memento();
               p.saveState(m);
               ps.set(PreferenceStore.serverProperty(p.getId()), m);
            }
            savePinArea(ps, PinLocation.BOTTOM, bottomPinArea);
            savePinArea(ps, PinLocation.LEFT, leftPinArea);
            savePinArea(ps, PinLocation.RIGHT, rightPinArea);
            savePopOutViews(ps);
            session.removeListener(sessionListener);
         }
      });

      shell.setImages(Startup.windowIcons);
   }
   
   /**
    * Save pin area views
    * @param ps persistent storage
    * @param location pin area location
    * @param pinArea pin area view
    */
   void savePinArea(PreferenceStore ps, PinLocation location, ViewFolder pinArea)
   {
      if (pinArea == null)
         return;
      Memento m = new Memento();
      pinArea.saveState(m);
      ps.set(PreferenceStore.serverProperty(location.name()), m);      
   }

   /**
    * Save pop out
    * 
    * @param ps persistent storage
    */
   void savePopOutViews(PreferenceStore ps)
   {
      List<String> viewList = new ArrayList<String>();
      Memento m = new Memento();
      for(View v : Registry.getPopoutViews())
      {
         String id = v.getGlobalId();        
         viewList.add(id);         
         Memento viewConfig = new Memento();
         v.saveState(viewConfig);
         m.set(id + ".state", viewConfig);
      }  
      m.set("PopOutViews.Views", viewList);     
      ps.set(PreferenceStore.serverProperty("PopOutViews"), m);   
   }

   /**
    * @see org.eclipse.jface.window.Window#getShellListener()
    */
   @Override
   protected ShellListener getShellListener()
   {
      return new ShellAdapter() {
         @Override
         public void shellActivated(ShellEvent e)
         {
            if (postOpenRunnables != null)
            {
               for(Runnable r : postOpenRunnables)
               {
                  logger.debug("Executing post-open handler");
                  r.run();
               }
               postOpenRunnables.clear();
               postOpenRunnables = null;
               showWelcomePage();
            }
         }

         @Override
         public void shellClosed(ShellEvent event)
         {
            event.doit = false; // don't close now
            if (canHandleShellCloseEvent())
            {
               handleShellCloseEvent();
            }
         }
      };
   }

   /**
    * Add runnable to be executed after window is open.
    *
    * @param postOpenRunnable runnable to be executed after window is open
    */
   public void addPostOpenRunnable(Runnable postOpenRunnable)
   {
      postOpenRunnables.add(postOpenRunnable);
   }

   /**
    * Show welcome page if not hidden
    */
   private void showWelcomePage()
   {
      final NXCSession session = Registry.getSession();

      if (!BrandingManager.isWelcomePageEnabled())
         return;

      PreferenceStore ps = PreferenceStore.getInstance();
      if (ps.getAsBoolean("WelcomePage.Disabled", !session.getClientConfigurationHintAsBoolean("EnableWelcomePage", true)))
         return;

      String serverVersion = session.getServerVersion();
      int dotCount = 0;
      for(int i = 0; i < serverVersion.length(); i++)
      {
         char ch = serverVersion.charAt(i);
         if (ch == '-')
         {
            serverVersion = serverVersion.substring(0, i);
            break;
         }
         if (ch == '.')
         {
            dotCount++;
            if (dotCount == 3)
            {
               serverVersion = serverVersion.substring(0, i);
               break;
            }
         }
      }

      String v = ps.getAsString("WelcomePage.LastDisplayedVersion");
      if (serverVersion.equals(v))
         return;

      WelcomePage welcomePage = new WelcomePage(mainArea, SWT.NONE, serverVersion);
      welcomePage.setSize(mainArea.getSize());
      welcomePage.moveAbove(null);
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
      headerFontBold = FontTools.createAdjustedFont(headerFont, 20 - (int)headerFont.getFontData()[0].height, SWT.BOLD);
      clockFont = FontTools.createAdjustedFont(headerFont, 2, SWT.BOLD);

      windowContent = new Composite(parent, SWT.NONE);

      GridLayout windowContentLayout = new GridLayout();
      windowContentLayout.marginWidth = 0;
      windowContentLayout.marginHeight = 0;
      windowContentLayout.verticalSpacing = 0;
      windowContent.setLayout(windowContentLayout);

      // Header
      Color headerBackgroundColor = new Color(parent.getDisplay(), BrandingManager.getAppHeaderBackground());
      Color headerForegroundColor = ThemeEngine.getForegroundColor("Window.Header");

      headerArea = new Composite(windowContent, SWT.NONE);
      headerArea.setBackground(headerBackgroundColor);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      headerArea.setLayoutData(gd);
      GridLayout layout = new GridLayout();
      layout.marginWidth = 5;
      layout.marginHeight = 5;
      layout.horizontalSpacing = 5;
      layout.numColumns = 15;
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
      title.setText(BrandingManager.getProductName().toUpperCase());

      Label filler = new Label(headerArea, SWT.CENTER);
      filler.setBackground(headerBackgroundColor);
      filler.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      objectsOutOfSyncIndicator = new RoundedLabel(headerArea);
      objectsOutOfSyncIndicator.setLabelForeground(null, headerForegroundColor);
      objectsOutOfSyncIndicator.setFont(headerFont);
      objectsOutOfSyncIndicator.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      serverClockArea = new Composite(headerArea, SWT.NONE);
      serverClockArea.setBackground(headerBackgroundColor);
      serverClockArea.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, true));
      serverClockArea.setLayout(new FillLayout());

      if (showServerClock)
      {
         createServerClockWidget();
      }

      new Spacer(headerArea, 32);

      RoundedLabel serverName = new RoundedLabel(headerArea);
      serverName.setLabelForeground(null, headerForegroundColor);
      serverName.setFont(headerFont);
      serverName.setText(session.getServerName());
      serverName.setToolTipText(i18n.tr("Server name"));
      RGB serverColor = ColorConverter.parseColorDefinition(session.getServerColor());
      if (serverColor != null)
      {
         serverName.setLabelBackground(new Color(serverColor));
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

      new HeaderButton(headerArea, "icons/main-window/preferences.png", i18n.tr("Client preferences"), () -> showPreferences());

      helpMenuButton = new HeaderButton(headerArea, "icons/main-window/help.png", 
            BrandingManager.isExtendedHelpMenuEnabled() ? i18n.tr("Help") : i18n.tr("Open user manual"), () -> {
               if (BrandingManager.isExtendedHelpMenuEnabled())
               {
                  Rectangle bounds = helpMenuButton.getBounds();
                  showMenu(helpMenuManager, headerArea.toDisplay(new Point(bounds.x, bounds.y + bounds.height + 2)));
               }
               else
               {
                  ExternalWebBrowser.open(BrandingManager.getAdministratorGuideURL());
               }
            });

      new HeaderButton(headerArea, "icons/main-window/about.png", i18n.tr("About NetXMS Management Client"), () -> BrandingManager.createAboutDialog(getShell()).open());

      new Spacer(headerArea, 8);

      mainArea = new Composite(windowContent, SWT.NONE);
      mainArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      mainArea.addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            for(Control c : mainArea.getChildren())
               c.setSize(mainArea.getSize());
         }
      });

      final Composite mainAreaInner = new Composite(mainArea, SWT.NONE);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      layout.numColumns = verticalLayout ? 2 : 1;
      mainAreaInner.setLayout(layout);

      // Perspective switcher
      Composite menuArea = new Composite(mainAreaInner, SWT.NONE);
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

      messageArea = new MessageArea(mainAreaInner, SWT.NONE);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      messageArea.setLayoutData(gd);

      horizontalSplitArea = new SashForm(mainAreaInner, SWT.HORIZONTAL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      horizontalSplitArea.setLayoutData(gd);

      verticalSplitArea = new SashForm(horizontalSplitArea, SWT.VERTICAL);

      perspectiveArea = new Composite(verticalSplitArea, SWT.NONE);
      perspectiveArea.addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            resizePerspectiveAreaContent();
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

      switchToPerspective("pinboard");
      PreferenceStore ps = PreferenceStore.getInstance();
      switchToPerspective(ps.getAsString(PreferenceStore.serverProperty("MainWindow.CurrentPerspective", session)));
      restorePinArea(ps, PinLocation.BOTTOM);
      restorePinArea(ps, PinLocation.LEFT);
      restorePinArea(ps, PinLocation.RIGHT);

      String motd = session.getMessageOfTheDay();
      if ((motd != null) && !motd.isEmpty())
         addMessage(MessageArea.INFORMATION, session.getMessageOfTheDay());

      getShell().addDisposeListener((e) -> {
         headerFontBold.dispose();
         clockFont.dispose();
      });

      actionToggleFullScreen = new Action(i18n.tr("&Full screen"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            getShell().setFullScreen(actionToggleFullScreen.isChecked());
         }
      };
      keyBindingManager.addBinding("F11", actionToggleFullScreen);

      return windowContent;
   }

   /**
    * Restore pin area
    * 
    * @param ps persistent storage
    * @param location restore location
    */
   public void restorePinArea(PreferenceStore ps, PinLocation location)
   {
      Memento memento = ps.getAsMemento(PreferenceStore.serverProperty(location.name()));
      List<String> views = memento.getAsStringList("ViewFolder.Views");
      for (String id : views)
      {
         Memento viewConfig = memento.getAsMemento(id + ".state");
         View v = null;
         try
         {
            Class<?> widgetClass = Class.forName(viewConfig.getAsString("class"));            
            Constructor<?> c = widgetClass.getDeclaredConstructor();
            c.setAccessible(true);         
            v = (View)c.newInstance();
            if (v != null)
            {
               v.restoreState(viewConfig);
               pinView(v, location);
            }
         }
         catch (Exception e)
         {
            pinView(new NonRestorableView(e, v != null ? v.getFullName() : id), location);
            logger.error("Cannot restore pinned view", e);
         }
      }     
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
      if (keyBindingManager.processKeyStroke(ks))
         return;

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
      boolean darkTheme = WidgetHelper.isSystemDarkTheme();
      final List<Image> perspectiveIcons = darkTheme ? new ArrayList<>() : null;

      UIElementFilter filter = Registry.getSingleton(UIElementFilter.class);

      final NXCSession session = Registry.getSession();
      perspectives = Registry.getPerspectives().stream().filter((p) -> p.isValidForSession(session) && filter.isVisible(ElementType.PERSPECTIVE, p.getId())).toList();
      for(final Perspective p : perspectives)
      {
         if (p instanceof PerspectiveSeparator)
         {
            new ToolItem(mainMenu, SWT.SEPARATOR);
            continue;
         }

         p.bindToWindow(this);
         ToolItem item = new ToolItem(mainMenu, SWT.RADIO);
         item.setData("PerspectiveId", p.getId());
         if (darkTheme)
         {
            Image image = new Image(getShell().getDisplay(), ColorConverter.invertImageColors(p.getImage().getImageData()));
            item.setImage(image);
            perspectiveIcons.add(image);
         }
         else
         {
            item.setImage(p.getImage());
         }
         if (!verticalLayout)
            item.setText(p.getName());
         KeyStroke shortcut = p.getKeyboardShortcut();
         item.setToolTipText((shortcut != null) ? p.getName() + "\t" + shortcut.toString() : p.getName());
         item.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               switchToPerspective(p);
            }
         });
         if (p.getId().equals("pinboard"))
            pinboardPerspective = p;
      }

      if (darkTheme)
      {
         getShell().addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               for(Image i : perspectiveIcons)
                  i.dispose();
            }
         });
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
    * @param location where to pin view
    */
   public void pinView(View view, PinLocation location)
   {
      logger.debug("Request to pin view with GlobalID=" + view.getGlobalId() + " at location=" + location);
      switch(location)
      {
         case BOTTOM:
            bottomPinArea = pinViewToArea(view, bottomPinArea, verticalSplitArea, false);
            break;
         case LEFT:
            leftPinArea = pinViewToArea(view, leftPinArea, horizontalSplitArea, true);
            break;
         case PINBOARD:
            pinboardPerspective.addMainView(view, true, true);
            break;
         case RIGHT:
            rightPinArea = pinViewToArea(view, rightPinArea, horizontalSplitArea, false);
            break;
      }
   }

   /**
    * Pin view to given area
    *
    * @param view view to pin
    * @param pinArea area to pin to (can be null)
    * @param splitter parent splitter control
    * @param moveToTop true if newly created area should be moved above all other elements
    * @return pin area (can be different from what was passed in <code>pinArea</code> argument)
    */
   private ViewFolder pinViewToArea(View view, ViewFolder pinArea, SashForm splitter, boolean moveToTop)
   {
      if ((pinArea == null) || pinArea.isDisposed())
      {
         int[] weights = splitter.getWeights();
         pinArea = new ViewFolder(this, null, splitter, true, false, false, false);
         pinArea.setAllViewsAsCloseable(true);
         pinArea.setUseGlobalViewId(true);
         pinArea.setDisposeWhenEmpty(true);
         if (moveToTop)
         {
            pinArea.moveAbove(null);
            int[] nweights = new int[weights.length + 1];
            int total = 0;
            for(int w : weights)
               total += w;
            nweights[0] = 3 * total / 7; // 30% of new full weight
            System.arraycopy(weights, 0, nweights, 1, weights.length);
            weights = nweights;
         }
         else
         {
            weights = Arrays.copyOf(weights, weights.length + 1);
            int total = 0;
            for(int w : weights)
               total += w;
            weights[weights.length - 1] = 3 * total / 7; // 30% of new full weight
         }
         splitter.setWeights(weights);
         windowContent.layout(true, true);
      }
      pinArea.addView(view, true, true);
      return pinArea;
   }

   /**
    * Show console preferences
    */
   private void showPreferences()
   {
      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("appearance", new AppearancePage()));
      pm.addToRoot(new PreferenceNode("alarm", new AlarmPreferences()));
      pm.addTo("alarm", new PreferenceNode("alarmSounds", new AlarmSounds()));
      pm.addToRoot(new PreferenceNode("httpProxy", new HttpProxyPage()));
      pm.addToRoot(new PreferenceNode("language", new LanguagePage()));
      pm.addToRoot(new PreferenceNode("networkMap", new GeneralMapPreferences()));
      pm.addToRoot(new PreferenceNode("objects", new ObjectsPreferences()));
      pm.addToRoot(new PreferenceNode("regionalSettings", new RegionalSettingsPage()));
      pm.addToRoot(new PreferenceNode("themes", new ThemesPage()));

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
         for(Control c : serverClockArea.getChildren())
            c.dispose();
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
      new Spacer(serverClockArea, 27, null);

      serverClock = new ServerClock(serverClockArea, SWT.NONE);
      serverClock.setBackground(serverClockArea.getBackground());
      serverClock.setForeground(ThemeEngine.getForegroundColor("Window.Header"));
      serverClock.setFont(clockFont);
      serverClock.setDisplayFormatChangeListener(() -> headerArea.layout());
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
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String, boolean, java.lang.String, java.lang.Runnable)
    */
   @Override
   public int addMessage(int level, String text, boolean sticky, String buttonText, Runnable action)
   {
      return messageArea.addMessage(level, text, sticky, buttonText, action);
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
    * Switch to object
    * 
    * @param objectId object id
    * @param dciId dci id
    */
   public static void switchToObject(long objectId, long dciId)
   {
      AbstractObject object = Registry.getSession().findObjectById(objectId);
      if (object == null)
         return;

      for (Perspective p : Registry.getPerspectives())
      {
         if (p instanceof ObjectsPerspective && ((ObjectsPerspective)p).showObject(object, dciId))
            break;
      }
   }

   /**
    * Process session notification.
    *
    * @param n notification
    */
   private void processSessionNotification(SessionNotification n)
   {
      if (n.getCode() == SessionNotification.OBJECTS_OUT_OF_SYNC)
      {
         getShell().getDisplay().asyncExec(() -> {
            objectsOutOfSyncIndicator.setText(i18n.tr("OBJECTS OUT OF SYNC"));
            objectsOutOfSyncIndicator.setLabelBackground(StatusDisplayInfo.getStatusBackgroundColor(Severity.MINOR));
            headerArea.layout(true);
         });
      }
      else if (n.getCode() == SessionNotification.OBJECTS_IN_SYNC)
      {
         getShell().getDisplay().asyncExec(() -> {
            objectsOutOfSyncIndicator.setText("");
            objectsOutOfSyncIndicator.setLabelBackground(null);
            headerArea.layout(true);
         });
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
