/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.util.Util;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Edit pane widget for dashboard elements
 */
public class EditPaneWidget extends Composite
{
   private static final RGB BACKGROUND_COLOR = new RGB(0, 0, 127);

   private final I18n i18n = LocalizationHelper.getI18n(EditPaneWidget.class);

	private DashboardControl dbc;
	private DashboardElement element;
	private Action actionEdit;
	private Action actionEditJson;
   private Action actionDuplicate;
	private Action actionDelete;
   private Action actionMoveLeft;
   private Action actionMoveRight;
   private Action actionHSpanIncrease;
   private Action actionHSpanDecrease;
   private Action actionHSpanFull;
   private Action actionVSpanIncrease;
   private Action actionVSpanDecrease;

	/**
	 * @param parent
	 */
	public EditPaneWidget(Composite parent, DashboardControl dbc, DashboardElement element)
	{
		super(parent, SWT.TRANSPARENT | SWT.NO_BACKGROUND);
		this.dbc = dbc;
		this.element = element;

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      setLayout(layout);

      final Color color;
      if (Util.isWindows())
      {
         color = new Color(getDisplay(), BACKGROUND_COLOR);
         addPaintListener(new PaintListener() {
            @Override
            public void paintControl(PaintEvent e)
            {
               final GC gc = e.gc;
               final Point size = getSize();
               gc.setBackground(color);
               gc.setAlpha(20);
               gc.fillRectangle(0, 0, size.x, size.y);
            }
         });
      }
      else
      {
         color = new Color(getDisplay(), BACKGROUND_COLOR, 20);
         setBackground(color);
      }
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            color.dispose();
         }
      });

		createActions();
		createContextMenu();
      createToolBar();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionEdit = new Action(i18n.tr("&Edit"), SharedIcons.EDIT) {
			@Override
			public void run()
			{
				dbc.editElement(element);
			}
		};

      actionEditJson = new Action(i18n.tr("Edit &JSON"), SharedIcons.JSON) { 
			@Override
			public void run()
			{
				dbc.editElementJson(element);
			}
		};

      actionDuplicate = new Action(i18n.tr("D&uplicate"), ResourceManager.getImageDescriptor("icons/dashboard-control/duplicate.png")) {
			@Override
			public void run()
			{
            dbc.duplicateElement(element);
			}
		};

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            dbc.deleteElement(element);
         }
      };

      actionMoveLeft = new Action(i18n.tr("Move &left"), ResourceManager.getImageDescriptor("icons/dashboard-control/move-left.png")) {
         @Override
         public void run()
         {
            dbc.moveElement(element, SWT.LEFT);
         }
      };

      actionMoveRight = new Action(i18n.tr("Move &right"), ResourceManager.getImageDescriptor("icons/dashboard-control/move-right.png")) {
         @Override
         public void run()
         {
            dbc.moveElement(element, SWT.RIGHT);
         }
      };

      actionHSpanIncrease = new Action(i18n.tr("Increase horizontal span"), ResourceManager.getImageDescriptor("icons/dashboard-control/h-span-increase.png")) {
         @Override
         public void run()
         {
            dbc.changeElementSpan(element, 1, 0);
         }
      };

      actionHSpanDecrease = new Action(i18n.tr("Decrease horizontal span"), ResourceManager.getImageDescriptor("icons/dashboard-control/h-span-decrease.png")) {
         @Override
         public void run()
         {
            dbc.changeElementSpan(element, -1, 0);
         }
      };

      actionHSpanFull = new Action(i18n.tr("&Full horizontal span"), ResourceManager.getImageDescriptor("icons/dashboard-control/full-width.png")) {
         @Override
         public void run()
         {
            dbc.setElementFullHSpan(element);
         }
      };

      actionVSpanIncrease = new Action(i18n.tr("Increase vertical span"), ResourceManager.getImageDescriptor("icons/dashboard-control/v-span-increase.png")) {
         @Override
         public void run()
         {
            dbc.changeElementSpan(element, 0, 1);
         }
      };

      actionVSpanDecrease = new Action(i18n.tr("Decrease vertical span"), ResourceManager.getImageDescriptor("icons/dashboard-control/v-span-decrease.png")) {
         @Override
         public void run()
         {
            dbc.changeElementSpan(element, 0, -1);
         }
      };
	}

	/**
	 * Create pop-up menu for alarm list
	 */
	private void createContextMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(this);
		setMenu(menu);
	}

	/**
    * Fill context menu
    * 
    * @param manager Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
	{
      manager.add(actionMoveLeft);
      manager.add(actionMoveRight);
      manager.add(new Separator());
      manager.add(actionHSpanIncrease);
      manager.add(actionHSpanDecrease);
      manager.add(actionHSpanFull);
      manager.add(actionVSpanIncrease);
      manager.add(actionVSpanDecrease);
      manager.add(new Separator());
      manager.add(actionEdit);
      manager.add(actionEditJson);
      manager.add(actionDuplicate);
      manager.add(actionDelete);
	}

   /**
    * Create toolbar
    */
   private void createToolBar()
   {
      Composite buttonBar = new Composite(this, SWT.BORDER);
      buttonBar.setBackground(getDisplay().getSystemColor(SWT.COLOR_WIDGET_BACKGROUND));
      buttonBar.setLayout(new RowLayout());
      buttonBar.setLayoutData(new GridData(SWT.CENTER, SWT.TOP, true, true));

      new ControlButton(buttonBar, actionMoveLeft);
      new ControlButton(buttonBar, actionMoveRight);
      new Label(buttonBar, SWT.NONE).setText("\u2003");
      new ControlButton(buttonBar, actionHSpanIncrease);
      new ControlButton(buttonBar, actionHSpanDecrease);
      new ControlButton(buttonBar, actionHSpanFull);
      new ControlButton(buttonBar, actionVSpanIncrease);
      new ControlButton(buttonBar, actionVSpanDecrease);
      new Label(buttonBar, SWT.NONE).setText("\u2003");
      new ControlButton(buttonBar, actionEdit);
      new ControlButton(buttonBar, actionEditJson);
      new ControlButton(buttonBar, actionDuplicate);
      new ControlButton(buttonBar, actionDelete);

      layout(true, true);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      return getParent().getSize();
   }

   /**
    * Control button
    */
   private static class ControlButton extends Canvas
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
      ControlButton(Composite parent, final Action action)
      {
         super(parent, SWT.NONE);

         setToolTipText(action.getText());
         final Color highlightColor = getDisplay().getSystemColor(SWT.COLOR_WIDGET_HIGHLIGHT_SHADOW);

         image = action.getImageDescriptor().createImage();
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
                  action.run();
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
