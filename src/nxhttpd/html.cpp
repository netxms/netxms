/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006, 2007 Alex Kirhenshtein and Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: html.cpp
**
**/

#include "nxhttpd.h"


//
// Generate login form
//

void ShowFormLogin(HttpResponse &response, TCHAR *pszErrorText)
{
	response.BeginPage(_T("NetXMS :: Login"));

	response.AppendBody(
		_T("<form action=\"/login.app\" method=\"post\">\r\n")
		_T("	<input type=\"hidden\" name=\"cmd\" value=\"login\" />\r\n")
		_T("	<div id=\"horizon\">\r\n")
		_T("		<div id=\"content\">\r\n")
		_T("			<table id=\"login_dialog\" border=\"0\" cellpadding=\"4\">\r\n")
		_T("				<tr>\r\n")
		_T("					<td colspan=\"3\">\r\n")
		_T("						<img src=\"/images/login.png\" />\r\n")
		_T("					</td>\r\n")
		_T("				</tr>\r\n")
		_T("				<tr>\r\n")
		_T("					<td colspan=\"3\">\r\n")
		_T("						<hr noshade=\"\" width=\"90%\" align=\"center\" />\r\n")
		_T("					</td>\r\n")
		_T("				</tr>\r\n")
	);

   if (pszErrorText != NULL)
   {
		response.AppendBody(_T("<tr><td align=\"center\"><font color=\"#FF0000\"><strong>")
		                    _T("Unable to login ("));
		response.AppendBody(pszErrorText);
      response.AppendBody(_T(")</strong></font><br></td></tr>\r\n"));
   }

	response.AppendBody(
		_T("				<tr>\r\n")
		_T("					<td align=\"center\">Please enter your NetXMS user name and password below:</td>\r\n")
		_T("				</tr>\r\n")
		_T("				<tr>\r\n")
		_T("					<td align=\"center\">\r\n")
		_T("						<table border=\"0\" cellpadding=\"1\">\r\n")
		_T("							<tr>\r\n")
		_T("								<td>\r\n")
		_T("									<strong>Login:</strong>\r\n")
		_T("								</td>\r\n")
		_T("								<td>\r\n")
		_T("									<input id=\"focus_on_load\" name=\"user\" tabindex=\"1\" accesskey=\"l\" type=\"text\" size=\"21\" />\r\n")
		_T("								</td>\r\n")
		_T("								<td rowspan=\"2\" style=\"padding: 1em;\">\r\n")
		_T("									<input tabindex=\"3\" accesskey=\"s\" type=\"submit\" value=\"Login\" />\r\n")
		_T("								</td>\r\n")
		_T("							</tr>\r\n")
		_T("							<tr>\r\n")
		_T("								<td>\r\n")
		_T("									<strong>Password:</strong>\r\n")
		_T("								</td>\r\n")
		_T("								<td>\r\n")
		_T("									<input name=\"pwd\" tabindex=\"2\" accesskey=\"p\" type=\"password\" size=\"21\" />\r\n")
		_T("								</td>\r\n")
		_T("							</tr>\r\n")
		_T("						</table>\r\n")
		_T("					</td>\r\n")
		_T("				</tr>\r\n")
		_T("			</table>\r\n")
		_T("		</div>\r\n")
		_T("</div>\r\n")
		_T("</form>\r\n")
	);

	response.EndPage();
}


//
// Escape text for HTML
//

TCHAR *EscapeHTMLText(String &text)
{
	text.Translate(_T("&"), _T("&amp;"));
	text.Translate(_T("<"), _T("&lt;"));
	text.Translate(_T(">"), _T("&gt;"));
	text.Translate(_T("\""), _T("&quot;"));
	return (TCHAR *)text;
}


//
// Start table in a box
//

void AddTableHeader(HttpResponse &response, TCHAR *pszClass, ...)
{
   va_list args;
   TCHAR *pszTitle, *pszOptions;

	response.StartTableHeader(pszClass);
   va_start(args, pszClass);
   while(1)
   {
      pszTitle = va_arg(args, TCHAR *);
      if (pszTitle == NULL)
         break;
      pszOptions = va_arg(args, TCHAR *);
      response.AppendBody(_T("<td "));
      if (pszOptions != NULL)
      {
         response.AppendBody(pszOptions);
      }
      response.AppendBody(_T(">"));
		response.AppendBody(pszTitle);
		response.AppendBody(_T("</td>"));
   }
   response.AppendBody(_T("</tr>\r\n"));
}


//
// Show error message
//

void ShowErrorMessage(HttpResponse &response, DWORD dwError)
{
	response.AppendBody(_T("<div class=\"errorMessage\">ERROR: "));
	response.AppendBody((TCHAR *)NXCGetErrorText(dwError));
	response.AppendBody(_T("</div>"));
}


//
// Show informational message
//

void ShowInfoMessage(HttpResponse &response, TCHAR *pszText)
{
	String tmp;

	tmp = pszText;
	response.AppendBody(_T("<div class=\"infoMessage\">"));
	response.AppendBody(EscapeHTMLText(tmp));
	response.AppendBody(_T("</div>"));
}
