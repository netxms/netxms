/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.propertypages;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.PredictionEngine;
import org.netxms.ui.eclipse.datacollection.propertypages.helpers.DCIPropertyPageDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Analytics" property page for DCI
 */
public class Analytics extends DCIPropertyPageDialog
{
	private DataCollectionItem dci;
	private Combo predictionEngine;
	private List<PredictionEngine> engines;

	/**
	 * Constructor 
	 */
	public Analytics()
   {
      super();
      try
      {
         engines = ConsoleSharedData.getSession().getPredictionEngines();
      }
      catch(Exception e)
      {
         engines = new ArrayList<PredictionEngine>(0);
      }
   }

   /* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
	   Composite dialogArea = (Composite)super.createContents(parent);
		dci = editor.getObjectAsItem();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      predictionEngine = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Prediction engine", new GridData(SWT.FILL, SWT.CENTER, true, false));
      predictionEngine.add("None");
      for(PredictionEngine e : engines)
      {
         predictionEngine.add(e.getDescription());
         if (e.getName().equals(dci.getPredictionEngine()))
            predictionEngine.select(predictionEngine.getItemCount() - 1);
      }
      if (predictionEngine.getSelectionIndex() == -1)
         predictionEngine.select(0);

      return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
	   int index = predictionEngine.getSelectionIndex();
	   dci.setPredictionEngine((index > 0) ? engines.get(index - 1).getName() : "");
		editor.modify();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		predictionEngine.select(0);
	}
}
