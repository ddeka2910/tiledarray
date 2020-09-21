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
 *  David Williams-Young
 *  Computational Research Division, Lawrence Berkeley National Laboratory
 *
 *  svd.h
 *  Created:    12 June, 2020
 *
 */
#ifndef TILEDARRAY_MATH_SCALAPACK_SVD_H__INCLUDED
#define TILEDARRAY_MATH_SCALAPACK_SVD_H__INCLUDED

#include <TiledArray/config.h>
#if TILEDARRAY_HAS_SCALAPACK

#include <TiledArray/conversions/block_cyclic.h>
#include <scalapackpp/svd.hpp>

namespace TiledArray {


struct SVDReturnType{ };
struct SVDValuesOnly   : public SVDReturnType { };
struct SVDLeftVectors  : public SVDReturnType { };
struct SVDRightVectors : public SVDReturnType { };
struct SVDAllVectors   : public SVDReturnType { };

namespace detail {

template <typename SVDType, typename = void>
struct is_svd_return_type : public std::false_type { };

template <typename SVDType>
struct is_svd_return_type<
  SVDType, 
  std::enable_if_t<std::is_base_of_v<SVDReturnType, SVDType>>
> : public std::true_type { };

template <typename SVDType>
inline constexpr bool is_svd_return_type_v = is_svd_return_type<SVDType>::value;

template <typename SVDType, typename U = void>
struct enable_if_svd_return_type : 
  public std::enable_if< is_svd_return_type_v<SVDType>, U > { };

template <typename SVDType, typename U = void>
using enable_if_svd_return_type_t = 
  typename enable_if_svd_return_type<SVDType,U>::type;

}

/**
 *  @brief Compute the singular value decomposition (SVD) via ScaLAPACK
 *
 *  A(i,j) = S(k) U(i,k) conj(V(j,k))
 *
 *  Example Usage:
 *
 *  auto S          = svd<SVDValuesOnly>  (A, ...)
 *  auto [S, U]     = svd<SVDLeftVectors> (A, ...)
 *  auto [S, VT]    = svd<SVDRightVectors>(A, ...)
 *  auto [S, U, VT] = svd<SVDAllVectors>  (A, ...)
 *
 *  @tparam Array Input array type, must be convertable to BlockCyclicMatrix
 *
 *  @param[in] A           Input array to be decomposed. Must be rank-2
 *  @param[in] MB          ScaLAPACK row blocking factor. Defaults to 128
 *  @param[in] NB          ScaLAPACK column blocking factor. Defaults to 128
 *  @param[in] u_trange    TiledRange for resulting left singlar vectors. 
 *  @param[in] vt_trange   TiledRange for resulting right singlar vectors (transposed).
 *
 *  @returns A tuple containing the eigenvalues and eigenvectors of input array
 *  as std::vector and in TA format, respectively.
 */
template <typename SVDType, typename Array,
  typename = detail::enable_if_svd_return_type<SVDType>
>
auto svd( const Array& A, TiledRange u_trange, TiledRange vt_trange,
  size_t MB = 128, size_t NB = 128 
) {

  using value_type = typename Array::element_type;
  using real_type  = scalapackpp::detail::real_t<value_type>;

  auto& world = A.world();
  auto world_comm = world.mpi.comm().Get_mpi_comm();
  //auto world_comm = MPI_COMM_WORLD;
  blacspp::Grid grid = blacspp::Grid::square_grid(world_comm);

  world.gop.fence(); // stage ScaLAPACK execution
  auto matrix = scalapack::array_to_block_cyclic( A, grid, MB, NB );
  world.gop.fence(); // stage ScaLAPACK execution

  auto [M, N] = matrix.dims();
  auto SVD_SIZE = std::min(M,N);

  auto [AMloc, ANloc]   = matrix.dist().get_local_dims(M, N);
  auto [UMloc, UNloc]   = matrix.dist().get_local_dims(M, SVD_SIZE);
  auto [VTMloc, VTNloc] = matrix.dist().get_local_dims(SVD_SIZE, N);


  auto desc_a  = matrix.dist().descinit_noerror(M, N,        AMloc );
  auto desc_u  = matrix.dist().descinit_noerror(M, SVD_SIZE, UMloc );
  auto desc_vt = matrix.dist().descinit_noerror(SVD_SIZE, N, VTMloc);

  std::vector<real_type>        S( SVD_SIZE );

  constexpr bool need_uv = std::is_same_v< SVDType, SVDAllVectors   >;
  constexpr bool need_u  = std::is_same_v< SVDType, SVDLeftVectors  > or need_uv;
  constexpr bool need_vt = std::is_same_v< SVDType, SVDRightVectors > or need_uv;

  std::shared_ptr<scalapack::BlockCyclicMatrix<value_type>> U = nullptr, VT = nullptr;

  scalapackpp::VectorFlag JOBU  = scalapackpp::VectorFlag::NoVectors;
  scalapackpp::VectorFlag JOBVT = scalapackpp::VectorFlag::NoVectors;

  value_type* U_ptr  = nullptr;
  value_type* VT_ptr = nullptr;

  if constexpr (need_u) {
    JOBU = scalapackpp::VectorFlag::Vectors;
    U = std::make_shared<scalapack::BlockCyclicMatrix<value_type>>( 
      world, grid, M, SVD_SIZE, MB, NB 
    );

    U_ptr = U->local_mat().data();
  }

  if constexpr (need_vt) {
    JOBVT = scalapackpp::VectorFlag::Vectors;
    VT = std::make_shared<scalapack::BlockCyclicMatrix<value_type>>( 
      world, grid, SVD_SIZE, N, MB, NB
    );

    VT_ptr = VT->local_mat().data();
  }


  auto info = scalapackpp::pgesvd( JOBU, JOBVT, M, N,
    matrix.local_mat().data(), 1, 1, desc_a, S.data(),
    U_ptr, 1, 1, desc_u, VT_ptr, 1, 1, desc_vt );
  if (info) TA_EXCEPTION("SVD Failed");

  world.gop.fence();


  if constexpr (need_uv) {

    auto U_ta  = scalapack::block_cyclic_to_array<Array>( *U,  u_trange  );
    auto VT_ta = scalapack::block_cyclic_to_array<Array>( *VT, vt_trange );
    world.gop.fence();

    return std::tuple( S, U_ta, VT_ta );

  } else if constexpr (need_u) {

    auto U_ta  = scalapack::block_cyclic_to_array<Array>( *U,  u_trange  );
    world.gop.fence();

    return std::tuple( S, U_ta );

  } else if constexpr (need_vt) {

    auto VT_ta  = scalapack::block_cyclic_to_array<Array>( *VT,  vt_trange  );
    world.gop.fence();

    return std::tuple( S, VT_ta );

  } else {

    return S;

  }



}

} // namespace TiledArray

#endif // TILEDARRAY_HAS_SCALAPACK
#endif // TILEDARRAY_MATH_SCALAPACK_H__INCLUDED

