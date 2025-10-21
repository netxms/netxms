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
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.Bullet;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.LineStyleEvent;
import org.eclipse.swt.custom.LineStyleListener;
import org.eclipse.swt.custom.ST;
import org.eclipse.swt.custom.StyleRange;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.custom.VerifyKeyListener;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.TextTransfer;
import org.eclipse.swt.dnd.TransferData;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.KeyAdapter;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GlyphMetrics;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
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
import org.netxms.nxmc.base.widgets.helpers.StyledTextUndoManager;
import org.netxms.nxmc.keyboard.KeyBindingManager;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * NXSL script editor
 */
public class ScriptEditor extends CompositeWithMessageArea
{
   private final I18n i18n = LocalizationHelper.getI18n(ScriptEditor.class);

   private static final Color ERROR_COLOR = new Color(Display.getDefault(), 255, 0, 0);
   private static final Color WARNING_COLOR = new Color(Display.getDefault(), 224, 224, 0);
   private static final Color COMMENT_COLOR = new Color(Display.getDefault(), 63, 127, 95);
   private static final Color STRING_COLOR = new Color(Display.getDefault(), 42, 0, 255);
   private static final Color KEYWORD_COLOR = new Color(Display.getDefault(), 127, 0, 85);
   private static final Color FUNCTION_COLOR = new Color(Display.getDefault(), 100, 90, 0);

   private static final Set<String> KEYWORDS = Set.of("abort", "and", "array", "boolean", "break", "case", "catch", "const", "continue", "default", "do", "else", "exit", "false", "for", "foreach",
         "function", "global", "if", "ilike", "imatch", "import", "in", "int32", "int64", "like", "match", "new", "not", "null", "or", "real", "return", "select", "string", "switch", "true",
         "try", "uint32", "uint64", "when", "while", "with", "FALSE", "NULL", "TRUE");

   private static final Set<String> BUILTIN_FUNCTIONS = Set.of("_exit", "assert", "chr", "classof", "d2x", "ord", "print", "println", "range", "sleep", "time", "trace", "typeof", "x2d",
         "FormatMetricPrefix", "FormatNumber", "GetCurrentTime", "GetCurrentTimeMs", "GetMonotonicClockTime", "GetThreadPoolNames", "JsonParse", "ReadPersistentStorage", "SecondsToUptime",
         "WritePersistentStorage", "Base64::Decode", "Base64::Encode", "Crypto::MD5", "Crypto::SHA1", "Crypto::SHA256", "Math::Abs", "Math::Acos", "Math::Asin", "Math::Atan", "Math::Atan2",
         "Math::Atanh", "Math::Average", "Math::Ceil", "Math::Cos", "Math::Cosh", "Math::Exp", "Math::Floor", "Math::Log", "Math::Log10", "Math::Max", "Math::MeanAbsoluteDeviation", "Math::Min",
         "Math::Pow", "Math::Pow10", "Math::Random", "Math::Round", "Math::Sin", "Math::Sinh", "Math::StandardDeviation", "Math::Sqrt", "Math::Sum", "Math::Tan", "Math::Tanh", "Math::Weierstrass",
         "Net::GetLocalHostName", "Net::ResolveAddress", "Net::ResolveHostname");

   private Composite content;
   private StyledText editor;
   private LineStyleListener lineNumberingStyleListener;
   private ModifyListener lineNumberModifyListener;
   private Set<String> functions = new HashSet<String>(BUILTIN_FUNCTIONS);
	private Set<String> variables = new HashSet<String>(0);
   private Set<String> constants = new HashSet<String>(0);
	private String hintText;
	private Composite hintArea;
	private Text hintTextControl = null;
	private Label hintsExpandButton = null;
	private Button compileButton;
   private KeyBindingManager keyBindingManager;
   private StyledTextUndoManager undoManager;
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

      editor = new StyledText(content, editorStyle | SWT.MULTI);
      editor.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      lineNumberingStyleListener = new LineStyleListener() {
         public void lineGetStyle(LineStyleEvent e)
         {
            e.bulletIndex = editor.getLineAtOffset(e.lineOffset);

            // Set the style, 12 pixels wide for each digit
            StyleRange style = new StyleRange();
            style.metrics = new GlyphMetrics(0, 0, Integer.toString(Math.max(editor.getLineCount(), 999)).length() * 12);

            // Create and set the bullet
            e.bullet = new Bullet(ST.BULLET_NUMBER, style);
         }
      };
      lineNumberModifyListener = new ModifyListener() {
         private int lineCount = 0;

         @Override
         public void modifyText(ModifyEvent e)
         {
            int currLineCount = editor.getLineCount();
            if (currLineCount != lineCount)
            {
               editor.redraw(); // will cause line numbers to be redrawn
               lineCount = currLineCount;
            }
         }
      };
      editor.addLineStyleListener(lineNumberingStyleListener);
      editor.addModifyListener(lineNumberModifyListener);

      editor.addLineStyleListener((e) -> styleLine(e));

      editor.setFont(JFaceResources.getTextFont());
      editor.setWordWrap(false);

      editor.addVerifyKeyListener(new VerifyKeyListener() {
         public void verifyKey(VerifyEvent e)
         {
            if (e.keyCode == SWT.TAB)
            {
               if ((e.stateMask & SWT.SHIFT) != 0)
               {
                  e.doit = onShiftTabPressed();
               }
               else
               {
                  e.doit = onTabPressed();
               }
            }
         }
      });
      editor.addTraverseListener((e) -> {
         if (e.detail == SWT.TRAVERSE_TAB_PREVIOUS)
         {
            e.doit = false; // allows verifyKey listener to fire
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

      undoManager = new StyledTextUndoManager(editor);
	}

   /**
    * Set syntax highlighting
    *
    * @param e line style event
    */
   private void styleLine(LineStyleEvent e)
   {
      String text = e.lineText;
      ArrayList<StyleRange> styles = new ArrayList<StyleRange>();
      int lineOffset = e.lineOffset;
      int length = text.length();

      // Check if line is part of multiline comment
      String fullText = editor.getText();
      boolean inComment = false;
      int commentStart = fullText.lastIndexOf("/*", lineOffset);
      if (commentStart >= 0)
      {
         int commentEnd = fullText.lastIndexOf("*/", lineOffset);
         if ((commentEnd < 0) || (commentEnd < commentStart))
            inComment = true;
      }

      if (inComment)
      {
         // Find end of comment if any
         int endComment = text.indexOf("*/");
         if (endComment >= 0)
         {
            StyleRange style = new StyleRange();
            style.start = lineOffset;
            style.length = endComment + 2;
            style.foreground = COMMENT_COLOR;
            styles.add(style);
         }
         else
         {
            StyleRange style = new StyleRange();
            style.start = lineOffset;
            style.length = length;
            style.foreground = COMMENT_COLOR;
            styles.add(style);
            e.styles = styles.toArray(new StyleRange[styles.size()]);
            return;
         }
      }

      int pos = 0;
      while(pos < length)
      {
         // Skip whitespace
         while((pos < length) && Character.isWhitespace(text.charAt(pos)))
            pos++;
         if (pos >= length)
            break;

         // Check for comments
         if ((pos < length - 1) && (text.charAt(pos) == '/'))
         {
            if (text.charAt(pos + 1) == '/')
            {
               StyleRange style = new StyleRange();
               style.start = lineOffset + pos;
               style.length = length - pos;
               style.foreground = COMMENT_COLOR;
               styles.add(style);
               break;
            }
            else if (text.charAt(pos + 1) == '*')
            {
               int end = text.indexOf("*/", pos + 2);
               StyleRange style = new StyleRange();
               style.start = lineOffset + pos;
               style.length = (end >= 0) ? end - pos + 2 : length - pos;
               style.foreground = COMMENT_COLOR;
               styles.add(style);
               pos = (end >= 0) ? end + 2 : length;
               continue;
            }
         }

         // Check for strings
         if (text.charAt(pos) == '"')
         {
            int end = pos + 1;
            while((end < length) && (text.charAt(end) != '"'))
            {
               if ((text.charAt(end) == '\\') && (end < length - 1))
                  end++;
               end++;
            }
            if (end < length)
               end++;
            StyleRange style = new StyleRange();
            style.start = lineOffset + pos;
            style.length = end - pos;
            style.foreground = STRING_COLOR;
            styles.add(style);
            pos = end;
            continue;
         }

         // Check for keywords and functions
         if (Character.isJavaIdentifierStart(text.charAt(pos)))
         {
            int end = pos + 1;
            while((end < length) && (Character.isJavaIdentifierPart(text.charAt(end)) || (text.charAt(end) == ':')))
               end++;
            String word = text.substring(pos, end);

            StyleRange style = new StyleRange();
            style.start = lineOffset + pos;
            style.length = end - pos;

            if (KEYWORDS.contains(word))
            {
               style.foreground = KEYWORD_COLOR;
               style.fontStyle = SWT.BOLD;
            }
            else if (functions.contains(word))
            {
               style.foreground = FUNCTION_COLOR;
               style.fontStyle = SWT.BOLD;
            }

            if (style.foreground != null)
               styles.add(style);

            pos = end;
            continue;
         }

         pos++;
      }

      e.styles = styles.toArray(new StyleRange[styles.size()]);
   }

   /**
    * Handle Tab press
    *
    * @return true to continue with default processing, false to cancel it
    */
   private boolean onTabPressed()
   {
      int length = editor.getSelectionCount();
      if (length == 0)
         return true;

      Point range = editor.getSelectionRange();
      int start = range.x;

      String txt = editor.getText();
      while(start > 0 && txt.charAt(start - 1) != '\n')
      {
         start--;
         length++;
      }

      int replaceLength = length;
      editor.setSelectionRange(start, length);
      editor.showSelection();

      String selectedText = editor.getSelectionText();
      String[] lines = selectedText.split("\n");
      StringBuilder newText = new StringBuilder();

      for(int x = 0; x < lines.length; x++)
      {
         if (x > 0)
         {
            newText.append('\n');
         }
         newText.append('\t');
         length++;
         newText.append(lines[x]);
      }
      newText.append('\n');

      editor.replaceTextRange(start, replaceLength, newText.toString());
      editor.setSelectionRange(start, length);
      editor.showSelection();

      return false;
   }

   /**
    * Handle Shift+Tab press
    *
    * @return true to continue with default processing, false to cancel it
    */
   private boolean onShiftTabPressed()
   {
      int length = editor.getSelectionCount();
      if (length == 0)
         return true;

      Point range = editor.getSelectionRange();
      int start = range.x;

      String txt = editor.getText();
      while(start > 0 && txt.charAt(start - 1) != '\n')
      {
         --start;
         ++length;
      }

      int replaceLength = length;
      editor.setSelectionRange(start, length);
      editor.showSelection();

      String selectedText = editor.getSelectionText();
      String[] lines = selectedText.split("\n");
      StringBuilder newText = new StringBuilder();

      for(int x = 0; x < lines.length; x++)
      {
         if (x > 0)
         {
            newText.append('\n');
         }
         if (lines[x].charAt(0) == '\t')
         {
            newText.append(lines[x].substring(1));
            length--;
         }
         else if (lines[x].startsWith(" "))
         {
            newText.append(lines[x].substring(1));
            length--;
         }
         else
         {
            newText.append(lines[x]);
         }
      }
      newText.append('\n');

      editor.replaceTextRange(start, replaceLength, newText.toString());
      editor.setSelectionRange(start, length);
      editor.showSelection();

      return false;
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

      int selectionLength = editor.getSelectionRange().y;
      actionCut.setEnabled(selectionLength > 0);
      actionCopy.setEnabled(selectionLength > 0);
      actionPaste.setEnabled(canPaste());
   }

	/**
	 * Get underlying text widget
	 * @return text widget
	 */
	public StyledText getTextWidget()
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
      undoManager.reset();
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
		this.functions = functions;
	}
	
	/**
	 * Add functions
	 * 
	 * @param fc
	 */
	public void addFunctions(Collection<String> fc)
	{
		functions.addAll(fc);
	}

	/**
	 * @param variables the variables to set
	 */
	public void setVariables(Set<String> variables)
	{
		this.variables = variables;
	}

	/**
	 * Add variables
	 * 
	 * @param vc
	 */
	public void addVariables(Collection<String> vc)
	{
		variables.addAll(vc);
	}

   /**
    * @param constants new constant set
    */
   public void setConstants(Set<String> constants)
   {
      this.constants = constants;
   }

   /**
    * Add constants
    * 
    * @param cc constants to add
    */
   public void addConstants(Collection<String> cc)
   {
      constants.addAll(cc);
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
	   if (show)
      {
         editor.addLineStyleListener(lineNumberingStyleListener);
         editor.addModifyListener(lineNumberModifyListener);
      }
	   else
      {
         editor.removeLineStyleListener(lineNumberingStyleListener);
         editor.removeModifyListener(lineNumberModifyListener);
      }
      editor.redraw();
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
                  editor.setLineBackground(0, editor.getLineCount(), null);

                  if (result.success)
                  {
                     clearMessages();
                     addMessage(MessageArea.SUCCESS, i18n.tr("Script compiled successfully"), true);
                  }
                  else
                  {
                     clearMessages();
                     addMessage(MessageArea.ERROR, result.errorMessage, true);
                     editor.setLineBackground(result.errorLine - 1, 1, ERROR_COLOR);
                  }

                  for(ScriptCompilationWarning w : result.warnings)
                  {
                     addMessage(MessageArea.WARNING, i18n.tr("Warning in line {0}: {1}", Integer.toString(w.lineNumber), w.message), true);
                     if (w.lineNumber != result.errorLine)
                        editor.setLineBackground(w.lineNumber - 1, 1, WARNING_COLOR);
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
      final int maxLine = editor.getLineCount();
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
      editor.setLineBackground(0, editor.getLineCount(), null);
      editor.setLineBackground(lineNumber - 1, 1, ERROR_COLOR);
   }

   /**
    * Highlight warning line.
    *
    * @param lineNumber line number to highlight
    */
   public void highlightWarningLine(int lineNumber)
   {
      editor.setLineBackground(lineNumber - 1, 1, WARNING_COLOR);
   }

   /**
    * Clear line highlighting
    */
   public void clearHighlighting()
   {
      editor.setLineBackground(0, editor.getLineCount(), null);
   }

   /**
    * Get number of lines in editor.
    *
    * @return number of lines in editor
    */
   public int getLineCount()
   {
      return editor.getLineCount();
   }

   /**
    * Get current line number.
    *
    * @return current line number
    */
   public int getCurrentLine()
   {
      return editor.getLineAtOffset(editor.getCaretOffset()) + 1;
   }

   /**
    * Set caret to line with given number.
    *
    * @param lineNumber line number
    */
   public void setCaretToLine(int lineNumber)
   {
      editor.setCaretOffset(editor.getOffsetAtLine(lineNumber - 1));
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
      int line = editor.getLineAtOffset(editor.getCaretOffset());
      int start = editor.getOffsetAtLine(line);
      if (line == editor.getLineCount() - 1)
      {
         editor.replaceTextRange(start, editor.getCharCount() - start, "");
      }
      else
      {
         editor.replaceTextRange(start, editor.getOffsetAtLine(line + 1) - start, "");
      }
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
      Clipboard cb = new Clipboard(Display.getCurrent());
      TransferData[] available = WidgetHelper.getAvailableTypes(cb);
      boolean enabled = false;
      for(int i = 0; i < available.length; i++)
      {
         if (TextTransfer.getInstance().isSupportedType(available[i]))
         {
            enabled = true;
            break;
         }
      }
      cb.dispose();
      return enabled;
   }
}
