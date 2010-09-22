/* 
** NetXMS - Network Management System
** Network Maps Library
** Copyright (C) 2006 Victor Kirhenshtein
**
** This code is derived from Graph visualization library for VTK
**	(c) 2003 D.J. Duke. Original license follows.
**
** Graph visualization library for VTK
** (c) 2003 D.J. Duke
**
** You are warned that this is an alpha release of the library.  The
** software has not been extensively tested or used, beyond the 
** personal research work of the developer.
** 
** This copyright notice is based on the copyright distributed with the
** VTK toolkit (see www.vtk.org), which this library extends.  The terms
** for using the material in this directory are similar to those of the
** full tool: 
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
** 
**  * Redistributions of source code must retain the above copyright notice,
**    this list of conditions and the following disclaimer.
** 
**  * Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
** 
**  * With respect to this library, neither the name of the developer 
**    (David Duke) or the name(s) of any contributor(s) to this library
**    may be used to endorse or promote products derived from this 
**    software without specific prior written permission.
** 
**  * Modified source versions must be plainly marked as such, and must not be
**    misrepresented as being the original software.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** --- end of original license ---
**
** File: radial.cpp
**
**/

#include "libnxmap.h"
#include <math.h>

#define PI     3.14159


//
// Constructor
//

nxleRadial::nxleRadial(nxmap_Graph *pGraph)
           :nxleGeneric(pGraph)
{
   m_bConvexity = FALSE;
   m_delta = MAP_OBJECT_SIZE_X + MAP_OBJECT_INTERVAL_X;
   m_increase = 50.0 * m_delta / 100.0;
   m_width = (double *)malloc(pGraph->GetVertexCount() * sizeof(double));
   m_fullWidth = (double *)malloc(pGraph->GetVertexCount() * sizeof(double));
   m_angle = (double *)malloc(pGraph->GetVertexCount() * sizeof(double));
   m_distance = (double *)malloc(pGraph->GetVertexCount() * sizeof(double));
}


//
// Destructor
//

nxleRadial::~nxleRadial()
{
   safe_free(m_width);
   safe_free(m_fullWidth);
   safe_free(m_angle);
   safe_free(m_distance);
}


//
// Position the tree rooted at "root".
// The allocated posiition is not final; each sub-tree is positioned
// on the assumption that its parent is located at the origin, and all
// nodes are positioned in the plane z=0.  This will be corrected in a
// second pass over the tree.
//
// The return value is the radius of the circle containing the layout
// of this (sub)-tree.
//

double nxleRadial::SetWidth(nxmap_Vertex *root)
{
	double width;
   DWORD i;

   if (root->GetNumChilds() == 0)
   {
      width = MAP_OBJECT_SIZE_X;
   }
   else
   {
      width = 0.0;
      for(i = 0; i < root->GetNumChilds(); i++)
      {
         width += SetWidth(root->GetChild(i)) + MAP_OBJECT_INTERVAL_X;
      }
   }
	m_width[m_graph->GetVertexIndex(root)] = width;
	m_fullWidth[m_graph->GetVertexIndex(root)] = width;
	return width;
}


//
// Calculate the final layout for a node, given its level in 
// the tree, and the (final) position of its parent.
//

void nxleRadial::SetPlacement(nxmap_Vertex *root, double ro,
                              double alpha1, double alpha2,	int layer)
{
   double s, tau, alpha, rootFullWidth, childWidth;
   double myDelta = m_delta + m_increase;
   double fi = (alpha1 + alpha2)/2.0;
   DWORD i, index;
   nxmap_Vertex *child;

	root->SetPosition((int)(ro * sin(fi)), (int)(ro * cos(fi)));

   index = m_graph->GetVertexIndex(root);
	m_angle[index] = fi;
	m_distance[index] = ro;
	
   if (root->GetNumChilds() > 0)
   {
      tau = m_bConvexity ? 2.0 * acos(ro / (ro + myDelta)) : 0.0;
		rootFullWidth = m_fullWidth[index];
		if (m_bConvexity && (ro != 0.0) && (tau < (alpha2 - alpha1)))
      {
         s = tau / rootFullWidth;
         alpha = (alpha1 + alpha2 - tau) / 2.0;
      }
		else 
      {
         s = (alpha2 - alpha1) / rootFullWidth;
         alpha = alpha1;
      }

      for(i = 0; i < root->GetNumChilds(); i++)
      {
         child = root->GetChild(i);
			childWidth = m_width[m_graph->GetVertexIndex(child)];
			SetPlacement(child, ro + myDelta, alpha,
                      alpha + s * childWidth, layer + 1);
			alpha += s * childWidth;
      }
   }
}


//
// Execute
//

void nxleRadial::Execute(void)
{
   if (m_graph->GetVertexCount() == 0)
      return;

   // Layout is performed in two passes.  First, find a provisional location
   // for each node, via a bottom-up traversal.  Then calculate a final 
   // position for each node, by performing a top-down traversal, placing
   // the root at the origin, and then positioning each child using the
   // provisional location of the child and final location of the parent.

   SetWidth(m_graph->GetRootVertex());
   SetPlacement(m_graph->GetRootVertex(), 0.0, 0.0, 2.0 * PI, 0);
}
