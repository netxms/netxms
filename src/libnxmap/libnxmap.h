/* 
** NetXMS - Network Management System
** Network Map Library
** Copyright (C) 2006 Victor Kirhenshtein
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
** File: libnxmap.h
**
**/

#ifndef _libnxmap_h_
#define _libnxmap_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxcpapi.h>
#include <netxms_maps.h>


//
// Generic layout engine
//

class nxleGeneric
{
protected:
   nxmap_Graph *m_graph;

public:
   nxleGeneric(nxmap_Graph *pGraph) { m_graph = pGraph; }
   virtual ~nxleGeneric() { }

   virtual void Execute(void) { }
};


//
// Radial layout engine
//

class nxleRadial : public nxleGeneric
{
protected:
   double *m_width;
   double *m_fullWidth;
   double *m_angle;
   double *m_distance;
   double m_delta;
   double m_increase;
   BOOL m_bConvexity;

   double SetWidth(nxmap_Vertex *root);
   void SetPlacement(nxmap_Vertex *root, double ro,
                     double alpha1, double alpha2,	int layer);

public:
   nxleRadial(nxmap_Graph *pGraph);
   virtual ~nxleRadial();

   virtual void Execute(void);
};


//
// Reingold Tilford layout engine
//

class nxleReingoldTilford : public nxleGeneric
{
protected:
   double *m_xPreliminary;
   double *m_xModifier;
   double *m_xParentModifier;
   int *m_passCount;
   nxmap_Vertex **m_leftBrother;
   DWORD *m_layer;
   nxmap_Vertex **m_contour;
   double m_spacing;

   void Initialize(nxmap_Vertex *root, DWORD layer);
   double TrueX(nxmap_Vertex *root);
   void FirstPass(nxmap_Vertex *root);
   void SecondPass(nxmap_Vertex *root, double modifier);

public:
   nxleReingoldTilford(nxmap_Graph *pGraph);
   virtual ~nxleReingoldTilford();

   virtual void Execute(void);
};

#endif   /* _libnxmap_h_ */
