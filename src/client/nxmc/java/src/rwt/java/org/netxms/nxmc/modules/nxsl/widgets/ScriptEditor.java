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

import java.util.Collection;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.window.Window;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.KeyAdapter;
import org.eclipse.swt.events.KeyEvent;
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
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.ScrollBar;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.ScriptCompilationResult;
import org.netxms.client.ScriptCompilationWarning;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.CompositeWithMessageArea;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.keyboard.KeyBindingManager;
import org.netxms.nxmc.keyboard.KeyStroke;
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
   private Text editor;
	private String hintText;
	private Composite hintArea;
	private Text hintTextControl = null;
	private Label hintsExpandButton = null;
	private Button compileButton;
   private KeyBindingManager keyBindingManager;
   private Action actionGoToLine;
   private Action actionSelectAll;
   private Action actionCut;
   private Action actionCopy;
   private Action actionPaste;
   private Action actionDeleteLine;

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

      editor = new Text(content, editorStyle | SWT.MULTI);
      editor.setData(RWT.CUSTOM_VARIANT, "monospace");
      editor.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      editor.addTraverseListener((e) -> {
         if (e.detail == SWT.TRAVERSE_TAB_NEXT)
         {
            e.doit = false; // Prevent default tab traversal

            Point selection = editor.getSelection();
            if (selection.x == selection.y)
            {
               // Insert tab character at current caret position
               editor.insert("\t");
               editor.setSelection(editor.getCaretPosition() + 1); // Move caret after the inserted tab
            }
            else
            {
               // Find each line starting within selection and indent it
               String text = editor.getText();
               int startLineOffset = text.lastIndexOf('\n', selection.x - 1);
               if (startLineOffset == -1)
                  startLineOffset = 0;
               else
                  startLineOffset++; // Move to the first character of the line
               int endLineOffset = text.indexOf('\n', selection.y - 1);
               if (endLineOffset == -1)
                  endLineOffset = text.length();
               StringBuilder newText = new StringBuilder();
               int lineStart = startLineOffset;
               while(lineStart < endLineOffset)
               {
                  int lineEnd = text.indexOf('\n', lineStart);
                  if (lineEnd == -1 || lineEnd > endLineOffset)
                     lineEnd = endLineOffset;
                  newText.append("\t").append(text, lineStart, lineEnd);
                  if (lineEnd < endLineOffset)
                     newText.append('\n');
                  lineStart = lineEnd + 1;
               }
               editor.setText(text.substring(0, startLineOffset) + newText.toString() + text.substring(endLineOffset));
               editor.setSelection(selection.x, selection.x + newText.length());
            }

            editor.setFocus();
         }
         else if (e.detail == SWT.TRAVERSE_TAB_PREVIOUS)
         {
            Point selection = editor.getSelection();
            if (selection.x == selection.y)
               return; // No selection - nothing to unindent

            e.doit = false; // Prevent default tab traversal

            // Find each line starting within selection and unindent it
            String text = editor.getText();
            int startLineOffset = text.lastIndexOf('\n', selection.x - 1);
            if (startLineOffset == -1)
               startLineOffset = 0;
            else
               startLineOffset++; // Move to the first character of the line
            int endLineOffset = text.indexOf('\n', selection.y - 1);
            if (endLineOffset == -1)
               endLineOffset = text.length();
            StringBuilder newText = new StringBuilder();
            int lineStart = startLineOffset;
            while(lineStart < endLineOffset)
            {
               int lineEnd = text.indexOf('\n', lineStart);
               if (lineEnd == -1 || lineEnd > endLineOffset)
                  lineEnd = endLineOffset;
               if (text.startsWith("\t", lineStart))
               {
                  newText.append(text, lineStart + 1, lineEnd);
               }
               else
               {
                  newText.append(text, lineStart, lineEnd);
               }
               if (lineEnd < endLineOffset)
                  newText.append('\n');
               lineStart = lineEnd + 1;
            }
            editor.setText(text.substring(0, startLineOffset) + newText.toString() + text.substring(endLineOffset));
            editor.setSelection(selection.x, selection.x + newText.length());

            editor.setFocus();
         }
      });

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

         editor.addControlListener(new ControlListener() {
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

      keyBindingManager = new KeyBindingManager();

      createActions();
      createContextMenu();

      editor.addKeyListener(new KeyAdapter() {
         @Override
         public void keyPressed(KeyEvent e)
         {
            keyBindingManager.processKeyStroke(new KeyStroke(e.stateMask, e.keyCode));
         }
      });
	}

   /**
    * Position "Compile" button
    */
   private void positionCompileButton()
   {
      compileButton.moveAbove(null);
      Point location = editor.getLocation();
      int editorWidth = editor.getSize().x;
      ScrollBar sb = editor.getVerticalBar();
      if (sb != null)
         editorWidth -= sb.getSize().x;
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
    * Create actions
    */
   private void createActions()
   {
      actionGoToLine = new Action(i18n.tr("&Go to line..."), ResourceManager.getImageDescriptor("icons/nxsl/go-to-line.png")) {
         @Override
         public void run()
         {
            goToLine();
         }
      };
      keyBindingManager.addBinding("M1+G", actionGoToLine);

      actionSelectAll = new Action(i18n.tr("Select &all")) {
         @Override
         public void run()
         {
            selectAll();
         }
      };
      keyBindingManager.addBinding("M1+A", actionSelectAll);

      actionDeleteLine = new Action(i18n.tr("&Delete line")) {
         @Override
         public void run()
         {
            deleteLine();
         }
      };
      keyBindingManager.addBinding("M1+M2+K", actionDeleteLine);

      // Do not require key bindings (handled by StyledText widget)
      actionCut = new Action(i18n.tr("C&ut") + "\t" + KeyStroke.parse("M1+X").toString(), SharedIcons.CUT) {
         @Override
         public void run()
         {
            cut();
         }
      };
      actionCut.setEnabled(false);

      actionCopy = new Action(i18n.tr("&Copy") + "\t" + KeyStroke.parse("M1+C").toString(), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copy();
         }
      };
      actionCopy.setEnabled(false);

      actionPaste = new Action(i18n.tr("&Paste") + "\t" + KeyStroke.parse("M1+P").toString(), SharedIcons.PASTE) {
         @Override
         public void run()
         {
            paste();
         }
      };
      actionPaste.setEnabled(false);
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

      // Create menu.
      Menu menu = menuMgr.createContextMenu(editor);
      editor.setMenu(menu);
   }

   /**
    * Fill context menu.
    *
    * @param manager menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionSelectAll);
      manager.add(new Separator());
      manager.add(actionCopy);
      manager.add(actionCut);
      manager.add(actionPaste);
      manager.add(new Separator());
      manager.add(actionDeleteLine);
      manager.add(new Separator());
      manager.add(actionGoToLine);

      int selectionLength = editor.getSelectionCount();
      actionCut.setEnabled(selectionLength > 0);
      actionCopy.setEnabled(selectionLength > 0);
      actionPaste.setEnabled(canPaste());
   }

	/**
	 * Get underlying text widget
	 * @return text widget
	 */
   public Text getTextWidget()
	{
      return editor;
	}
	
	/**
	 * Set text for editing
	 * @param text
	 */
	public void setText(String text)
	{
      editor.setText(text != null ? text : "");
	}

	/**
	 * Get editor's content
	 * @return
	 */
	public String getText()
	{
      return editor.getText();
	}

	/**
	 * @param functions the functions to set
	 */
	public void setFunctions(Set<String> functions)
	{
	}
	
	/**
	 * Add functions
	 * 
	 * @param fc
	 */
	public void addFunctions(Collection<String> fc)
	{
	}

	/**
	 * @param variables the variables to set
	 */
	public void setVariables(Set<String> variables)
	{
	}

	/**
	 * Add variables
	 * 
	 * @param vc
	 */
	public void addVariables(Collection<String> vc)
	{
	}

   /**
    * @param constants new constant set
    */
   public void setConstants(Set<String> constants)
   {
   }

   /**
    * Add constants
    * 
    * @param cc constants to add
    */
   public void addConstants(Collection<String> cc)
   {
   }

	/**
	 * @return the functionsCache
	 */
	public String[] getFunctions()
	{
      return new String[0];
	}

	/**
	 * @return the variablesCache
	 */
	public String[] getVariables()
	{
      return new String[0];
	}

   /**
    * @return constants cache
    */
   public String[] getConstants()
   {
      return new String[0];
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
      Point p = editor.computeSize(wHint, hHint, changed);
		p.y += 4;
		return p;
	}

	/**
    * @see org.eclipse.swt.widgets.Composite#setFocus()
    */
   @Override
   public boolean setFocus()
   {
      return !editor.isDisposed() ? editor.setFocus() : super.setFocus();
   }

   /**
    * Show/hide line numbers in editor
    * 
    * @param show
    */
	public void showLineNumbers(boolean show)
	{
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
      editor.setEditable(false);
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
                  if (result.success)
                  {
                     clearMessages();
                     addMessage(MessageArea.SUCCESS, i18n.tr("Script compiled successfully"), true);
                  }
                  else
                  {
                     clearMessages();
                     addMessage(MessageArea.ERROR, result.errorMessage, true);
                  }

                  for(ScriptCompilationWarning w : result.warnings)
                  {
                     addMessage(MessageArea.WARNING, i18n.tr("Warning in line {0}: {1}", Integer.toString(w.lineNumber), w.message), true);
                  }

                  editor.setEditable(true);
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
   }

   /**
    * Highlight warning line.
    *
    * @param lineNumber line number to highlight
    */
   public void highlightWarningLine(int lineNumber)
   {
   }

   /**
    * Clear line highlighting
    */
   public void clearHighlighting()
   {
   }

   /**
    * Get number of lines in editor.
    *
    * @return number of lines in editor
    */
   public int getLineCount()
   {
      String text = editor.getText();
      if (text.isEmpty())
         return 0;
      return text.split(editor.getLineDelimiter()).length + 1;
   }

   /**
    * Get current line number.
    *
    * @return current line number
    */
   public int getCurrentLine()
   {
      return getLineAtOffset(editor.getCaretPosition());
   }

   /**
    * Get line number at given caret position
    *
    * @param offset caret position
    * @return line number
    */
   private int getLineAtOffset(int offset)
   {
      String text = editor.getText().substring(0, offset);
      if (text.isEmpty())
         return 0;
      return text.split(editor.getLineDelimiter()).length + 1;
   }

   /**
    * Set caret to line with given number.
    *
    * @param lineNumber line number
    */
   public void setCaretToLine(int lineNumber)
   {
      // TODO: implement if possible
   }

   /**
    * Select whole text
    */
   public void selectAll()
   {
      editor.selectAll();
   }

   /**
    * Delete current line
    */
   public void deleteLine()
   {
      String d = editor.getLineDelimiter();
      char de = d.charAt(d.length() - 1);

      String text = editor.getText();
      int start = editor.getCaretPosition();
      while((start > 0) && (text.charAt(start) != de))
         start--;
      if (text.charAt(start) == de)
         start++;

      int end = editor.getCaretPosition();
      while((end < text.length()) && (text.charAt(end) != de))
         end++;

      editor.setText(text.substring(0, start) + text.substring(end));
   }

   /**
    * Cut selected text
    */
   public void cut()
   {
      editor.cut();
   }

   /**
    * Copy selected text
    */
   public void copy()
   {
      editor.copy();
   }

   /**
    * Paste text from clipboard
    */
   public void paste()
   {
      editor.paste();
   }

   /**
    * Check if paste action can work
    * 
    * @return
    */
   public boolean canPaste()
   {
      return true; // TODO: can we determine if client-side clipboard is available?
   }
}
