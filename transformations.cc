// Copyright (C) 2016 West Virginia University.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of West Virginia University nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Please contact the author of this library if you have any questions.
// Author: Victor Fragoso (victor.fragoso@mail.wvu.edu)
// Author: Dustin Teel (dlteel@mix.wvu.edu)
// Author: Brandon Horn (<Add Email Here>)

#include "transformations.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iostream>

namespace wvu {
// Compute translation transformation matrix.
// Params:
//   offset  The translation offset vector.
Eigen::Matrix4f ComputeTranslationMatrix(const Eigen::Vector3f& offset) {
  // TODO: Implement me!
    Eigen::Matrix4f translation_matrix = Eigen::Matrix4f::Identity();
    translation_matrix.col(3) = offset.homogeneous();
    return translation_matrix;
}

// Compute rotation transformation matrix.
// Params:
//   rotation_axis  The rotation axis.
//   angle_in_radians  Angle in radians.
Eigen::Matrix4f ComputeRotationMatrix(const Eigen::Vector3f& rotation_axis,
                                      const float angle_in_radians) {
    Eigen::AngleAxisf rotation(angle_in_radians, rotation_axis);
    Eigen::Matrix4f rotation_matrix = Eigen::Matrix4f::Identity();
    Eigen::Matrix3f rotation3 = rotation.matrix();
    rotation_matrix.block(0, 0, 3, 3) = rotation3;
  return rotation_matrix;
}

// Compute scaling transformation matrix.
// Params:
//   scale  Scale factor.
Eigen::Matrix4f ComputeScalingMatrix(const float scale) {
    Eigen::Matrix4f scale_matrix = Eigen::Matrix4f::Identity();
    scale_matrix(0, 0) = scale;
    scale_matrix(1, 1) = scale;
    scale_matrix(2, 2) = scale;
    return scale_matrix;
}

// Converts angles in degrees to radians.
// Parameters:
//   angle_in_degrees  The angle in degrees.
float ConvertDegreesToRadians(const float angle_in_degrees) {
    float angle_in_radians = angle_in_degrees * 3.1416f / 180.0f;
    return angle_in_radians;
}

}  // namespace wvu
