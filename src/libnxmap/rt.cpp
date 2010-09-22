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
** File: rt.cpp
**
**/

#include "libnxmap.h"


//
// Constructor
//

nxleReingoldTilford::nxleReingoldTilford(nxmap_Graph *pGraph)
                    :nxleGeneric(pGraph)
{
   m_xPreliminary = (double *)malloc(pGraph->GetVertexCount() * sizeof(double));
   m_xModifier = (double *)malloc(pGraph->GetVertexCount() * sizeof(double));
   m_xParentModifier = (double *)malloc(pGraph->GetVertexCount() * sizeof(double));
   m_passCount = (int *)malloc(pGraph->GetVertexCount() * sizeof(int));
   m_leftBrother = (nxmap_Vertex **)malloc(pGraph->GetVertexCount() * sizeof(nxmap_Vertex *));
   m_layer = (DWORD *)malloc(pGraph->GetVertexCount() * sizeof(DWORD));
   m_contour = (nxmap_Vertex **)malloc(pGraph->GetVertexCount() * sizeof(nxmap_Vertex *));
   m_spacing = MAP_OBJECT_SIZE_Y + MAP_TEXT_BOX_HEIGHT + MAP_OBJECT_INTERVAL_Y;
}


//
// Destructor
//

nxleReingoldTilford::~nxleReingoldTilford()
{
   safe_free(m_xPreliminary);
   safe_free(m_xModifier);
   safe_free(m_xParentModifier);
   safe_free(m_passCount);
   safe_free(m_leftBrother);
   safe_free(m_layer);
   safe_free(m_contour);
}


//
// Initialise a node for RT-layout.  This involves finding,
// for each node, the left-hand brother of the node, i.e. the
// node at the same depth in the tree that would be to the 
// immediate left of this node.  This is achieved by doing a
// traversal of the tree.
// 
// The parameter "layer" represents the nodes at the level
// of the tree currently being considered.
//

void nxleReingoldTilford::Initialize(nxmap_Vertex *root, DWORD layer)
{
   DWORD i, index;

   index = m_graph->GetVertexIndex(root);

   if ((m_layer[index] < layer) || (m_layer[index] == 0))
   {
   	m_leftBrother[index] = m_contour[layer];
   	m_contour[layer] = root;
	   m_layer[index] = layer;

      for(i = 0; i < root->GetNumChilds(); i++)
      {
         Initialize(root->GetChild(i), layer + 1);
      }
   }
}


//
// Return actual x coordinate of a node, taking into account
// the position modifiers of its ancestors up to the root. 
//

double nxleReingoldTilford::TrueX(nxmap_Vertex *root)
{
   double x;
   DWORD index;
   nxmap_Vertex *node;
	
   index = m_graph->GetVertexIndex(root);
	node = root;
   x = m_xPreliminary[index];
  
	while(node->GetNumParents() > 0)
   {
      x += m_xModifier[index];
      node = node->GetParent(node->GetNumParents() / 2);
      index = m_graph->GetVertexIndex(node);
   }
   return x;
}


//
// First pass
//

void nxleReingoldTilford::FirstPass(nxmap_Vertex *root) 
{
   double modifier, xRoot, xBrother;
   DWORD i, index, leftIndex, rightIndex;
   nxmap_Vertex *brother;

   index = m_graph->GetVertexIndex(root);

   // Case 1. Node is a leaf
   if (root->GetNumChilds() == 0) 
   {
      brother = m_leftBrother[index];
      if (brother == NULL)
      {           
         xRoot = 0.0; 	// no left brother
      }
      else 
      {
         // Find coordinate relative to brother.  Include the
         // size of the node and an internode interval
         xRoot = TrueX(brother) + MAP_OBJECT_SIZE_X + MAP_OBJECT_INTERVAL_X;
      }
		modifier = 0.0;
   }
   else 
   {
      // Apply FirstPass to each child
      for(i = 0; i < root->GetNumChilds(); i++)
      {
         FirstPass(root->GetChild(i));
      }

		// Position the node.
      leftIndex = m_graph->GetVertexIndex(root->GetChild(0));
      rightIndex = m_graph->GetVertexIndex(root->GetChild(root->GetNumChilds() - 1));
      xRoot = (m_xPreliminary[leftIndex] + m_xModifier[leftIndex] + m_xPreliminary[rightIndex] + m_xModifier[rightIndex]) / 2.0;
		modifier = 0.0;
		brother = m_leftBrother[index];

      if (brother != NULL) 
      {
         xBrother = TrueX(brother);
         modifier = MAP_OBJECT_SIZE_X + MAP_OBJECT_INTERVAL_X - (xRoot - xBrother);
			if (modifier < 0) 
				modifier = 0;
      }
   }

   m_xPreliminary[index] = xRoot;
   m_xModifier[index] = modifier;
}


//
// Second pass
//

void nxleReingoldTilford::SecondPass(nxmap_Vertex *root, double modifier)
{
   double childModif;
   DWORD i, index;

   index = m_graph->GetVertexIndex(root);
   m_passCount[index]++;
   m_xParentModifier[index] += modifier;
   root->SetPosition((int)(m_xPreliminary[index] + m_xModifier[index] + m_xParentModifier[index] / m_passCount[index]),
                     (int)(m_layer[index] * m_spacing));

   childModif = modifier + m_xModifier[index];
   for(i = 0; i < root->GetNumChilds(); i++)
   {
      SecondPass(root->GetChild(i), childModif);
   }
}


//
// Execute
//

void nxleReingoldTilford::Execute(void)
{
   if (m_graph->GetVertexCount() == 0)
      return;

   memset(m_xPreliminary, 0, sizeof(double) * m_graph->GetVertexCount());
   memset(m_xModifier, 0, sizeof(double) * m_graph->GetVertexCount());
   memset(m_xParentModifier, 0, sizeof(double) * m_graph->GetVertexCount());
   memset(m_passCount, 0, sizeof(int) * m_graph->GetVertexCount());
   memset(m_contour, 0, sizeof(nxmap_Vertex *) * m_graph->GetVertexCount());
   memset(m_leftBrother, 0, sizeof(nxmap_Vertex *) * m_graph->GetVertexCount());
   memset(m_layer, 0, sizeof(DWORD) * m_graph->GetVertexCount());

   Initialize(m_graph->GetRootVertex(), 0);
   FirstPass(m_graph->GetRootVertex());
   SecondPass(m_graph->GetRootVertex(), 0);
}
