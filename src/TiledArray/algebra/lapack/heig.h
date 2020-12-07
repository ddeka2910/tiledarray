/*
 *  This file is a part of TiledArray.
 *  Copyright (C) 2020 Virginia Tech
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Eduard Valeyev
 *
 *  heig.h
 *  Created:  19 October,  2020
 *
 */
#ifndef TILEDARRAY_ALGEBRA_LAPACK_HEIG_H__INCLUDED
#define TILEDARRAY_ALGEBRA_LAPACK_HEIG_H__INCLUDED

#include <TiledArray/config.h>

#include <TiledArray/algebra/lapack/lapack.h>
#include <TiledArray/algebra/lapack/util.h>
#include <TiledArray/conversions/eigen.h>

namespace TiledArray::lapack {

/**
 *  @brief Solve the standard eigenvalue problem with LAPACK
 *
 *  A(i,k) X(k,j) = X(i,j) E(j)
 *
 *  Example Usage:
 *
 *  auto [E, X] = heig(A, ...)
 *
 *  @tparam Array Input array type
 *
 *  @param[in] A           Input array to be diagonalized. Must be rank-2
 *  @param[in] evec_trange TiledRange for resulting eigenvectors. If left empty,
 *                         will default to array.trange()
 *
 *  @returns A tuple containing the eigenvalues and eigenvectors of input array
 *  as std::vector and in TA format, respectively.
 */
template <typename Array>
auto heig(const Array& A, TiledRange evec_trange = TiledRange()) {
  using numeric_type = typename lapack::array_traits<Array>::numeric_type;
  World& world = A.world();
  auto A_eig = detail::to_eigen(A);
  std::vector<numeric_type> evals;
  if (world.rank() == 0) {
    lapack::heig(A_eig, evals);
  }
  world.gop.broadcast_serializable(A_eig, 0);
  world.gop.broadcast_serializable(evals, 0);
  if (evec_trange.rank() == 0) evec_trange = A.trange();
  return std::tuple(
    evals,
    eigen_to_array<Array>(world, evec_trange, A_eig)
  );
}

/**
 *  @brief Solve the generalized eigenvalue problem with LAPACK
 *
 *  A(i,k) X(k,j) = B(i,k) X(k,j) E(j)
 *
 *  with
 *
 *  X(k,i) B(k,l) X(l,j) = I(i,j)
 *
 *  Example Usage:
 *
 *  auto [E, X] = heig(A, B, ...)
 *
 *  @tparam Array Input array type
 *
 *  @param[in] A           Input array to be diagonalized. Must be rank-2
 *  @param[in] B           Positive-definite matrix
 *  @param[in] evec_trange TiledRange for resulting eigenvectors. If left empty,
 *                         will default to array.trange()
 *
 *  @returns A tuple containing the eigenvalues and eigenvectors of input array
 *  as std::vector and in TA format, respectively.
 */
template <typename ArrayA, typename ArrayB, typename EVecType = ArrayA>
auto heig(const ArrayA& A, const ArrayB& B, TiledRange evec_trange = TiledRange()) {
  using numeric_type = typename lapack::array_traits<ArrayA>::numeric_type;
  (void)lapack::array_traits<ArrayB>{};
  World& world = A.world();
  auto A_eig = detail::to_eigen(A);
  auto B_eig = detail::to_eigen(B);
  std::vector<numeric_type> evals;
  if (world.rank() == 0) {
    lapack::heig(A_eig, B_eig, evals);
  }
  world.gop.broadcast_serializable(A_eig, 0);
  world.gop.broadcast_serializable(evals, 0);
  if (evec_trange.rank() == 0) evec_trange = A.trange();
  return std::tuple(
    evals,
    eigen_to_array<ArrayA>(A.world(), evec_trange, A_eig)
  );
}

}  // namespace TiledArray::lapack

#endif  // TILEDARRAY_ALGEBRA_LAPACK_HEIG_H__INCLUDED
