/*!
 * \file multigrid.hpp
 * \brief Class to apply h/p multigrid operations
 *
 * \author - Josh Romero
 *           Adapted for Flurry++ by Jacob Crabill
 *           Aerospace Computing Laboratory (ACL)
 *           Aero/Astro Department. Stanford University
 *
 * \version 1.0.0
 *
 * Flux Reconstruction in C++ (Flurry++) Code
 * Copyright (C) 2016 Jacob Crabill
 *
 * Flurry++ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Flurry++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Flurry++; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA..
 *
 */
#pragma once

#include <memory>
#include <vector>

#include "input.hpp"
#include "matrix.hpp"
#include "solver.hpp"


class multiGrid
{
  private:
    input *params = NULL;
    vector<input> Inputs;
    int order;
    vector<shared_ptr<solver>> grids;

    void restrict_pmg(solver &grid_fine, solver &grid_coarse);
    void prolong_pmg(solver &grid_fine, solver &grid_coarse);
    void prolong_err(solver &grid_c, solver &grid_f);
    void compute_source_term(solver &grid);

  public:
    void setup(int order, input *params, geo *Geo);
    void cycle(solver &Solver);
};