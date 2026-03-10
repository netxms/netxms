/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.nxsl.widgets;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.window.Window;
import org.eclipse.rap.json.JsonArray;
import org.eclipse.rap.json.JsonObject;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.remote.AbstractOperationHandler;
import org.eclipse.rap.rwt.remote.RemoteObject;
import org.eclipse.rap.rwt.widgets.WidgetUtil;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.ScriptCompilationResult;
import org.netxms.client.ScriptCompilationWarning;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.CompositeWithMessageArea;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * NXSL script editor
 */
public class ScriptEditor extends CompositeWithMessageArea
{
   private final I18n i18n = LocalizationHelper.getI18n(ScriptEditor.class);

   private Composite content;
   private Composite editorContainer;
   private RemoteObject remoteObject;
   private String cachedText = "";
   private boolean editable = true;
   private Set<String> functions = new HashSet<>();
   private Set<String> variables = new HashSet<>();
   private Set<String> constants = new HashSet<>();
   private List<Integer> errorLines = new ArrayList<>();
   private List<Integer> warningLines = new ArrayList<>();
   private List<ModifyListener> modifyListeners = new ArrayList<>();
   private List<MouseListener> mouseListeners = new ArrayList<>();
   private String hintText;
   private Composite hintArea;
   private Text hintTextControl = null;
   private Label hintsExpandButton = null;
   private Button compileButton;

   /**
    * @param parent
    * @param style
    * @param editorStyle
    */
   public ScriptEditor(Composite parent, int style, int editorStyle)
   {
      this(parent, style, editorStyle, null, true);
   }

   /**
    * 
    * @param parent
    * @param style
    * @param editorStyle
    * @param hints
    * @param showCompileButton
    */
   public ScriptEditor(Composite parent, int style, int editorStyle, String hints)
   {
      this(parent, style, editorStyle, hints, true);
   }

   /**
    * 
    * @param parent
    * @param style
    * @param editorStyle
    * @param showCompileButton
    */
   public ScriptEditor(Composite parent, int style, int editorStyle, boolean showCompileButton)
   {
      this(parent, style, editorStyle, null, showCompileButton);
   }

	/**
	 * @param parent
	 * @param style
	 * @param editorStyle
	 * @param hints
	 * @param showCompileButton
	 */
	public ScriptEditor(Composite parent, int style, int editorStyle, String hints, boolean showCompileButton)
	{
		super(parent, style);

      hintText = hints;

      content = new Composite(this, SWT.NONE);
      setContent(content);

		GridLayout layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.verticalSpacing = 0;
      content.setLayout(layout);

      if (hints != null)
      {
         createHintsArea();
      }

      editorContainer = new Composite(content, SWT.NONE);
      editorContainer.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      remoteObject = RWT.getUISession().getConnection().createRemoteObject("netxms.CodeMirror");
      remoteObject.set("parent", WidgetUtil.getId(editorContainer));
      remoteObject.set("lineNumbers", true);
      remoteObject.setHandler(new AbstractOperationHandler() {
         @Override
         public void handleNotify(String event, JsonObject properties)
         {
            if ("textChanged".equals(event))
            {
               cachedText = properties.get("text").asString();
               fireModifyListeners();
            }
            else if ("textSync".equals(event))
            {
               cachedText = properties.get("text").asString();
            }
            else if ("mouseDown".equals(event))
            {
               fireMouseDownListeners();
            }
         }
      });
      remoteObject.listen("textChanged", true);
      remoteObject.listen("textSync", true);
      remoteObject.listen("mouseDown", true);

      if (showCompileButton)
      {
         final Image compileImage = ResourceManager.getImage("icons/compile.png");
         compileButton = new Button(content, SWT.PUSH | SWT.FLAT);
         compileButton.setToolTipText("Compile script");
         compileButton.setImage(compileImage);
         compileButton.addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               compileImage.dispose();
            }
         });

         GridData gd = new GridData();
         gd.exclude = true;
         compileButton.setLayoutData(gd);

         editorContainer.addControlListener(new ControlListener() {
            @Override
            public void controlResized(ControlEvent e)
            {
               positionCompileButton();
            }

            @Override
            public void controlMoved(ControlEvent e)
            {
               positionCompileButton();
            }
         });

         compileButton.addSelectionListener(new SelectionListener() {
            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               widgetSelected(e);
            }

            @Override
            public void widgetSelected(SelectionEvent e)
            {
               compileScript();
            }
         });

         compileButton.setSize(compileButton.computeSize(SWT.DEFAULT, SWT.DEFAULT));
         positionCompileButton();
      }
   }

   /**
    * Position "Compile" button
    */
   private void positionCompileButton()
   {
      compileButton.moveAbove(null);
      Point location = editorContainer.getLocation();
      int editorWidth = editorContainer.getSize().x;
      compileButton.setLocation(location.x + editorWidth - compileButton.getSize().x - 3, location.y + 3);
   }

	/**
	 * Create hints area
	 */
	private void createHintsArea()
	{
      hintArea = new Composite(content, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      layout.numColumns = 2;
      hintArea.setLayout(layout);
      hintArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      hintArea.setBackground(ThemeEngine.getBackgroundColor("MessageBar"));

      CLabel hintsTitle = new CLabel(hintArea, SWT.NONE);
      hintsTitle.setBackground(hintArea.getBackground());
      hintsTitle.setForeground(ThemeEngine.getForegroundColor("MessageBar"));
      hintsTitle.setImage(SharedIcons.IMG_INFORMATION);
      hintsTitle.setText(i18n.tr("Hints"));
      hintsTitle.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      hintsTitle.addMouseListener(new MouseAdapter() {
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            if (e.button == 1)
               toggleHints();
         }
      });

      hintsExpandButton = new Label(hintArea, SWT.NONE);
      hintsExpandButton.setBackground(hintArea.getBackground());
      hintsExpandButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      hintsExpandButton.setImage(SharedIcons.IMG_EXPAND);
      hintsExpandButton.setToolTipText(i18n.tr("Hide message"));
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      hintsExpandButton.setLayoutData(gd);
      hintsExpandButton.addMouseListener(new MouseListener() {
         private boolean doAction = false;

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            if (e.button == 1)
               doAction = false;
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 1)
               doAction = true;
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
            if ((e.button == 1) && doAction)
               toggleHints();
         }
      });
      
      Label separator = new Label(content, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
   }

   /**
    * Fire registered modify listeners.
    */
   private void fireModifyListeners()
   {
      Event e = new Event();
      e.display = getDisplay();
      e.widget = this;
      ModifyEvent me = new ModifyEvent(e);
      for(ModifyListener l : modifyListeners)
         l.modifyText(me);
   }

   /**
    * Fire registered mouse down listeners.
    */
   private void fireMouseDownListeners()
   {
      Event e = new Event();
      e.display = getDisplay();
      e.widget = this;
      e.button = 1;
      MouseEvent me = new MouseEvent(e);
      for(MouseListener l : mouseListeners)
         l.mouseDown(me);
   }

   /**
    * Convert a set of strings to a JSON array.
    */
   private static JsonArray toJsonArray(Set<String> set)
   {
      JsonArray array = new JsonArray();
      for(String s : set)
         array.add(s);
      return array;
   }

   /**
    * Convert a list of integers to a JSON array.
    */
   private static JsonArray toJsonArray(List<Integer> list)
   {
      JsonArray array = new JsonArray();
      for(Integer n : list)
         array.add(n.intValue());
      return array;
   }

   /**
    * Add modify listener to the editor.
    *
    * @param listener modify listener
    */
   public void addModifyListener(ModifyListener listener)
   {
      modifyListeners.add(listener);
   }

   /**
    * Set editor's editable state.
    *
    * @param editable true to make editor editable
    */
   public void setEditable(boolean editable)
   {
      this.editable = editable;
      remoteObject.set("readOnly", !editable);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#addMouseListener(org.eclipse.swt.events.MouseListener)
    */
   @Override
   public void addMouseListener(MouseListener listener)
   {
      mouseListeners.add(listener);
   }
	
	/**
	 * Set text for editing
	 * @param text
	 */
	public void setText(String text)
	{
      cachedText = (text != null) ? text : "";
      remoteObject.set("text", cachedText);
	}

	/**
	 * Get editor's content
	 * @return
	 */
	public String getText()
	{
      return cachedText;
	}

	/**
	 * @param functions the functions to set
	 */
	public void setFunctions(Set<String> functions)
	{
      this.functions = functions;
      remoteObject.set("functions", toJsonArray(functions));
	}

	/**
	 * Add functions
	 *
	 * @param fc functions to add
	 */
	public void addFunctions(Collection<String> fc)
	{
      functions.addAll(fc);
      remoteObject.set("functions", toJsonArray(functions));
	}

	/**
	 * @param variables the variables to set
	 */
	public void setVariables(Set<String> variables)
	{
      this.variables = variables;
      remoteObject.set("variables", toJsonArray(variables));
	}

	/**
	 * Add variables
	 *
	 * @param vc variables to add
	 */
	public void addVariables(Collection<String> vc)
	{
      variables.addAll(vc);
      remoteObject.set("variables", toJsonArray(variables));
	}

   /**
    * @param constants new constant set
    */
   public void setConstants(Set<String> constants)
   {
      this.constants = constants;
      remoteObject.set("constants", toJsonArray(constants));
   }

   /**
    * Add constants
    *
    * @param cc constants to add
    */
   public void addConstants(Collection<String> cc)
   {
      constants.addAll(cc);
      remoteObject.set("constants", toJsonArray(constants));
   }

	/**
	 * @return the functions
	 */
	public String[] getFunctions()
	{
      return functions.toArray(new String[0]);
	}

	/**
	 * @return the variables
	 */
	public String[] getVariables()
	{
      return variables.toArray(new String[0]);
	}

   /**
    * @return the constants
    */
   public String[] getConstants()
   {
      return constants.toArray(new String[0]);
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
      Point p = editorContainer.computeSize(wHint, hHint, changed);
		p.y += 4;
		return p;
	}

	/**
    * @see org.eclipse.swt.widgets.Composite#setFocus()
    */
   @Override
   public boolean setFocus()
   {
      return !editorContainer.isDisposed() ? editorContainer.setFocus() : super.setFocus();
   }

   /**
    * Show/hide line numbers in editor
    * 
    * @param show
    */
	public void showLineNumbers(boolean show)
	{
      remoteObject.set("lineNumbers", show);
	}

	/**
	 * Toggle hints area
	 */
	private void toggleHints()
	{
	   if (hintTextControl != null)
	   {
	      hintTextControl.dispose();
	      hintTextControl = null;
	      hintsExpandButton.setImage(SharedIcons.IMG_EXPAND);
	   }
	   else
	   {
         hintTextControl = new Text(hintArea, SWT.MULTI | SWT.WRAP);
         hintTextControl.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));
         hintTextControl.setEditable(false);
         hintTextControl.setText(hintText);
         hintTextControl.setBackground(ThemeEngine.getBackgroundColor("MessageBar"));
         hintTextControl.setForeground(ThemeEngine.getForegroundColor("MessageBar"));
         hintsExpandButton.setImage(SharedIcons.IMG_COLLAPSE);
      }
      layout(true, true);
	}

   /**
    * Compile script
    */
   public void compileScript()
   {
      final String source = getText();
      setEditable(false);
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Compile script"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final ScriptCompilationResult result = session.compileScript(source, false);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  clearHighlighting();

                  if (result.success)
                  {
                     clearMessages();
                     addMessage(MessageArea.SUCCESS, i18n.tr("Script compiled successfully"), true);
                  }
                  else
                  {
                     clearMessages();
                     addMessage(MessageArea.ERROR, result.errorMessage, true);
                     highlightErrorLine(result.errorLine);
                  }

                  for(ScriptCompilationWarning w : result.warnings)
                  {
                     addMessage(MessageArea.WARNING, i18n.tr("Warning in line {0}: {1}", Integer.toString(w.lineNumber), w.message), true);
                     if (w.lineNumber != result.errorLine)
                        highlightWarningLine(w.lineNumber);
                  }

                  setEditable(true);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot compile script");
         }
      }.start();
   }

   /**
    * Go to line specified by user (will display interactive UI element)
    */
   public void goToLine()
   {
      final int maxLine = getLineCount();
      InputDialog dlg = new InputDialog(getShell(), i18n.tr("Go to Line"), String.format(i18n.tr("Enter line number (1..%d)"), maxLine), Integer.toString(getCurrentLine()),
            new IInputValidator() {
               @Override
               public String isValid(String newText)
               {
                  try
                  {
                     int n = Integer.parseInt(newText);
                     if ((n < 1) || (n > maxLine))
                        return i18n.tr("Number out of range");
                     return null;
                  }
                  catch(NumberFormatException e)
                  {
                     return i18n.tr("Invalid number");
                  }
               }
            });
      if (dlg.open() != Window.OK)
         return;

      setCaretToLine(Integer.parseInt(dlg.getValue()));
   }

   /**
    * Highlight error line.
    *
    * @param lineNumber line number to highlight
    */
   public void highlightErrorLine(int lineNumber)
   {
      errorLines.add(lineNumber);
      remoteObject.set("errorLines", toJsonArray(errorLines));
   }

   /**
    * Highlight warning line.
    *
    * @param lineNumber line number to highlight
    */
   public void highlightWarningLine(int lineNumber)
   {
      warningLines.add(lineNumber);
      remoteObject.set("warningLines", toJsonArray(warningLines));
   }

   /**
    * Clear line highlighting
    */
   public void clearHighlighting()
   {
      errorLines.clear();
      warningLines.clear();
      remoteObject.set("errorLines", toJsonArray(errorLines));
      remoteObject.set("warningLines", toJsonArray(warningLines));
   }

   /**
    * Get number of lines in editor.
    *
    * @return number of lines in editor
    */
   public int getLineCount()
   {
      if (cachedText.isEmpty())
         return 1;
      int count = 1;
      for(int i = 0; i < cachedText.length(); i++)
         if (cachedText.charAt(i) == '\n')
            count++;
      return count;
   }

   /**
    * Get current line number.
    *
    * @return current line number (1-based)
    */
   public int getCurrentLine()
   {
      return 1;
   }

   /**
    * Set caret to line with given number.
    *
    * @param lineNumber line number (1-based)
    */
   public void setCaretToLine(int lineNumber)
   {
      // Caret position is managed by CodeMirror on the client side
   }

   /**
    * Fill context menu. Can be overridden by subclasses to add extra items.
    * CodeMirror provides its own context menu, so this is only used for
    * server-side menu contributions added via SWT Menu on the container.
    *
    * @param manager menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
   }

   /**
    * @see org.eclipse.swt.widgets.Widget#dispose()
    */
   @Override
   public void dispose()
   {
      if (remoteObject != null)
         remoteObject.destroy();
      super.dispose();
   }
}
