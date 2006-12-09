/* 
** NetXMS - Network Management System
** Network Maps Library
** Copyright (C) 2006 Victor Kirhenshtein
**
** This code is derived from Graph visualization library for VTK
**	(c) 2003 D.J. Duke
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
** File: rt.cpp
**
**/

#include "libnxmap.h"


//
// Constructor
//

nxleReingoldTilford::nxleReingoldTilford(nxGraph *pGraph)
                    :nxleGeneric(pGraph)
{
   m_xPreliminary = (double *)malloc(pGraph->GetVertexCount() * sizeof(double));
   m_xModifier = (double *)malloc(pGraph->GetVertexCount() * sizeof(double));
   m_xParentModifier = (double *)malloc(pGraph->GetVertexCount() * sizeof(double));
   m_passCount = (int *)malloc(pGraph->GetVertexCount() * sizeof(int));
   m_leftBrother = (nxVertex **)malloc(pGraph->GetVertexCount() * sizeof(nxVertex *));
   m_layer = (DWORD *)malloc(pGraph->GetVertexCount() * sizeof(DWORD));
   m_contour = (nxVertex **)malloc(pGraph->GetVertexCount() * sizeof(nxVertex *));
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

void nxleReingoldTilford::Initialize(nxVertex *root, DWORD layer)
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

double nxleReingoldTilford::TrueX(nxVertex *root)
{
   double x;
   DWORD index;
   nxVertex *node;
	
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

void nxleReingoldTilford::FirstPass(nxVertex *root) 
{
   double modifier, xRoot, xBrother;
   DWORD i, index, leftIndex, rightIndex;
   nxVertex *brother;

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
         // size of the node and a factor of 0.5 to allow some
         // minimum distance of either side.
         xRoot = TrueX(brother) + MAP_OBJECT_SIZE_X * 1.5;
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
         modifier = MAP_OBJECT_SIZE_X * 1.5 - (xRoot - xBrother);
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

void nxleReingoldTilford::SecondPass(nxVertex *root, double modifier)
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
   memset(m_contour, 0, sizeof(nxVertex *) * m_graph->GetVertexCount());
   memset(m_leftBrother, 0, sizeof(nxVertex *) * m_graph->GetVertexCount());
   memset(m_layer, 0, sizeof(DWORD) * m_graph->GetVertexCount());

   Initialize(m_graph->GetRootVertex(), 0);
   FirstPass(m_graph->GetRootVertex());
   SecondPass(m_graph->GetRootVertex(), 0);
}
