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
package org.netxms.nxmc.modules.objects.views;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Room;
import org.netxms.client.objects.configs.RoomPassiveElement;
import org.netxms.client.objects.configs.RoomPoint;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.dialogs.RoomPassiveElementEditDialog;
import org.netxms.nxmc.modules.objects.widgets.FloorPlanWidget;
import org.netxms.nxmc.modules.objects.widgets.FloorPlanWidget.OutlineVertex;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.modules.objects.widgets.helpers.FloorPlanEditListener;
import org.netxms.nxmc.modules.objects.widgets.helpers.FloorPlanModel;
import org.netxms.nxmc.modules.objects.widgets.helpers.FloorPlanModel.RackPlacement;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Room floor plan view
 */
public class FloorPlanView extends ObjectView implements ISelectionProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(FloorPlanView.class);

   private Composite area;
   private FloorPlanWidget floorPlan;
   private Composite tray;
   private TableViewer trayViewer;
   private Menu objectMenu;
   private Menu editMenu;
   private boolean modified = false;
   private int warningMessageId = 0;
   private ISelection selection = new StructuredSelection();
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();

   private Action actionZoomIn;
   private Action actionZoomOut;
   private Action actionZoomFit;
   private Action actionEditMode;
   private Action actionSave;
   private Action actionDiscard;
   private Action actionRotateCW;
   private Action actionRotateCCW;
   private Action actionDelete;
   private Action actionAddElement;
   private Action actionEditElement;
   private Action actionSnapToGrid;
   private Action actionMoveBackdrop;
   private Action actionCalibrate;
   private Action actionPlaceRack;
   private Action actionOpenRack;

   /**
    * Create floor plan view
    */
   public FloorPlanView()
   {
      super(LocalizationHelper.getI18n(FloorPlanView.class).tr("Floor Plan"), ResourceManager.getImageDescriptor("icons/object-views/floor-plan.png"), "objects.floor-plan", false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Room);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 1;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      area = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout(2, false);
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.horizontalSpacing = 0;
      area.setLayout(layout);

      tray = new Composite(area, SWT.BORDER);
      tray.setLayout(new GridLayout());
      GridData gd = new GridData(SWT.FILL, SWT.FILL, false, true);
      gd.widthHint = 220;
      gd.exclude = true;
      tray.setLayoutData(gd);
      tray.setVisible(false);

      Label trayLabel = new Label(tray, SWT.NONE);
      trayLabel.setText(i18n.tr("Racks not placed on plan"));
      trayLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      trayViewer = new TableViewer(tray, SWT.SINGLE | SWT.FULL_SELECTION);
      trayViewer.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      trayViewer.setContentProvider(new ArrayContentProvider());
      trayViewer.setLabelProvider(new BaseObjectLabelProvider());
      trayViewer.addSelectionChangedListener((e) -> {
         IStructuredSelection s = trayViewer.getStructuredSelection();
         if (!s.isEmpty())
         {
            floorPlan.setSelection(null);
            setSelection(s);
         }
         updateActionState();
      });
      trayViewer.addDoubleClickListener((e) -> placeSelectedRack());

      createActions();
      createContextMenus();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionZoomIn = new Action(i18n.tr("Zoom &in"), SharedIcons.ZOOM_IN) {
         @Override
         public void run()
         {
            if (floorPlan != null)
               floorPlan.zoomIn();
         }
      };
      addKeyBinding("M1+=", actionZoomIn);

      actionZoomOut = new Action(i18n.tr("Zoom &out"), SharedIcons.ZOOM_OUT) {
         @Override
         public void run()
         {
            if (floorPlan != null)
               floorPlan.zoomOut();
         }
      };
      addKeyBinding("M1+-", actionZoomOut);

      actionZoomFit = new Action(i18n.tr("Zoom to &fit"), ResourceManager.getImageDescriptor("icons/netmap/fit.png")) {
         @Override
         public void run()
         {
            if (floorPlan != null)
               floorPlan.zoomToFit();
         }
      };
      addKeyBinding("M1+0", actionZoomFit);

      actionEditMode = new Action(i18n.tr("&Edit floor plan"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            setEditMode(isChecked());
         }
      };
      actionEditMode.setImageDescriptor(SharedIcons.EDIT);
      addKeyBinding("M1+E", actionEditMode);

      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            save();
         }
      };
      addKeyBinding("M1+S", actionSave);

      actionDiscard = new Action(i18n.tr("&Discard changes"), ResourceManager.getImageDescriptor("icons/netmap/refresh.png")) {
         @Override
         public void run()
         {
            discard();
         }
      };

      actionRotateCW = new Action(i18n.tr("Rotate clockwise")) {
         @Override
         public void run()
         {
            floorPlan.rotateSelection(90);
         }
      };
      addKeyBinding("M1+R", actionRotateCW);

      actionRotateCCW = new Action(i18n.tr("Rotate counterclockwise")) {
         @Override
         public void run()
         {
            floorPlan.rotateSelection(-90);
         }
      };
      addKeyBinding("M1+M2+R", actionRotateCCW);

      actionDelete = new Action(i18n.tr("&Remove from plan"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            floorPlan.deleteSelection();
         }
      };
      addKeyBinding("M1+D", actionDelete);

      actionAddElement = new Action(i18n.tr("&Add element..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            RoomPassiveElement e = floorPlan.createPassiveElement();
            if (new RoomPassiveElementEditDialog(getWindow().getShell(), e).open() == Window.OK)
               floorPlan.passiveElementChanged();
         }
      };
      addKeyBinding("M1+N", actionAddElement);

      actionEditElement = new Action(i18n.tr("&Properties..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            Object s = floorPlan.getSelection();
            if (s instanceof RoomPassiveElement)
               editElement((RoomPassiveElement)s);
         }
      };

      actionSnapToGrid = new Action(i18n.tr("Snap to &grid"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            floorPlan.setSnapToGrid(isChecked());
         }
      };
      actionSnapToGrid.setImageDescriptor(ResourceManager.getImageDescriptor("icons/netmap/snap_to_grid.png"));
      actionSnapToGrid.setChecked(true);

      actionMoveBackdrop = new Action(i18n.tr("&Move background image"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            floorPlan.setMoveBackdropMode(isChecked());
         }
      };

      actionCalibrate = new Action(i18n.tr("&Calibrate background image...")) {
         @Override
         public void run()
         {
            if (floorPlan.isCalibrating())
            {
               floorPlan.cancelCalibration();
               return;
            }
            addMessage(MessageArea.INFORMATION, i18n.tr("Click two points on the background image with known real-world distance between them"), false);
            floorPlan.startCalibration();
         }
      };

      actionPlaceRack = new Action(i18n.tr("&Place on plan")) {
         @Override
         public void run()
         {
            placeSelectedRack();
         }
      };

      actionOpenRack = new Action(i18n.tr("&Open rack view")) {
         @Override
         public void run()
         {
            Object s = floorPlan.getSelection();
            if (s instanceof Rack)
               openView(new AdHocRackView(getObjectId(), (Rack)s));
         }
      };
   }

   /**
    * Create context menus: object menu for racks, edit menu for everything else
    */
   private void createContextMenus()
   {
      MenuManager objectMenuManager = new ObjectContextMenuManager(this, this, null) {
         @Override
         protected void fillContextMenu()
         {
            add(actionOpenRack);
            if (floorPlan.isEditMode())
            {
               add(actionRotateCW);
               add(actionRotateCCW);
               add(actionDelete);
            }
            add(new Separator());
            super.fillContextMenu();
         }
      };
      objectMenu = objectMenuManager.createContextMenu(area);

      MenuManager editMenuManager = new MenuManager();
      editMenuManager.setRemoveAllWhenShown(true);
      editMenuManager.addMenuListener((manager) -> {
         Object s = floorPlan.getSelection();
         if (s instanceof RoomPassiveElement)
         {
            manager.add(actionEditElement);
            manager.add(actionRotateCW);
            manager.add(actionRotateCCW);
            manager.add(actionDelete);
         }
         else if (s instanceof OutlineVertex)
         {
            manager.add(actionDelete);
         }
         else
         {
            manager.add(actionAddElement);
         }
         manager.add(new Separator());
         manager.add(actionSnapToGrid);
         manager.add(actionMoveBackdrop);
         manager.add(actionCalibrate);
      });
      editMenu = editMenuManager.createContextMenu(area);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(actionZoomFit);
      manager.add(new Separator());
      manager.add(actionEditMode);
      manager.add(actionSave);
      manager.add(actionDiscard);
      manager.add(new Separator());
      manager.add(actionAddElement);
      manager.add(actionDelete);
      manager.add(actionSnapToGrid);
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(actionZoomFit);
      manager.add(new Separator());
      manager.add(actionEditMode);
      manager.add(actionSave);
      manager.add(actionDiscard);
      manager.add(new Separator());
      manager.add(actionAddElement);
      manager.add(actionRotateCW);
      manager.add(actionRotateCCW);
      manager.add(actionDelete);
      manager.add(new Separator());
      manager.add(actionSnapToGrid);
      manager.add(actionMoveBackdrop);
      manager.add(actionCalibrate);
      super.fillLocalMenu(manager);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if (floorPlan != null)
      {
         floorPlan.dispose();
         floorPlan = null;
      }
      modified = false;
      actionEditMode.setChecked(false);

      if (object instanceof Room)
      {
         floorPlan = new FloorPlanWidget(area, SWT.NONE, (Room)object, this);
         floorPlan.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
         floorPlan.moveAbove(tray);
         floorPlan.setSnapToGrid(actionSnapToGrid.isChecked());
         floorPlan.addSelectionListener((item) -> {
            if (item != null)
               trayViewer.setSelection(StructuredSelection.EMPTY);
            setSelection((item instanceof Rack) ? new StructuredSelection(item) : StructuredSelection.EMPTY);
            floorPlan.setMenu((item instanceof Rack) ? objectMenu : (floorPlan.isEditMode() ? editMenu : null));
            updateActionState();
         });
         floorPlan.setEditListener(new FloorPlanEditListener() {
            @Override
            public void modelChanged()
            {
               modified = true;
               refreshTray();
               updateActionState();
               updateWarnings();
            }

            @Override
            public void editElementRequested(RoomPassiveElement element)
            {
               editElement(element);
            }

            @Override
            public void calibrationPointsSelected(double imagePixelDistance)
            {
               calibrate(imagePixelDistance);
            }
         });
         area.layout(true, true);
      }
      refreshTray();
      updateActionState();
      updateWarnings();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      if (floorPlan == null)
         return;

      Room room = (object instanceof Room) ? (Room)object : session.findObjectById(getObjectId(), Room.class);
      if (room == null)
         return;

      if (modified)
         floorPlan.update(room);   // keep local edits, refresh rack list and statuses
      else
         floorPlan.reload(room);
      refreshTray();
      updateActionState();
      updateWarnings();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isRelatedObject(long)
    */
   @Override
   protected boolean isRelatedObject(long objectId)
   {
      AbstractObject object = getObject();
      if (object == null)
         return false;
      for(long id : object.getChildIdList())
         if (id == objectId)
            return true;
      return false;
   }

   /**
    * Switch edit mode on or off. Leaving edit mode with unsaved changes asks for confirmation.
    *
    * @param enable true to enable edit mode
    */
   private void setEditMode(boolean enable)
   {
      if (floorPlan == null)
         return;

      if (enable)
      {
         Room room = (Room)getObject();
         if ((room.getEffectiveRights() & UserAccessRights.OBJECT_ACCESS_MODIFY) == 0)
         {
            actionEditMode.setChecked(false);
            addMessage(MessageArea.WARNING, i18n.tr("You do not have permission to modify this room"));
            return;
         }
      }
      else if (modified)
      {
         if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Discard Changes"), i18n.tr("Floor plan has unsaved changes. Discard them?")))
         {
            actionEditMode.setChecked(true);
            return;
         }
         discard();
      }

      floorPlan.setEditMode(enable);
      actionMoveBackdrop.setChecked(false);
      floorPlan.setMenu((floorPlan.getSelection() instanceof Rack) ? objectMenu : (enable ? editMenu : null));
      refreshTray();
      updateActionState();
      updateWarnings();
   }

   /**
    * Update enabled state of actions
    */
   private void updateActionState()
   {
      boolean hasPlan = (floorPlan != null);
      boolean edit = hasPlan && floorPlan.isEditMode();
      Object s = hasPlan ? floorPlan.getSelection() : null;

      actionZoomIn.setEnabled(hasPlan);
      actionZoomOut.setEnabled(hasPlan);
      actionZoomFit.setEnabled(hasPlan);
      actionEditMode.setEnabled(hasPlan);
      actionSave.setEnabled(edit && modified);
      actionDiscard.setEnabled(edit && modified);
      actionAddElement.setEnabled(edit);
      actionSnapToGrid.setEnabled(edit);
      actionMoveBackdrop.setEnabled(edit && (((Room)getObject()).getBackgroundImage() != null));
      actionCalibrate.setEnabled(edit && (((Room)getObject()).getBackgroundImage() != null));
      actionRotateCW.setEnabled(edit && ((s instanceof Rack) || (s instanceof RoomPassiveElement)));
      actionRotateCCW.setEnabled(actionRotateCW.isEnabled());
      actionDelete.setEnabled(edit && (s != null));
      actionEditElement.setEnabled(edit && (s instanceof RoomPassiveElement));
      actionPlaceRack.setEnabled(edit && !trayViewer.getStructuredSelection().isEmpty());
      actionOpenRack.setEnabled(s instanceof Rack);
   }

   /**
    * Show or hide tray with unplaced racks and refresh its content
    */
   private void refreshTray()
   {
      List<Rack> racks = (floorPlan != null) ? floorPlan.getModel().getUnplacedRacks() : new ArrayList<Rack>();
      boolean visible = !racks.isEmpty();
      if (tray.getVisible() != visible)
      {
         tray.setVisible(visible);
         ((GridData)tray.getLayoutData()).exclude = !visible;
         area.layout(true, true);
      }
      if (visible)
      {
         trayViewer.setInput(racks);
         if (trayViewer.getTable().getMenu() == null)
         {
            MenuManager manager = new MenuManager();
            manager.add(actionPlaceRack);
            trayViewer.getTable().setMenu(manager.createContextMenu(trayViewer.getTable()));
         }
      }
   }

   /**
    * Place rack selected in the tray on the floor plan
    */
   private void placeSelectedRack()
   {
      if ((floorPlan == null) || !floorPlan.isEditMode())
      {
         addMessage(MessageArea.INFORMATION, i18n.tr("Switch to edit mode to place racks on the floor plan"));
         return;
      }
      Object s = trayViewer.getStructuredSelection().getFirstElement();
      if (s instanceof Rack)
         floorPlan.placeRack((Rack)s);
   }

   /**
    * Open passive element edit dialog
    */
   private void editElement(RoomPassiveElement element)
   {
      if (new RoomPassiveElementEditDialog(getWindow().getShell(), element).open() == Window.OK)
         floorPlan.passiveElementChanged();
   }

   /**
    * Ask for real distance between calibration points and apply calibration
    */
   private void calibrate(double imagePixelDistance)
   {
      InputDialog dlg = new InputDialog(getWindow().getShell(), i18n.tr("Calibrate Background Image"), i18n.tr("Real distance between selected points (mm)"), "1000",
            new IInputValidator() {
               @Override
               public String isValid(String newText)
               {
                  try
                  {
                     return (Integer.parseInt(newText.trim()) > 0) ? null : i18n.tr("Distance must be a positive number");
                  }
                  catch(NumberFormatException e)
                  {
                     return i18n.tr("Distance must be a positive number");
                  }
               }
            });
      if (dlg.open() == Window.OK)
         floorPlan.applyCalibration(imagePixelDistance, Integer.parseInt(dlg.getValue().trim()));
   }

   /**
    * Show placement warnings in message area (edit mode only)
    */
   private void updateWarnings()
   {
      if (warningMessageId != 0)
      {
         deleteMessage(warningMessageId);
         warningMessageId = 0;
      }
      if ((floorPlan == null) || !floorPlan.isEditMode())
         return;

      List<String> warnings = floorPlan.getWarnings();
      if (warnings.isEmpty())
         return;

      StringBuilder sb = new StringBuilder();
      for(int i = 0; (i < warnings.size()) && (i < 5); i++)
      {
         if (i > 0)
            sb.append("; ");
         sb.append(warnings.get(i));
      }
      if (warnings.size() > 5)
         sb.append(i18n.tr("; and {0} more", warnings.size() - 5));
      warningMessageId = addMessage(MessageArea.WARNING, sb.toString(), true);
   }

   /**
    * Save floor plan: one modification for the room, one for each rack whose placement changed
    */
   private void save()
   {
      if (floorPlan == null)
         return;

      final FloorPlanModel model = floorPlan.getModel();
      final Room room = model.getRoom();
      final List<NXCObjectModificationData> updates = new ArrayList<NXCObjectModificationData>();

      if (model.isRoomModified())
      {
         NXCObjectModificationData md = new NXCObjectModificationData(room.getObjectId());
         List<RoomPoint> outline = new ArrayList<RoomPoint>(model.getOutline().size());
         for(RoomPoint p : model.getOutline())
            outline.add(new RoomPoint(p));
         md.setRoomOutline(outline);
         List<RoomPassiveElement> elements = new ArrayList<RoomPassiveElement>(model.getPassiveElements().size());
         for(RoomPassiveElement e : model.getPassiveElements())
            elements.add(new RoomPassiveElement(e));
         md.setRoomPassiveElements(elements);
         md.setRoomBackgroundScale(model.getBackgroundScale());
         md.setRoomBackgroundX(model.getBackgroundX());
         md.setRoomBackgroundY(model.getBackgroundY());
         updates.add(md);
      }

      for(Rack rack : model.getModifiedRacks())
      {
         RackPlacement p = model.getPlacement(rack.getObjectId());
         NXCObjectModificationData md = new NXCObjectModificationData(rack.getObjectId());
         md.setRoomX(p.x);
         md.setRoomY(p.y);
         md.setRoomRotation(p.rotation);
         md.setObjectFlags(p.placed ? Rack.PLACED_IN_ROOM : 0, Rack.PLACED_IN_ROOM);
         updates.add(md);
      }

      new Job(i18n.tr("Saving floor plan of room {0}", room.getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(NXCObjectModificationData md : updates)
               session.modifyObject(md);
            runInUIThread(() -> {
               modified = false;
               updateActionState();
               addMessage(MessageArea.INFORMATION, i18n.tr("Floor plan saved"));
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save floor plan");
         }
      }.start();
   }

   /**
    * Discard local changes and reload floor plan from server
    */
   private void discard()
   {
      if (floorPlan == null)
         return;
      Room room = session.findObjectById(getObjectId(), Room.class);
      if (room != null)
         floorPlan.reload(room);
      modified = false;
      refreshTray();
      updateActionState();
      updateWarnings();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#beforeClose()
    */
   @Override
   public boolean beforeClose()
   {
      if (!modified)
         return true;
      return MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Discard Changes"), i18n.tr("Floor plan has unsaved changes. Close anyway?"));
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void addSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionListeners.add(listener);
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void removeSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionListeners.remove(listener);
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
    */
   @Override
   public ISelection getSelection()
   {
      return selection;
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void setSelection(ISelection selection)
   {
      this.selection = selection;
      for(ISelectionChangedListener l : selectionListeners)
         l.selectionChanged(new SelectionChangedEvent(this, selection));
   }
}
