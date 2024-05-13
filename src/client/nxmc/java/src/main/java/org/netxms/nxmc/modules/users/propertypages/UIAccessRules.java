/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.users.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.dialogs.UIElementSelectionDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "UI Access Rules" property page for user object
 */
public class UIAccessRules extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(UIAccessRules.class);

   private NXCSession session = Registry.getSession();
	private AbstractUserObject object;
   private LabeledText rules;

   /**
    * Default constructor
    */
   public UIAccessRules(AbstractUserObject object, MessageAreaHolder messageArea)
   {
      super(LocalizationHelper.getI18n(UIAccessRules.class).tr("UI Access Rules"), messageArea);
      this.object = object;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      rules = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL);
      rules.setLabel(i18n.tr("Rules"));
      rules.setText(object.getUIAccessRules().replace(";", "\n"));
      rules.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      Button addButton = new Button(dialogArea, SWT.PUSH);
      addButton.setText("&Add element...");
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            UIElementSelectionDialog dlg = new UIElementSelectionDialog(getShell());
            if (dlg.open() == Window.OK)
            {
               StringBuilder sb = new StringBuilder(rules.getText());
               sb.append('\n');
               for(String s : dlg.getSelection())
               {
                  sb.append(s);
                  sb.append('\n');
               }
               rules.setText(sb.toString());
            }
         }
      });
      GridData gd = new GridData();
      gd.widthHint = WidgetHelper.WIDE_BUTTON_WIDTH_HINT;
      addButton.setLayoutData(gd);

		return dialogArea;
	}

	/**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
	@Override
	protected boolean applyChanges(final boolean isApply)
	{
      if (isApply)
      {
         setMessage(null);
         setValid(false);
      }

      StringBuilder sb = new StringBuilder();
      for(String s : rules.getText().trim().split("\n"))
      {
         s = s.trim();
         if (!s.isEmpty())
         {
            if (sb.length() > 0)
               sb.append(';');
            sb.append(s);
         }
      }
      object.setUIAccessRules(sb.toString());

      new Job(i18n.tr("Updating user database"), null, getMessageArea(isApply)) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
            session.modifyUserDBObject(object, AbstractUserObject.MODIFY_UI_ACCESS_RULES);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot update user object");
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> UIAccessRules.this.setValid(true));
			}
		}.start();

		return true;
	}
}
