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
package org.netxms.nxmc.modules.events.dialogs;

import java.lang.reflect.InvocationTargetException;
import java.util.List;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.dialogs.DialogWithFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.EventTemplateList;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Event selection dialog
 */
public class EventSelectionDialog extends DialogWithFilter
{
   private final I18n i18n = LocalizationHelper.getI18n(EventSelectionDialog.class);
   private static final String CONFIG_PREFIX = "SelectEvent"; //$NON-NLS-1$
   private static final int CREATE_BUTTON_ID = IDialogConstants.CLIENT_ID + 1;

	private boolean multiSelection;
	private EventTemplate selectedEvents[];
	private EventTemplateList eventTemplateList;
   private EventTemplate defaultNewTemplate;

	/**
	 * @param parentShell
	 */
	public EventSelectionDialog(Shell parentShell)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		multiSelection = false;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Select Event"));
      PreferenceStore settings = PreferenceStore.getInstance();
      int cx = settings.getAsInteger(CONFIG_PREFIX + ".cx", 0);
      int cy = settings.getAsInteger(CONFIG_PREFIX + ".cy", 0);
      if ((cx > 0) && (cy > 0))
      {
         newShell.setSize(cx, cy);
      }
      else
      {
         newShell.setSize(600, 460);
      }
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      dialogArea.setLayout(new FillLayout());

      eventTemplateList = new EventTemplateList(dialogArea, SWT.NONE, CONFIG_PREFIX, true);
      setFilterClient(eventTemplateList.getViewer(), eventTemplateList.getFilter());

      eventTemplateList.getViewer().addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            EventSelectionDialog.this.okPressed();
         }
      });

      eventTemplateList.addDisposeListener(new DisposeListener() {

         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            Point size = getShell().getSize();
            PreferenceStore settings = PreferenceStore.getInstance();
            settings.set(CONFIG_PREFIX + ".cx", size.x); //$NON-NLS-1$
            settings.set(CONFIG_PREFIX + ".cy", size.y); //$NON-NLS-1$
         }
      });

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@SuppressWarnings("unchecked")
	@Override
	protected void okPressed()
	{
      final IStructuredSelection selection = eventTemplateList.getViewer().getStructuredSelection();
		final List<EventTemplate> list = selection.toList();
		selectedEvents = list.toArray(new EventTemplate[list.size()]);
		super.okPressed();
	}

	/**
	 * @return true if multiple event selection is enabled
	 */
	public boolean isMultiSelectionEnabled()
	{
		return multiSelection;
	}

	/**
	 * Enable or disable selection of multiple events.
	 *
	 * @param enable true to enable multiselection, false to disable
	 */
	public void enableMultiSelection(boolean enable)
	{
		this.multiSelection = enable;
	}

	/**
	 * Get selected event templates
	 *
	 * @return Selected event templates
	 */
	public EventTemplate[] getSelectedEvents()
	{
		return selectedEvents;
	}

   /**
    * Set default template for new event creation. If set, the "Create" button will pre-populate new event fields from this template.
    *
    * @param defaultNewTemplate default template for new events (can be null)
    */
   public void setDefaultNewTemplate(EventTemplate defaultNewTemplate)
   {
      this.defaultNewTemplate = defaultNewTemplate;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, CREATE_BUTTON_ID, i18n.tr("&Create new..."), false);
      super.createButtonsForButtonBar(parent);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#buttonPressed(int)
    */
   @Override
   protected void buttonPressed(int buttonId)
   {
      if (buttonId == CREATE_BUTTON_ID)
      {
         createNewEvent();
         return;
      }
      super.buttonPressed(buttonId);
   }

   /**
    * Create new event template and select it
    */
   private void createNewEvent()
   {
      final EventTemplate tmpl = (defaultNewTemplate != null) ? new EventTemplate(defaultNewTemplate) : new EventTemplate(0);
      tmpl.setCode(0);
      EditEventTemplateDialog editDlg = new EditEventTemplateDialog(getShell(), tmpl, true);
      if (editDlg.open() != Window.OK)
         return;

      NXCSession session = Registry.getSession();
      try
      {
         new ProgressMonitorDialog(getShell()).run(true, false, (monitor) -> {
            try
            {
               session.modifyEventObject(tmpl);
            }
            catch(Exception e)
            {
               throw new InvocationTargetException(e);
            }
         });
      }
      catch(InvocationTargetException e)
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Cannot create event template: ") + e.getCause().getLocalizedMessage());
         return;
      }
      catch(InterruptedException e)
      {
         return;
      }

      selectedEvents = new EventTemplate[] { tmpl };
      super.okPressed();
   }
}
