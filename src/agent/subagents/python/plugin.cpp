/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
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
** File: plugin.cpp
**
**/

#include "python_subagent.h"

/**
 * Constructor
 */
PythonPlugin::PythonPlugin(PythonInterpreter *interpreter)
{
   m_interpreter = interpreter;
}

/**
 * Destructor
 */
PythonPlugin::~PythonPlugin()
{
   delete m_interpreter;
}

/**
 * Load plugin
 */
PythonPlugin *PythonPlugin::load(const TCHAR *file, StructArray<NETXMS_SUBAGENT_PARAM> *parameters)
{
   return NULL;
}
