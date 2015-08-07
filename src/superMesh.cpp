/*!
 * \file superMesh.cpp
 * \brief superMesh class definition
 *
 * Creates a local supermesh for an element in one grid from
 * elements in another grid (See Farrell and Maddison, 2010)
 *
 * \author - Jacob Crabill
 *           Aerospace Computing Laboratory (ACL)
 *           Aero/Astro Department. Stanford University
 *
 * \version 0.0.1
 *
 * Flux Reconstruction in C++ (Flurry++) Code
 * Copyright (C) 2015 Jacob Crabill
 *
 */

#include "superMesh.hpp"

#include <array>
#include <set>
#include <map>

#include "funcs.hpp"


/*
 * Tetrahedron Node-Ordering Conventions:
 *                 v
 *                .
 *              ,/
 *             /
 *            2                                     2
 *          ,/|`\                                 ,/|`\
 *        ,/  |  `\                             ,/  |  `\
 *      ,/    '.   `\                         ,6    '.   `5
 *    ,/       |     `\                     ,/       8     `\
 *  ,/         |       `\                 ,/         |       `\
 * 0-----------'.--------1 --> u         0--------4--'.--------1
 *  `\.         |      ,/                 `\.         |      ,/
 *     `\.      |    ,/                      `\.      |    ,9
 *        `\.   '. ,/                           `7.   '. ,/
 *           `\. |/                                `\. |/
 *              `3                                    `3
 *               `\.
 *                 ` w
 */
superMesh::superMesh()
{

}

superMesh::~superMesh()
{

}

void superMesh::setup(geo* _gridT, geo* _gridD, int _targetCell, int _order)
{
  gridT = _gridT;
  gridD = _gridD;

  targetCell = _targetCell;
  order = _order;

  buildSuperMesh();
}

void superMesh::buildSuperMesh(void)
{

}

vector<tetra> superMesh::splitHexIntoTet(vector<point> &hexNodes)
{
  vector<tetra> newTets(5);

  int ind[5][4] = {{0,1,4,3},{2,1,6,3},{5,1,6,4},{7,3,4,6},{1,3,6,4}};

  for (int i=0; i<5; i++)
    for (int j=0; j<4; j++)
      newTets[i].nodes[j] = hexNodes[ind[i][j]];

  return newTets;
}

vector<tetra> superMesh::clipTet(tetra &tet, vector<point> &clipFace, Vec3 &norm)
{
  /* --- WARNING: Assuming a linear, planar face --- */

  vector<tetra> outTets;

  // Get face centroid
  point xc;
  for (auto &pt:clipFace)
    xc += pt;
  xc /= clipFace.size();

  set<int> deadPts;
  for (int i=0; i<4; i++) {
    // Check each point of tetra to see which must be removed
    Vec3 dx = tet.nodes[i] - xc;
    double dot = dx*norm;
    if (dot > 0) // Point lies on cut-side of clipping plane
      deadPts.insert(i);
  }

  /*
   * Perform the clipping and subdivide the new volume into new tets
   * Only 3 cases in which the clipping can occur
   * New points are created at the intersections of the original tet's edges
   * with the clipping plane: http://geomalgorithms.com/a05-_intersect-1.html
   */
  switch (deadPts.size()) {
    case 0: {
      // No intersection
      outTets.push_back(tet);
      break;
    }

    case 1: {
      // Remove 1 point to get a prism; split prism into 3 new tets
      int kill = *(deadPts.begin());  // The point to remove

      // Get the new points by intersecting the tet's edges with the clipping plane
      // Have to be careful about orientation of final tet
      map<int,array<int,3>> flipTet;
      flipTet[0] = {1,3,2};  flipTet[1] = {0,2,3};
      flipTet[2] = {0,3,1};  flipTet[3] = {0,1,2};
      array<int,3> ePts = flipTet[kill];

      // Find the intersection points
      array<point,3> newPts;
      for (int i=0; i<3; i++) {
        Vec3 ab = tet.nodes[ePts[i]] - tet.nodes[kill];
        Vec3 ac = xc - tet.nodes[kill];
        newPts[i] = ab*((norm*ac)/(norm*ab)) + tet.nodes[kill];
      }

      outTets.resize(3);
      outTets[0].nodes = {tet.nodes[ePts[0]], tet.nodes[ePts[1]], newPts[0], tet.nodes[ePts[2]]};
      outTets[1].nodes = {tet.nodes[ePts[2]],newPts[0],newPts[2],newPts[1]};
      outTets[2].nodes = {tet.nodes[ePts[1]],tet.nodes[ePts[2]],newPts[1],newPts[0]};
      break;
    }

    case 2: {
      // Tet cut in half through 4 edges; split into 3 new tets
      // Get the points we're keeping and 'killing'
      array<int,2> kill, keep;
      int m=0, n=0;
      for (int i=0; i<4; i++) {
        if (deadPts.count(i)) {
          kill[m] = i;
          m++;
        }
        else {
          keep[n] = i;
          n++;
        }
      }

      /* Re-orient tet (shuffle nodes) based on kept nodes so that
       * clipping becomes standardized; 'base case' is keeping {0,1}
       * One possible case for edge edge being removed */
      map<array<int,2>,array<int,4>> flipTet;
      flipTet[{0,1}] = {0,1,2,3};  flipTet[{0,2}] = {1,2,0,3};
      flipTet[{0,3}] = {1,3,2,0};  flipTet[{1,2}] = {2,0,1,3};
      flipTet[{1,3}] = {3,0,2,1};  flipTet[{2,3}] = {3,2,1,0};
      array<int,4> ind = flipTet[keep];

      // Intersect the plane with the edges to get the new points
      array<point,4> newPts;
      point a,b;
      Vec3 ab,ac;

      // Edge 0-3
      a = tet.nodes[ind[0]];
      b = tet.nodes[ind[3]];
      ab = b - a;
      ac = xc - a;
      newPts[0] = ab*((norm*ac)/(norm*ab)) + a;

      // Edge 1-3
      a = tet.nodes[ind[1]];
      b = tet.nodes[ind[3]];
      ab = b - a;
      ac = xc - a;
      newPts[1] = ab*((norm*ac)/(norm*ab)) + a;

      // Edge 1-2
      a = tet.nodes[ind[1]];
      b = tet.nodes[ind[2]];
      ab = b - a;
      ac = xc - a;
      newPts[2] = ab*((norm*ac)/(norm*ab)) + a;

      // Edge 0-2
      a = tet.nodes[ind[0]];
      b = tet.nodes[ind[2]];
      ab = b - a;
      ac = xc - a;
      newPts[3] = ab*((norm*ac)/(norm*ab)) + a;

      // Setup the new tets
      outTets.resize(3);
      outTets[0].nodes = {tet.nodes[ind[1]],newPts[0],newPts[3],tet.nodes[ind[0]]};
      outTets[1].nodes = {newPts[0],newPts[3],newPts[1],tet.nodes[ind[1]]};
      outTets[2].nodes = {newPts[1],newPts[3],newPts[2],tet.nodes[ind[1]]};
      break;
    }

    case 3: {

      // The opposite of case 1; new tet is one corner of original tet
      int keep = -1;
      for (int i=0; i<4; i++) {
        if (!deadPts.count(i)) {
          keep = i;
          break;
        }
      }

      // Get the new points by intersecting the tet's edges with the clipping plane
      // Have to be careful about orientation of final tet, so map to a 'standard' orientation
      map<int,array<int,3>> flipTet;
      flipTet[0] = {1,3,2};  flipTet[1] = {0,2,3};
      flipTet[2] = {0,3,1};  flipTet[3] = {0,1,2};
      array<int,3> ePts = flipTet[keep];

      // Setup outgoing tet; node 3 is the 'kept' node
      // Find the intersection points
      outTets.resize(1);
      outTets[0].nodes[3] = tet.nodes[keep];
      for (int i=0; i<3; i++) {
        Vec3 ab = tet.nodes[ePts[i]] - tet.nodes[keep];
        Vec3 ac = xc - tet.nodes[keep];
        outTets[0].nodes[i] = ab*((norm*ac)/(norm*ab)) + tet.nodes[keep];
      }
      break;
    }

    case 4: {
      cout << "WARNING: input donor-cell tet is completely exterior to target cell!" << endl;
      break;
    }
  }

  return outTets;
}

double superMesh::integrate(vector<double> &data)
{
  if (data.size() != nQpts) FatalError("To integrate over supermesh, data must lie at its quadrature nodes.");

  double val = 0;
  for (int i=0; i<nTets; i++) {
    for (int j=0; j<nQpts_tet; j++) {

    }
  }
}
