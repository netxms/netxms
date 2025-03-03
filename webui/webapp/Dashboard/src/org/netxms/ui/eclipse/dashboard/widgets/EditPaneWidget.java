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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;

/**
 * Edit pane widget for dashboard elements
 */
public class EditPaneWidget extends Composite
{
   private static final RGB BACKGROUND_COLOR = new RGB(0, 0, 127);

	private DashboardControl dbc;
	private DashboardElement element;
	private Action actionEdit;
	private Action actionEditXml;
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
		super(parent, SWT.NONE);
		this.dbc = dbc;
		this.element = element;

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      setLayout(layout);

      final Color color = new Color(getDisplay(), BACKGROUND_COLOR, 20);
      setBackground(color);
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
      actionEdit = new Action(Messages.get().EditPaneWidget_Edit, SharedIcons.EDIT) {
			@Override
			public void run()
			{
				dbc.editElement(element);
			}
		};

      actionEditXml = new Action(Messages.get().EditPaneWidget_EditXML, SharedIcons.XML) {
			@Override
			public void run()
			{
				dbc.editElementXml(element);
			}
		};

      actionDuplicate = new Action("Duplicate", Activator.getImageDescriptor("icons/duplicate.png")) {
			@Override
			public void run()
			{
            dbc.duplicateElement(element);
			}
		};

      actionDelete = new Action(Messages.get().EditPaneWidget_Delete, SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            dbc.deleteElement(element);
         }
      };

      actionMoveLeft = new Action("Move &left", Activator.getImageDescriptor("icons/move-left.png")) {
         @Override
         public void run()
         {
            dbc.moveElement(element, SWT.LEFT);
         }
      };

      actionMoveRight = new Action("Move &right", Activator.getImageDescriptor("icons/move-right.png")) {
         @Override
         public void run()
         {
            dbc.moveElement(element, SWT.RIGHT);
         }
      };

      actionHSpanIncrease = new Action("Increase horizontal span", Activator.getImageDescriptor("icons/h-span-increase.png")) {
         @Override
         public void run()
         {
            dbc.changeElementSpan(element, 1, 0);
         }
      };

      actionHSpanDecrease = new Action("Decrease horizontal span", Activator.getImageDescriptor("icons/h-span-decrease.png")) {
         @Override
         public void run()
         {
            dbc.changeElementSpan(element, -1, 0);
         }
      };

      actionHSpanFull = new Action("&Full horizontal span", Activator.getImageDescriptor("icons/full-width.png")) {
         @Override
         public void run()
         {
            dbc.setElementFullHSpan(element);
         }
      };

      actionVSpanIncrease = new Action("Increase vertical span", Activator.getImageDescriptor("icons/v-span-increase.png")) {
         @Override
         public void run()
         {
            dbc.changeElementSpan(element, 0, 1);
         }
      };

      actionVSpanDecrease = new Action("Decrease vertical span", Activator.getImageDescriptor("icons/v-span-decrease.png")) {
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
      manager.add(actionEditXml);
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
      Label label = new Label(buttonBar, SWT.NONE);
      label.setText("\u2003");
      label.setBackground(buttonBar.getBackground());
      new ControlButton(buttonBar, actionHSpanIncrease);
      new ControlButton(buttonBar, actionHSpanDecrease);
      new ControlButton(buttonBar, actionHSpanFull);
      new ControlButton(buttonBar, actionVSpanIncrease);
      new ControlButton(buttonBar, actionVSpanDecrease);
      label = new Label(buttonBar, SWT.NONE);
      label.setText("\u2003");
      label.setBackground(buttonBar.getBackground());
      new ControlButton(buttonBar, actionEdit);
      new ControlButton(buttonBar, actionEditXml);
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
      private Button button;

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

         setLayout(new FillLayout());
         setBackground(parent.getBackground());

         image = action.getImageDescriptor().createImage();
         addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               image.dispose();
            }
         });

         button = new Button(this, SWT.PUSH);
         button.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_ARROW));
         button.setBackground(parent.getBackground());
         button.setImage(image);
         button.setToolTipText(action.getText());
         button.setData(RWT.CUSTOM_VARIANT, "DashboardEditorButton");
         button.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
            }
         });
      }
   }
}
