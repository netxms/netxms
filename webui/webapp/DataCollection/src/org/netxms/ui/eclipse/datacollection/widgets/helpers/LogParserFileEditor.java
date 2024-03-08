/**
 * NetXMS - open source network management system
 * Copyright (C) 2018-2024 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.util.Arrays;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.widgets.LogParserEditor;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.DashboardComposite;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Editor widget for single log parser file
 */
public class LogParserFileEditor extends DashboardComposite
{	
	private LogParserFile file;
	private LogParserEditor editor;

   private Combo comboFileEncoding;
   private LabeledText labelFileName;
   private Button checkPreallocated;
   private Button checkSnapshot;
   private Button checkKeepOpen;
   private Button checkIgnoreModificationTime;
   private Button checkRescan;
   private Button checkFollowSymlinks;
   private Button checkRemoveEscapeSequences;

	/**
	 * @param parent
	 * @param style
	 */
   public LogParserFileEditor(Composite parent, LogParserFile file, final LogParserEditor editor)
	{
		super(parent, SWT.BORDER);

		this.file = file;
		this.editor = editor;

      setBackground(parent.getBackground());

		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		layout.makeColumnsEqualWidth = false;
		setLayout(layout);
		
		labelFileName = new LabeledText(this, SWT.NONE);
      labelFileName.setLabel("File path");
      labelFileName.setBackground(getBackground());
      labelFileName.setText((file != null) ? file.getFile() : ""); //$NON-NLS-1$
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      labelFileName.setLayoutData(gd);
      labelFileName.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.fireModifyListeners();
            editor.updateRules();
         }
      });

      String[] items = { "AUTO", "ACP", "UTF-8", "UCS-2", "UCS-2LE", "UCS-2BE", "UCS-4", "UCS-4LE", "UCS-4BE" };

      gd = new GridData();
      gd.grabExcessHorizontalSpace = false;
      gd.horizontalAlignment = SWT.FILL;
      comboFileEncoding = WidgetHelper.createLabeledCombo(this, SWT.BORDER | SWT.READ_ONLY, "File encoding", gd);
      comboFileEncoding.setItems(items);
      comboFileEncoding.select((file != null && file.getEncoding() != null ) ? Arrays.asList(items).indexOf(file.getEncoding()) : 0);
      comboFileEncoding.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.fireModifyListeners();
         }
      });

      final Composite controlBar = new Composite(this, SWT.NONE);
      controlBar.setLayout(new RowLayout(SWT.HORIZONTAL));
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      controlBar.setLayoutData(gd);
      fillControlBar(controlBar);

      final Composite checkboxBar = new Composite(this, SWT.NONE);
      GridLayout checkboxBarLayout = new GridLayout();
      checkboxBarLayout.numColumns = 4;
      checkboxBarLayout.makeColumnsEqualWidth = true;
      checkboxBar.setLayout(checkboxBarLayout);
      gd = new GridData();
      gd.horizontalSpan = 3;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.LEFT;
      checkboxBar.setLayoutData(gd);

      final SelectionListener checkboxSelectionListener = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editor.fireModifyListeners();
         }
      };

      checkPreallocated = new Button(checkboxBar, SWT.CHECK);
      checkPreallocated.setBackground(getBackground());
      checkPreallocated.setText("&Preallocated");
      checkPreallocated.setSelection(file.getPreallocated());
      checkPreallocated.addSelectionListener(checkboxSelectionListener);

      checkSnapshot = new Button(checkboxBar, SWT.CHECK);
      checkSnapshot.setBackground(getBackground());
      checkSnapshot.setText("Use &snapshot");
      checkSnapshot.setSelection(file.getSnapshot());
      checkSnapshot.addSelectionListener(checkboxSelectionListener);

      checkKeepOpen = new Button(checkboxBar, SWT.CHECK);
      checkKeepOpen.setBackground(getBackground());
      checkKeepOpen.setText("&Keep open");
      checkKeepOpen.setSelection(file.getKeepOpen());
      checkKeepOpen.addSelectionListener(checkboxSelectionListener);
      
      checkIgnoreModificationTime = new Button(checkboxBar, SWT.CHECK);
      checkIgnoreModificationTime.setBackground(getBackground());
      checkIgnoreModificationTime.setText("Ignore &modification time");
      checkIgnoreModificationTime.setSelection(file.getIgnoreModificationTime());
      checkIgnoreModificationTime.addSelectionListener(checkboxSelectionListener);

      checkRescan = new Button(checkboxBar, SWT.CHECK);
      checkRescan.setBackground(getBackground());
      checkRescan.setText("&Rescan");
      checkRescan.setSelection(file.getRescan());
      checkRescan.addSelectionListener(checkboxSelectionListener);

      checkFollowSymlinks = new Button(checkboxBar, SWT.CHECK);
      checkFollowSymlinks.setBackground(getBackground());
      checkFollowSymlinks.setText("&Follow symlinks");
      checkFollowSymlinks.setSelection(file.getFollowSymlinks());
      checkFollowSymlinks.addSelectionListener(checkboxSelectionListener);

      checkRemoveEscapeSequences = new Button(checkboxBar, SWT.CHECK);
      checkRemoveEscapeSequences.setBackground(getBackground());
      checkRemoveEscapeSequences.setText("Remove &escape sequences");
      checkRemoveEscapeSequences.setSelection(file.getRemoveEscapeSequences());
      checkRemoveEscapeSequences.addSelectionListener(checkboxSelectionListener);
	}

	/**
	 * Fill control bar
	 * 
	 * @param parent
	 */
	private void fillControlBar(Composite parent)
	{
      ImageHyperlink link = new ImageHyperlink(parent, SWT.NONE);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.setToolTipText(Messages.get().LogParserRuleEditor_DeleteRule);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				editor.deleteFile(file);
			}
		});
	}

	/**
	 * Save data from controls into parser rule
	 */
	public void save()
	{
	   file.setFile(labelFileName.getText().trim());
	   file.setEncoding((comboFileEncoding.getSelectionIndex() == 0) ? null : comboFileEncoding.getText());
	   file.setPreallocated(checkPreallocated.getSelection());
	   file.setSnapshot(checkSnapshot.getSelection());
	   file.setKeepOpen(checkKeepOpen.getSelection());
	   file.setIgnoreModificationTime(checkIgnoreModificationTime.getSelection());
	   file.setRescan(checkRescan.getSelection());
	   file.setFollowSymlinks(checkFollowSymlinks.getSelection());
      file.setRemoveEscapeSequences(checkRemoveEscapeSequences.getSelection());
	}

   /**
    * @return parser type
    */
   public LogParserType getParserType()
   {
      return editor.getParserType();
   }
   
   /**
    * Get file name
    * 
    * @return file name
    */
   public String getFile()
   {
      return labelFileName.getText();
   }
}
