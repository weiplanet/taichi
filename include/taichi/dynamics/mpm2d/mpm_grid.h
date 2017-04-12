/*******************************************************************************
    Taichi - Physically based Computer Graphics Library

    Copyright (c) 2016 Yuanming Hu <yuanmhu@gmail.com>
                  2017 Yu Fang <squarefk@gmail.com>

    All rights reserved. Use of this source code is governed by
    the MIT license as written in the LICENSE file.
*******************************************************************************/

#pragma once

#include "mpm_utils.h"
#include <stb_image.h>
#include <algorithm>
#include <atomic>
#include <taichi/math/array_2d.h>
#include <taichi/math/array_1d.h>
#include <taichi/math/levelset_2d.h>
#include <taichi/math/dynamic_levelset_2d.h>
#include "mpm_particle.h"

TC_NAMESPACE_BEGIN

typedef MPMParticle Particle;

class Grid {
public:
    Array2D<Vector2> velocity;
    Array2D<Vector2> force_or_acc;
    Array2D<Vector2> velocity_backup;
    Array2D<Vector4> boundary_normal;
    Array2D<real> mass;
    Array2D<int> states;
    Array2D<int64> max_dt_int_strength;
    Array2D<int64> max_dt_int_cfl;
    Array2D<int64> max_dt_int;
    Array2D<int> particle_count;
    Array2D<Vector4> min_max_vel;
    Vector2i res;

    Vector2i low_res;

    void initialize(const Vector2i &sim_res) {
        this->res = sim_res + Vector2i(1);
        velocity.initialize(res, Vector2(0), Vector2(0));
        force_or_acc.initialize(res, Vector2(0), Vector2(0));
        boundary_normal.initialize(res, Vector4(0), Vector2(0));
        mass.initialize(res, 0.0f, Vector2(0));

        low_res.x = (res.x + grid_block_size - 1) / grid_block_size;
        low_res.y = (res.y + grid_block_size - 1) / grid_block_size;

        max_dt_int_strength.initialize(low_res, 0);
        max_dt_int_cfl.initialize(low_res, 0);
        max_dt_int.initialize(low_res, 0);
        particle_count.initialize(low_res, 0);
        states.initialize(low_res, 0);
        min_max_vel.initialize(low_res, Vector4(0));
    }

    void reset() {
        states = 0;
        particle_count = 0;
        min_max_vel = Vector4(1e30f, 1e30f, -1e30f, -1e30f);
        velocity = Vector2(0.0f);
        force_or_acc = Vector2(0.0f);
        max_dt_int_strength.reset(1LL << 60);
        max_dt_int_cfl.reset(1LL << 60);
        max_dt_int.reset(1LL << 60);
        mass = 0.0f;
    }

    void expand(bool expand_vel, bool expand_state) {
        Array2D<int> new_states;
        Array2D<Vector4> new_min_max_vel;
        new_min_max_vel.initialize(low_res, Vector4(1e30f, 1e30f, -1e30f, -1e30f));
        new_states.initialize(low_res, 0);

        auto update = [&](const Index2D ind, int dx, int dy,
                          const Array2D<Vector4> &min_max_vel, Array2D<Vector4> &new_min_max_vel,
                          const Array2D<int> &states, Array2D<int> &new_states) -> void {
            if (expand_vel) {
                auto &tmp = new_min_max_vel[ind.neighbour(dx, dy)];
                tmp[0] = std::min(tmp[0], min_max_vel[ind][0]);
                tmp[1] = std::min(tmp[1], min_max_vel[ind][1]);
                tmp[2] = std::max(tmp[2], min_max_vel[ind][2]);
                tmp[3] = std::max(tmp[3], min_max_vel[ind][3]);
            }
            if (expand_state) {
                if (states[ind.neighbour(dx, dy)])
                    new_states[ind.neighbour(dx, dy)] = 1;
            }
        };

        // Expand x
        for (auto &ind : states.get_region()) {
            update(ind, 0, 0, min_max_vel, new_min_max_vel, states, new_states);
            if (ind.i > 0) {
                update(ind, -1, 0, min_max_vel, new_min_max_vel, states, new_states);
            }
            if (ind.i < states.get_width() - 1) {
                update(ind, 1, 0, min_max_vel, new_min_max_vel, states, new_states);
            }
        }
        // Expand y
        for (auto &ind : states.get_region()) {
            update(ind, 0, 0, new_min_max_vel, min_max_vel, new_states, states);
            if (ind.j > 0) {
                update(ind, 0, -1, new_min_max_vel, min_max_vel, new_states, states);
            }
            if (ind.j < states.get_height() - 1) {
                update(ind, 0, 1, new_min_max_vel, min_max_vel, new_states, states);
            }
        }
    }

    int get_num_active_grids() {
        return states.abs_sum();
    }

    void backup_velocity() {
        velocity_backup = velocity;
    }


    void normalize_velocity() {
        for (auto &ind : velocity.get_region()) {
            if (mass[ind] > 0) { // Do not use EPS here!!
                velocity[ind] /= mass[ind];
            } else {
                velocity[ind] = Vector2(0, 0);
            }
            CV(velocity[ind]);
        }
    }

    void normalize_acceleration() {
        for (auto &ind : force_or_acc.get_region()) {
            if (mass[ind] > 0) { // Do not use EPS here!!
                force_or_acc[ind] /= mass[ind];
            } else {
                force_or_acc[ind] = Vector2(0, 0);
            }
            CV(force_or_acc[ind]);
        }
    }

    void apply_external_force(Vector2 acc) {
        for (auto &ind : mass.get_region()) {
            if (mass[ind] > 0) // Do not use EPS here!!
                force_or_acc[ind] += acc * mass[ind];
        }
    }

    void apply_boundary_conditions(const DynamicLevelSet2D &levelset, real delta_t, real t);

    void check_velocity() {
        for (int i = 0; i < res[0]; i++) {
            for (int j = 0; j < res[1]; j++) {
                if (!is_normal(velocity[i][j])) {
                    printf("Grid Velocity Check Fail!\n");
                    Pp(i);
                    Pp(j);
                    Pp(velocity[i][j]);
                    assert(false);
                }
            }
        }
    }
};


TC_NAMESPACE_END

