package org.netxms.ui.eclipse.widgets;

import org.eclipse.jface.text.IFindReplaceTarget;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;

public class TextEditor extends Composite implements IFindReplaceTarget
{
   private Text editor;
   
   public TextEditor(Composite parent, int style, int editorSyle)
   {
      super(parent, style);
      setLayout(new FillLayout());
      editor = new Text(this, editorSyle);
   }
   
   @Override
   public boolean canPerformFind()
   {
      return true;
   }

   @Override
   public int findAndSelect(int widgetOffset, String findString, boolean searchForward, boolean caseSensitive, boolean wholeWord)
   {
      String content;
      if (widgetOffset == -1)
      {
         content = editor.getText();
      }
      else if (searchForward)
      {
         content = editor.getText().substring(widgetOffset);
      }
      else
      {
         content = editor.getText().substring(0, widgetOffset + 1);
      }
      if (caseSensitive)
      {
         content = content.toUpperCase();
         findString = findString.toUpperCase();
      }
      int index = searchForward ? content.indexOf(findString) : content.lastIndexOf(findString);
      if (index != -1)
      {
         if ((widgetOffset >= 0) && searchForward)
            index += widgetOffset;
         editor.setSelection(index, index + findString.length());
      }
      return index;
   }   

   @Override
   public Point getSelection()
   {
      Point s = editor.getSelection();
      s.y -= s.x; // convert to length
      return s;
   }

   @Override
   public String getSelectionText()
   {
      return editor.getSelectionText();
   }

   @Override
   public boolean isEditable()
   {
      return true;
   }

   @Override
   public void replaceSelection(String text)
   {      
      String content = editor.getText();
      Point s = getSelection();
      String start = content.substring(0, s.x);
      String tail = content.substring(s.y);
      editor.setText(start + text + tail);
   }
   
   public Text getTextControl()
   {
      return editor;
   }

   public void addModifyListener(ModifyListener modifyListener)
   {
      editor.addModifyListener(modifyListener);      
   }

   public String getText()
   {
      return editor.getText();
   }

   public void setText(String text)
   {
      editor.setText(text);
   }
}
