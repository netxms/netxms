/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.Bullet;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.LineStyleEvent;
import org.eclipse.swt.custom.LineStyleListener;
import org.eclipse.swt.custom.ST;
import org.eclipse.swt.custom.StyleRange;
import org.eclipse.swt.custom.StyledText;
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
import org.eclipse.swt.widgets.ScrollBar;
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

   private static final Color ERROR_COLOR = new Color(Display.getDefault(), 255, 0, 0);
   private static final Color WARNING_COLOR = new Color(Display.getDefault(), 224, 224, 0);

   private Composite content;
   private StyledText editor;
   private LineStyleListener lineNumberingStyleListener;
   private ModifyListener lineNumberModifyListener;
	private Set<String> functions = new HashSet<String>(0);
	private Set<String> variables = new HashSet<String>(0);
   private Set<String> constants = new HashSet<String>(0);
	private String[] functionsCache = new String[0];
	private String[] variablesCache = new String[0];
   private String[] constantsCache = new String[0];
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
      this(parent, style, editorStyle, true, null, true);
   }

   /**
    * @param parent
    * @param style
    * @param editorStyle
    * @param showLineNumbers
    */
   public ScriptEditor(Composite parent, int style, int editorStyle, boolean showLineNumbers)
   {
      this(parent, style, editorStyle, showLineNumbers, null, true);
   }

   /**
    * 
    * @param parent
    * @param style
    * @param editorStyle
    * @param showLineNumbers
    * @param hints
    * @param showCompileButton
    */
   public ScriptEditor(Composite parent, int style, int editorStyle, boolean showLineNumbers, String hints)
   {
      this(parent, style, editorStyle, showLineNumbers, hints, true);
   }

   /**
    * 
    * @param parent
    * @param style
    * @param editorStyle
    * @param showLineNumbers
    * @param showCompileButton
    */
   public ScriptEditor(Composite parent, int style, int editorStyle, boolean showLineNumbers, boolean showCompileButton)
   {
      this(parent, style, editorStyle, showLineNumbers, null, showCompileButton);
   }

	/**
	 * @param parent
	 * @param style
	 * @param editorStyle
	 * @param showLineNumbers
	 * @param hints
	 * @param showCompileButton
	 */
	public ScriptEditor(Composite parent, int style, int editorStyle, boolean showLineNumbers, String hints, boolean showCompileButton)
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
		if (showLineNumbers)
      {
         editor.addLineStyleListener(lineNumberingStyleListener);
         editor.addModifyListener(lineNumberModifyListener);
      }

      editor.setFont(JFaceResources.getTextFont());
      editor.setWordWrap(false);

      if (showCompileButton)
      {
         final Image compileImage = ResourceManager.getImage("icons/compile.gif");
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
		functionsCache = functions.toArray(new String[functions.size()]);
	}
	
	/**
	 * Add functions
	 * 
	 * @param fc
	 */
	public void addFunctions(Collection<String> fc)
	{
		functions.addAll(fc);
		functionsCache = functions.toArray(new String[functions.size()]);
	}

	/**
	 * @param variables the variables to set
	 */
	public void setVariables(Set<String> variables)
	{
		this.variables = variables;
		variablesCache = variables.toArray(new String[variables.size()]);
	}

	/**
	 * Add variables
	 * 
	 * @param vc
	 */
	public void addVariables(Collection<String> vc)
	{
		variables.addAll(vc);
		variablesCache = variables.toArray(new String[variables.size()]);
	}

   /**
    * @param constants new constant set
    */
   public void setConstants(Set<String> constants)
   {
      this.constants = constants;
      constantsCache = constants.toArray(new String[constants.size()]);
   }

   /**
    * Add constants
    * 
    * @param cc constants to add
    */
   public void addConstants(Collection<String> cc)
   {
      constants.addAll(cc);
      constantsCache = constants.toArray(new String[constants.size()]);
   }

	/**
	 * @return the functionsCache
	 */
	public String[] getFunctions()
	{
		return functionsCache;
	}

	/**
	 * @return the variablesCache
	 */
	public String[] getVariables()
	{
		return variablesCache;
	}

   /**
    * @return constants cache
    */
   public String[] getConstants()
   {
      return constantsCache;
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
}
