/**
 * 
 */
package org.netxms.ui.eclipse.console.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.resources.Theme;
import org.netxms.ui.eclipse.console.resources.ThemeElement;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Theme editing dialog
 */
public class ThemeEditDialog extends Dialog
{
   public static final int COLUMN_TAG = 0;
   public static final int COLUMN_FOREGROUND = 1;
   public static final int COLUMN_BACKGROUND = 2;
   public static final int COLUMN_FONT = 3;

   private Theme theme;
   private LabeledText name;
   private TableViewer viewer;

   /**
    * @param parentShell
    */
   public ThemeEditDialog(Shell parentShell, Theme theme)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.theme = theme;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Edit Theme");
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      try
      {
         newShell.setSize(settings.getInt("ThemeEditDialog.cx"), settings.getInt("ThemeEditDialog.cy")); //$NON-NLS-1$ //$NON-NLS-2$
      }
      catch(NumberFormatException e)
      {
         newShell.setSize(670, 600);
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      dialogArea.setLayout(layout);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel("Name");
      name.setText(theme.getName());
      name.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Theme elements");
      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      label.setLayoutData(gd);

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
      setupViewer();
      viewer.setInput(theme.getTags());
      viewer.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      return dialogArea;
   }

   /**
    * Setup viewer
    */
   private void setupViewer()
   {
      TableColumn column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText("Tag");
      column.setWidth(400);

      column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText("FG");
      column.setWidth(50);

      column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText("BG");
      column.setWidth(50);

      column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText("Font");
      column.setWidth(150);

      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ColorListLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((String)e1).compareToIgnoreCase((String)e2);
         }
      });

      viewer.getTable().setLinesVisible(true);
      viewer.getTable().setHeaderVisible(true);
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      settings.put("ThemeEditDialog.cx", size.x); //$NON-NLS-1$
      settings.put("ThemeEditDialog.cy", size.y); //$NON-NLS-1$
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
   @Override
   protected void cancelPressed()
   {
      saveSettings();
      super.cancelPressed();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      saveSettings();
      super.okPressed();
   }

   /**
    * Label provider for color list
    */
   private class ColorListLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         switch(columnIndex)
         {
            case COLUMN_TAG:
               return (String)element;
            case COLUMN_FONT:
               ThemeElement te = theme.getElement((String)element);
               if ((te.fontName == null) || te.fontName.isEmpty())
                  return "";
               return te.fontName + " " + Integer.toString(te.fontHeight) + "pt";
            default:
               return "";
         }
      }
   }
}
