/**
 * 
 */
package org.netxms.ui.eclipse.console.views.helpers;

import java.io.IOException;
import java.io.OutputStream;
import java.io.Reader;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

/**
 * Reader for terminal. Provides line editing and command history.
 */
public class TerminalReader extends Reader
{
	private final static byte[] BACKSPACE_SEQUENCE = { 8, 32, 8 };
	
	private final static int ESC_NORMAL = 0;
	private final static int ESC_START = 1;
	private final static int ESC_91 = 2;
	private final static int ESC_5X = 3;
	
	private Reader in;
	private OutputStream terminal;
	private StringBuilder command;
	private List<String> history = new ArrayList<String>();
	private int historyPos;
	
	/**
	 * Create new terminal reader connected to given input stream
	 * 
	 * @param in
	 */
	public TerminalReader(Reader in, OutputStream terminal)
	{
		this.in = in;
		this.terminal = terminal;
	}
	
	/* (non-Javadoc)
	 * @see java.io.Reader#close()
	 */
	@Override
	public void close() throws IOException
	{
		in.close();
	}

	/* (non-Javadoc)
	 * @see java.io.Reader#read(char[], int, int)
	 */
	@Override
	public int read(char[] cbuf, int off, int len) throws IOException
	{
		return in.read(cbuf, off, len);
	}
	
	/**
	 * @return
	 * @throws IOException 
	 */
	public String readLine() throws IOException
	{
		writeToTerminal("\u001b[33mnetxmsd:\u001b[0m "); //$NON-NLS-1$
		historyPos = history.size();
		int escState = ESC_NORMAL;
		command = new StringBuilder();
		while(true)
		{
			int ch = in.read();
			if (escState != ESC_NORMAL)
			{
				escState = processEscape(escState, ch);
				continue;
			}
			if (ch == '\n')
				break;
			switch(ch)
			{
				case 3:	// Ctrl+C
					clearInput();
					break;
				case 8:	// backspace
					if (command.length() > 0)
					{
						terminal.write(BACKSPACE_SEQUENCE);
						command.deleteCharAt(command.length() - 1);
					}
					break;
				case 12:	// Ctrl+L
					writeToTerminal("\u001b[2J\u001b[H\u001b[33mnetxmsd:\u001b[0m "); //$NON-NLS-1$
					writeToTerminal(command.toString());
					break;
				case 27: // ESC
					escState = ESC_START;
					break;
				case '\r':
					break;
				default:
					command.append((char)ch);
					break;
			}
		}
		
		final String line = command.toString().trim();
		if (!line.isEmpty() && ((history.size() == 0) || (history.get(history.size() - 1).compareTo(line) != 0)))
		{
			history.add(line);
		}
		return line;
	}
	
	/**
	 * Process escape character
	 * 
	 * @param ch current character
	 * @return new state
	 */
	private int processEscape(int state, int ch)
	{
		switch(state)
		{
			case ESC_START:
				return (ch == 91) ? ESC_91 : ESC_NORMAL;
			case ESC_91:
				switch(ch)
				{
					case 50:
					case 51:
						return ESC_5X;
					case 65:	// up arrow
						prevCommand();
						return ESC_NORMAL;						
					case 66:	// down arrow
						nextCommand();
						return ESC_NORMAL;						
					default:
						return ESC_NORMAL;
				}
			default:
				return ESC_NORMAL;
		}
	}
	
	/**
	 * Clear current input
	 */
	private void clearInput()
	{
		writeToTerminal("\u001b[" + command.length() + "D\u001b[0J");
		command = new StringBuilder();
	}
	
	/**
	 * Show previous command
	 */
	private void prevCommand()
	{
		if (historyPos == 0)
			return;
		
		clearInput();
		final String line = history.get(--historyPos);
		command.append(line);
		writeToTerminal(line);
	}
	
	/**
	 * Show next command
	 */
	private void nextCommand()
	{
		if (historyPos == history.size())
			return;
		
		clearInput();
		if (historyPos < history.size() - 1)
		{
			final String line = history.get(++historyPos);
			command.append(line);
			writeToTerminal(line);
		}
	}
	
	/**
	 * Write text to terminal
	 * 
	 * @param text text to write
	 */
	private void writeToTerminal(String text)
	{
		try
		{
			terminal.write(text.getBytes());
		}
		catch(UnsupportedEncodingException e)
		{
			// should never happen!
			e.printStackTrace();
		}
		catch(IOException e)
		{
			// should never happen!
			e.printStackTrace();
		}
	}
}
