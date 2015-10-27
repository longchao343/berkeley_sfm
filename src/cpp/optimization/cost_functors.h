/*
 * Copyright (c) 2015, The Regents of the University of California (Regents).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *    3. Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Please contact the author(s) of this library if you have any questions.
 * Authors: Erik Nelson            ( eanelson@eecs.berkeley.edu )
 *          David Fridovich-Keil   ( dfk@eecs.berkeley.edu )
 */

///////////////////////////////////////////////////////////////////////////////
//
// This file defines cost functors that can be used in conjunction with Google
// Ceres solver to solve non-linear least-squares problems. Each functor must
// define a public templated operator() method that takes in arguments const T*
// const INPUT_VARIABLE, and T* OUTPUT_RESIDUAL, and returns a bool.
// 'INPUT_VARIABLE' will be the optimization variable, and 'OUTPUT_RESIDUAL'
// will be the cost associated with the input.
//
// One can define more specific cost functions by adding other structure to the
// functor, e.g. by passing in other parameters of the cost function to the
// functor's constructor.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BSFM_OPTIMIZATION_COST_FUNCTORS_H
#define BSFM_OPTIMIZATION_COST_FUNCTORS_H

#include <Eigen/Core>
#include <glog/logging.h>

#include "../geometry/point_3d.h"
#include "../matching/feature.h"

namespace bsfm {

// Geometric error is the distance in image space between a point x, and a
// projected point PX, where X is a 3D homogeneous point (4x1), P is a camera
// projection matrix (3x4), and x is the corresponding image-space point
// expressed in homogeneous coordinates (3x1). The geometric error can be
// expressed as sum_i d(x_i, PX_i)^2, where d( , ) is the Euclidean distance
// metric.
struct GeometricError {
  // We are trying to adjust our camera projection matrix, P, to satisfy
  // x - PX = 0. Input is a 2D point in image space ('x'), and a 3D point in
  // world space ('X').
  Feature x_;
  Point3D X_;
  GeometricError(const Feature& x, const Point3D& X)
      : x_(x), X_(X) {}

  // Residual is 1-dimensional, P is 12-dimensional (number of elements in a
  // camera projection matrix).
  template <typename T>
  bool operator()(const T* const P, T* geometric_error) const {

    // Matrix multiplication: x - PX. Assume homogeneous coordinates are 1.0.
    // Matrix P is stored in column-major order.
    T scale = P[2] * X_.X() + P[5] * X_.Y() + P[8] * X_.Z() + P[11];
    geometric_error[0] =
       x_.u_ - (P[0] * X_.X() + P[3] * X_.Y() + P[6] * X_.Z() + P[9]) / scale;
    geometric_error[1] =
       x_.v_ - (P[1] * X_.X() + P[4] * X_.Y() + P[7] * X_.Z() + P[10]) / scale;

    return true;
  }

  // Factory method.
  static ceres::CostFunction* Create(const Feature& x, const Point3D& X) {
    static const int kNumResiduals = 2;
    static const int kNumCameraParameters = 12;
    return new ceres::AutoDiffCostFunction<GeometricError,
           kNumResiduals,
           kNumCameraParameters>(new GeometricError(x, X));
  }
};  //\struct GeometricError

}  //\namespace bsfm

#endif
