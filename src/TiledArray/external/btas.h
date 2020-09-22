/*
 *  This file is a part of TiledArray.
 *  Copyright (C) 2018  Virginia Tech
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
 *  btas.h
 *  Jul 11, 2017
 *
 */

#ifndef TILEDARRAY_EXTERNAL_BTAS_H__INCLUDED
#define TILEDARRAY_EXTERNAL_BTAS_H__INCLUDED

#include <TiledArray/tensor/kernels.h>
#include <TiledArray/tile_interface/trace.h>
#include <TiledArray/utility.h>
#include "TiledArray/config.h"
#include "TiledArray/math/blas.h"
#include "TiledArray/math/gemm_helper.h"
#include "TiledArray/range.h"
#include "TiledArray/tensor/type_traits.h"
#include "TiledArray/tile_interface/cast.h"

#include <btas/features.h>
#include <btas/generic/axpy_impl.h>
#include <btas/generic/permute.h>
#include <btas/tensor.h>

#include <madness/world/archive.h>

namespace btas {
template <>
struct range_traits<TiledArray::Range> {
  const static CBLAS_ORDER order = CblasRowMajor;
  using index_type = TiledArray::Range::index_type;
  using ordinal_type = TiledArray::Range::ordinal_type;
  constexpr static const bool is_general_layout = false;
};
}  // namespace btas

namespace TiledArray {
namespace detail {
// these convert any range into TiledArray::Range

inline const TiledArray::Range& make_ta_range(const TiledArray::Range& range) {
  return range;
}

/// makes TiledArray::Range from a btas::RangeNd

/// \param[in] range a btas::RangeNd object
/// \throw TiledArray::Exception if \c range is non-row-major
template <CBLAS_ORDER Order, typename... Args>
inline TiledArray::Range make_ta_range(
    const btas::RangeNd<Order, Args...>& range) {
  TA_USER_ASSERT(Order == CblasRowMajor,
                 "TiledArray::detail::make_ta_range(btas::RangeNd<Order,...>): "
                 "not supported for col-major Order");
  return TiledArray::Range(range.lobound(), range.upbound());
}

}  // namespace detail
}  // namespace TiledArray

namespace btas {

/// Test if the two ranges are congruent

/// This function tests that the rank and extent of
/// \c r1 are equal to those of \c r2.
/// \param r1 The first Range to compare
/// \param r2 The second Range to compare
template <CBLAS_ORDER Order, typename... Args>
inline bool is_congruent(const btas::RangeNd<Order, Args...>& r1,
                         const btas::RangeNd<Order, Args...>& r2) {
  return (r1.rank() == r2.rank()) &&
         std::equal(r1.extent_data(), r1.extent_data() + r1.rank(),
                    r2.extent_data());
}

template <typename T, typename Range, typename Storage>
decltype(auto) make_ti(const btas::Tensor<T, Range, Storage>& arg) {
  return TiledArray::detail::TensorInterface<const T, Range,
                                             btas::Tensor<T, Range, Storage>>(
      arg.range(), arg.data());
}

template <typename T, typename Range, typename Storage>
decltype(auto) make_ti(btas::Tensor<T, Range, Storage>& arg) {
  return TiledArray::detail::TensorInterface<T, Range,
                                             btas::Tensor<T, Range, Storage>>(
      arg.range(), arg.data());
}

template <typename... Args>
inline bool operator==(const TiledArray::Range& range1,
                       const btas::BaseRangeNd<Args...>& range2) {
  const auto rank = range1.rank();
  if (rank == range2.rank()) {
    auto range1_lobound_data = range1.lobound_data();
    using std::cbegin;
    const auto lobound_match =
        std::equal(range1_lobound_data, range1_lobound_data + rank,
                   cbegin(range2.lobound()));
    if (lobound_match) {
      auto range1_upbound_data = range1.upbound_data();
      return std::equal(range1_upbound_data, range1_upbound_data + rank,
                        cbegin(range2.upbound()));
    }
  }
  return false;
}

/// Computes the result of applying permutation \c perm to \c arg
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> clone(
    const btas::Tensor<T, Range, Storage>& arg) {
  return arg;
}

/// Computes the result of applying permutation \c perm to \c arg
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> permute(
    const btas::Tensor<T, Range, Storage>& arg,
    const TiledArray::Permutation& perm) {
  btas::Tensor<T, Range, Storage> result;
  btas::permute(arg, perm.inv().data(), result);
  return result;
  // auto arg_view = make_ti(arg);
  // return arg_view.permute(perm);
}

// Shift operations ----------------------------------------------------------

/// Shift the range of \c arg

/// \param arg The tile argument to be shifted
/// \param range_shift The offset to be applied to the argument range
/// \return A copy of the tile with a new range
template <typename T, typename Range, typename Storage, typename Index>
inline btas::Tensor<T, Range, Storage> shift(
    const btas::Tensor<T, Range, Storage>& arg, const Index& range_shift) {
  auto shifted_arg = clone(arg);
  shift_to(shifted_arg, range_shift);
  return shifted_arg;
}

/// Shift the range of \c arg in place

/// \param arg The tile argument to be shifted
/// \param range_shift The offset to be applied to the argument range
/// \return A copy of the tile with a new range
template <typename T, typename Range, typename Storage, typename Index>
inline btas::Tensor<T, Range, Storage>& shift_to(
    btas::Tensor<T, Range, Storage>& arg, const Index& range_shift) {
  const_cast<Range&>(arg.range()).inplace_shift(range_shift);
  return arg;
}

/// result[i] = arg1[i] + arg2[i]
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> add(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.add(arg2_view);
}

/// result[i] = (arg1[i] + arg2[i]) * factor
template <typename T, typename Range, typename Storage, typename Scalar,
          typename std::enable_if<
              TiledArray::detail::is_numeric_v<Scalar>>::type* = nullptr>
inline btas::Tensor<T, Range, Storage> add(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2, const Scalar factor) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.add(arg2_view, factor);
}

/// result[perm ^ i] = (arg1[i] + arg2[i])
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> add(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2,
    const TiledArray::Permutation& perm) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.add(arg2_view, perm);
}

/// result[perm ^ i] = (arg1[i] + arg2[i]) * factor
template <typename T, typename Range, typename Storage, typename Scalar,
          typename std::enable_if<
              TiledArray::detail::is_numeric_v<Scalar>>::type* = nullptr>
inline btas::Tensor<T, Range, Storage> add(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2, const Scalar factor,
    const TiledArray::Permutation& perm) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.add(arg2_view, factor, perm);
}

/// result[i] += arg[i]
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage>& add_to(
    btas::Tensor<T, Range, Storage>& result,
    const btas::Tensor<T, Range, Storage>& arg) {
  auto result_view = make_ti(result);
  auto arg_view = make_ti(arg);
  result_view.add_to(arg_view);
  return result;
}

/// result[i] += factor * arg[i]
template <typename T, typename Range, typename Storage, typename Scalar,
          typename std::enable_if<
              TiledArray::detail::is_numeric_v<Scalar>>::type* = nullptr>
inline btas::Tensor<T, Range, Storage>& add_to(
    btas::Tensor<T, Range, Storage>& result,
    const btas::Tensor<T, Range, Storage>& arg, const Scalar factor) {
  auto result_view = make_ti(result);
  auto arg_view = make_ti(arg);
  result_view.add_to(arg_view, factor);
  return result;
}

/// result[i] = arg1[i] - arg2[i]
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> subt(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.subt(arg2_view);
}

/// result[i] = (arg1[i] - arg2[i]) * factor
template <typename T, typename Range, typename Storage, typename Scalar,
          typename std::enable_if<
              TiledArray::detail::is_numeric_v<Scalar>>::type* = nullptr>
inline btas::Tensor<T, Range, Storage> subt(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2, const Scalar factor) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.subt(arg2_view, factor);
}

/// result[perm ^ i] = (arg1[i] - arg2[i]) * factor
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> subt(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2,
    const TiledArray::Permutation& perm) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.subt(arg2_view, perm);
}

/// result[perm ^ i] = (arg1[i] - arg2[i]) * factor
template <typename T, typename Range, typename Storage, typename Scalar,
          typename std::enable_if<
              TiledArray::detail::is_numeric_v<Scalar>>::type* = nullptr>
inline btas::Tensor<T, Range, Storage> subt(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2, const Scalar factor,
    const TiledArray::Permutation& perm) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.subt(arg2_view, factor, perm);
}

/// result[i] -= arg[i]
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage>& subt_to(
    btas::Tensor<T, Range, Storage>& result,
    const btas::Tensor<T, Range, Storage>& arg) {
  auto result_view = make_ti(result);
  auto arg_view = make_ti(arg);
  result_view.subt_to(arg_view);
  return result;
}

template <typename T, typename Range, typename Storage, typename Scalar,
          typename std::enable_if<
              TiledArray::detail::is_numeric_v<Scalar>>::type* = nullptr>
inline btas::Tensor<T, Range, Storage>& subt_to(
    btas::Tensor<T, Range, Storage>& result,
    const btas::Tensor<T, Range, Storage>& arg, const Scalar factor) {
  auto result_view = make_ti(result);
  auto arg_view = make_ti(arg);
  result_view.subt_to(arg_view, factor);
  return result;
}

/// result[i] = arg1[i] * arg2[i]
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> mult(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.mult(arg2_view);
}

/// result[i] = arg1[i] * arg2[i] * factor
template <typename T, typename Range, typename Storage, typename Scalar,
          typename std::enable_if<
              TiledArray::detail::is_numeric_v<Scalar>>::type* = nullptr>
inline btas::Tensor<T, Range, Storage> mult(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2, const Scalar factor) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.mult(arg2_view, factor);
}

/// result[perm ^ i] = arg1[i] * arg2[i]
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> mult(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2,
    const TiledArray::Permutation& perm) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.mult(arg2_view, perm);
}

/// result[perm ^ i] = arg1[i] * arg2[i] * factor
template <typename T, typename Range, typename Storage, typename Scalar,
          typename std::enable_if<
              TiledArray::detail::is_numeric_v<Scalar>>::type* = nullptr>
inline btas::Tensor<T, Range, Storage> mult(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2, const Scalar factor,
    const TiledArray::Permutation& perm) {
  auto arg1_view = make_ti(arg1);
  auto arg2_view = make_ti(arg2);
  return arg1_view.mult(arg2_view, factor, perm);
}

/// result[i] *= arg[i]
template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage>& mult_to(
    btas::Tensor<T, Range, Storage>& result,
    const btas::Tensor<T, Range, Storage>& arg) {
  auto result_view = make_ti(result);
  auto arg_view = make_ti(arg);
  result_view.mult_to(arg_view);
  return result;
}

/// result[i] *= arg[i] * factor
template <typename T, typename Range, typename Storage, typename Scalar,
          typename std::enable_if<
              TiledArray::detail::is_numeric_v<Scalar>>::type* = nullptr>
inline btas::Tensor<T, Range, Storage>& mult_to(
    btas::Tensor<T, Range, Storage>& result,
    const btas::Tensor<T, Range, Storage>& arg, const Scalar factor) {
  auto result_view = make_ti(result);
  auto arg_view = make_ti(arg);
  result_view.mult_to(arg_view, factor);
  return result;
}

template <typename T, typename Range, typename Storage, typename Scalar,
          std::enable_if_t<TiledArray::detail::is_numeric_v<Scalar>>* = nullptr>
inline btas::Tensor<T, Range, Storage>& scale_to(
    btas::Tensor<T, Range, Storage>& result, const Scalar factor) {
  auto result_view = make_ti(result);
  result_view.scale_to(factor);
  return result;
}

template <typename T, typename Range, typename Storage, typename Scalar,
          std::enable_if_t<TiledArray::detail::is_numeric_v<Scalar>>* = nullptr>
inline decltype(auto) scale(const btas::Tensor<T, Range, Storage>& result,
                            const Scalar factor) {
  auto result_view = make_ti(result);
  return result_view.scale(factor);
}

template <typename T, typename Range, typename Storage, typename Scalar,
          std::enable_if_t<TiledArray::detail::is_numeric_v<Scalar>>* = nullptr>
inline decltype(auto) scale(const btas::Tensor<T, Range, Storage>& result,
                            const Scalar factor,
                            const TiledArray::Permutation& perm) {
  auto result_view = make_ti(result);
  return result_view.scale(factor, perm);
}

template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage>& neg_to(
    btas::Tensor<T, Range, Storage>& result) {
  auto result_view = make_ti(result);
  result_view.neg_to();
  return result;
}

template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> neg(
    const btas::Tensor<T, Range, Storage>& arg) {
  auto arg_view = make_ti(arg);
  return arg_view.neg();
}

template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> neg(
    const btas::Tensor<T, Range, Storage>& arg,
    const TiledArray::Permutation& perm) {
  auto arg_view = make_ti(arg);
  return arg_view.neg(perm);
}

template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> conj(
    const btas::Tensor<T, Range, Storage>& arg) {
  auto arg_view = make_ti(arg);
  return arg_view.conj();
}

template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage> conj(
    const btas::Tensor<T, Range, Storage>& arg,
    const TiledArray::Permutation& perm) {
  auto arg_view = make_ti(arg);
  return arg_view.conj(perm);
}

template <typename T, typename Range, typename Storage, typename Scalar,
          std::enable_if_t<TiledArray::detail::is_numeric_v<Scalar>>* = nullptr>
inline btas::Tensor<T, Range, Storage> conj(
    const btas::Tensor<T, Range, Storage>& arg, const Scalar factor) {
  auto arg_view = make_ti(arg);
  return arg_view.conj(factor);
}

template <typename T, typename Range, typename Storage, typename Scalar,
          std::enable_if_t<TiledArray::detail::is_numeric_v<Scalar>>* = nullptr>
inline btas::Tensor<T, Range, Storage> conj(
    const btas::Tensor<T, Range, Storage>& arg, const Scalar factor,
    const TiledArray::Permutation& perm) {
  auto arg_view = make_ti(arg);
  return arg_view.conj(factor, perm);
}

template <typename T, typename Range, typename Storage>
inline btas::Tensor<T, Range, Storage>& conj_to(
    btas::Tensor<T, Range, Storage>& arg) {
  auto arg_view = make_ti(arg);
  arg_view.conj_to();
  return arg;
}

template <typename T, typename Range, typename Storage, typename Scalar,
          std::enable_if_t<TiledArray::detail::is_numeric_v<Scalar>>* = nullptr>
inline btas::Tensor<T, Range, Storage>& conj_to(
    btas::Tensor<T, Range, Storage>& arg, const Scalar factor) {
  auto arg_view = make_ti(arg);
  arg_view.conj_to(factor);
  return arg;
}

template <typename T, typename Range, typename Storage, typename Scalar>
inline btas::Tensor<T, Range, Storage> gemm(
    const btas::Tensor<T, Range, Storage>& left,
    const btas::Tensor<T, Range, Storage>& right, Scalar factor,
    const TiledArray::math::GemmHelper& gemm_helper) {
  // Check that the arguments are not empty and have the correct ranks
  TA_ASSERT(!left.empty());
  TA_ASSERT(left.range().rank() == gemm_helper.left_rank());
  TA_ASSERT(!right.empty());
  TA_ASSERT(right.range().rank() == gemm_helper.right_rank());

  // Construct the result Tensor
  typedef btas::Tensor<T, Range, Storage> Tensor;
  Tensor result(
      gemm_helper.make_result_range<Range>(left.range(), right.range()));

  // Check that the inner dimensions of left and right match
  TA_ASSERT(
      TiledArray::ignore_tile_position() ||
      gemm_helper.left_right_congruent(std::cbegin(left.range().lobound()),
                                       std::cbegin(right.range().lobound())));
  TA_ASSERT(
      TiledArray::ignore_tile_position() ||
      gemm_helper.left_right_congruent(std::cbegin(left.range().upbound()),
                                       std::cbegin(right.range().upbound())));
  TA_ASSERT(gemm_helper.left_right_congruent(
      std::cbegin(left.range().extent()), std::cbegin(right.range().extent())));

  // Compute gemm dimensions
  integer m = 1, n = 1, k = 1;
  gemm_helper.compute_matrix_sizes(m, n, k, left.range(), right.range());

  // Get the leading dimension for left and right matrices.
  const integer lda =
      (gemm_helper.left_op() == madness::cblas::NoTrans ? k : m);
  const integer ldb =
      (gemm_helper.right_op() == madness::cblas::NoTrans ? n : k);

  T factor_t(factor);

  TiledArray::math::gemm(gemm_helper.left_op(), gemm_helper.right_op(), m, n, k,
                         factor_t, left.data(), lda, right.data(), ldb, T(0),
                         result.data(), n);

  return result;
}

template <typename T, typename Range, typename Storage, typename Scalar>
inline void gemm(btas::Tensor<T, Range, Storage>& result,
                 const btas::Tensor<T, Range, Storage>& left,
                 const btas::Tensor<T, Range, Storage>& right, Scalar factor,
                 const TiledArray::math::GemmHelper& gemm_helper) {
  // Check that this tensor is not empty and has the correct rank
  TA_ASSERT(!result.empty());
  TA_ASSERT(result.range().rank() == gemm_helper.result_rank());

  // Check that the arguments are not empty and have the correct ranks
  TA_ASSERT(!left.empty());
  TA_ASSERT(left.range().rank() == gemm_helper.left_rank());
  TA_ASSERT(!right.empty());
  TA_ASSERT(right.range().rank() == gemm_helper.right_rank());

  // Check that the outer dimensions of left match the the corresponding
  // dimensions in result
  TA_ASSERT(
      TiledArray::ignore_tile_position() ||
      gemm_helper.left_result_congruent(std::cbegin(left.range().lobound()),
                                        std::cbegin(result.range().lobound())));
  TA_ASSERT(
      TiledArray::ignore_tile_position() ||
      gemm_helper.left_result_congruent(std::cbegin(left.range().upbound()),
                                        std::cbegin(result.range().upbound())));
  TA_ASSERT(
      gemm_helper.left_result_congruent(std::cbegin(left.range().extent()),
                                        std::cbegin(result.range().extent())));

  // Check that the outer dimensions of right match the the corresponding
  // dimensions in result
  TA_ASSERT(TiledArray::ignore_tile_position() ||
            gemm_helper.right_result_congruent(
                std::cbegin(right.range().lobound()),
                std::cbegin(result.range().lobound())));
  TA_ASSERT(TiledArray::ignore_tile_position() ||
            gemm_helper.right_result_congruent(
                std::cbegin(right.range().upbound()),
                std::cbegin(result.range().upbound())));
  TA_ASSERT(
      gemm_helper.right_result_congruent(std::cbegin(right.range().extent()),
                                         std::cbegin(result.range().extent())));

  // Check that the inner dimensions of left and right match
  TA_ASSERT(
      TiledArray::ignore_tile_position() ||
      gemm_helper.left_right_congruent(std::cbegin(left.range().lobound()),
                                       std::cbegin(right.range().lobound())));
  TA_ASSERT(
      TiledArray::ignore_tile_position() ||
      gemm_helper.left_right_congruent(std::cbegin(left.range().upbound()),
                                       std::cbegin(right.range().upbound())));
  TA_ASSERT(gemm_helper.left_right_congruent(
      std::cbegin(left.range().extent()), std::cbegin(right.range().extent())));

  // Compute gemm dimensions
  integer m, n, k;
  gemm_helper.compute_matrix_sizes(m, n, k, left.range(), right.range());

  // Get the leading dimension for left and right matrices.
  const integer lda =
      (gemm_helper.left_op() == madness::cblas::NoTrans ? k : m);
  const integer ldb =
      (gemm_helper.right_op() == madness::cblas::NoTrans ? n : k);

  T factor_t(factor);

  TiledArray::math::gemm(gemm_helper.left_op(), gemm_helper.right_op(), m, n, k,
                         factor_t, left.data(), lda, right.data(), ldb, T(1),
                         result.data(), n);
}

// sum of the hyperdiagonal elements
template <typename T, typename Range, typename Storage>
inline typename btas::Tensor<T, Range, Storage>::value_type trace(
    const btas::Tensor<T, Range, Storage>& arg) {
  assert(false);
}
// foreach(i) result += arg[i]
template <typename T, typename Range, typename Storage>
inline decltype(auto) sum(const btas::Tensor<T, Range, Storage>& arg) {
  return make_ti(arg).sum();
}
// foreach(i) result *= arg[i]
template <typename T, typename Range, typename Storage>
inline decltype(auto) product(const btas::Tensor<T, Range, Storage>& arg) {
  return make_ti(arg).product();
}

// foreach(i) result += arg[i] * arg[i]
template <typename T, typename Range, typename Storage>
inline decltype(auto) squared_norm(const btas::Tensor<T, Range, Storage>& arg) {
  return make_ti(arg).squared_norm();
};

// foreach(i) result += arg1[i] * arg2[i]
template <typename T, typename Range, typename Storage>
inline decltype(auto) dot(const btas::Tensor<T, Range, Storage>& arg1,
                          const btas::Tensor<T, Range, Storage>& arg2) {
  return make_ti(arg1).dot(make_ti(arg2));
};

template <typename T, typename Range, typename Storage>
inline decltype(auto) inner_product(
    const btas::Tensor<T, Range, Storage>& arg1,
    const btas::Tensor<T, Range, Storage>& arg2) {
  return make_ti(arg1).inner_product(make_ti(arg2));
};

// sqrt(squared_norm(arg))
template <typename T, typename Range, typename Storage>
inline decltype(auto) norm(const btas::Tensor<T, Range, Storage>& arg) {
  return make_ti(arg).norm();
}
// sqrt(squared_norm(arg))
template <typename T, typename Range, typename Storage, typename ResultType>
inline void norm(const btas::Tensor<T, Range, Storage>& arg,
                 ResultType& result) {
  result = make_ti(arg).template norm<ResultType>();
}
// foreach(i) result = max(result, arg[i])
template <typename T, typename Range, typename Storage>
inline decltype(auto) max(const btas::Tensor<T, Range, Storage>& arg) {
  return make_ti(arg).max();
}
// foreach(i) result = min(result, arg[i])
template <typename T, typename Range, typename Storage>
inline decltype(auto) min(const btas::Tensor<T, Range, Storage>& arg) {
  return make_ti(arg).min();
}
// foreach(i) result = max(result, abs(arg[i]))
template <typename T, typename Range, typename Storage>
inline decltype(auto) abs_max(const btas::Tensor<T, Range, Storage>& arg) {
  return make_ti(arg).abs_max();
}
// foreach(i) result = max(result, abs(arg[i]))
template <typename T, typename Range, typename Storage>
inline typename btas::Tensor<T, Range, Storage>::value_type abs_min(
    const btas::Tensor<T, Range, Storage>& arg) {
  return make_ti(arg).abs_min();
}
}  // namespace btas

namespace TiledArray {

namespace detail {

/// Signals that we can take the trace of a btas::Tensor<T, Range, Storage> (for
/// numeric \c T)
template <typename T, typename Range, typename Storage>
struct TraceIsDefined<btas::Tensor<T, Range, Storage>, enable_if_numeric_t<T>>
    : std::true_type {};

}  // namespace detail

/**
 * permute function for TiledArray::Range class with non-TiledArray Permutation
 * object
 */
template <typename Perm>
typename std::enable_if<!std::is_same<Perm, TiledArray::Permutation>::value,
                        TiledArray::Range>::type
permute(const TiledArray::Range& r, const Perm& p) {
  TiledArray::Permutation pp(p.begin(), p.end());
  return pp * r;
}

}  // namespace TiledArray

namespace TiledArray {
namespace detail {

template <typename T, typename... Args>
struct is_tensor_helper<btas::Tensor<T, Args...>> : public std::true_type {};

template <typename T, typename... Args>
struct is_contiguous_tensor_helper<btas::Tensor<T, Args...>>
    : public std::true_type {};

}  // namespace detail
}  // namespace TiledArray

namespace TiledArray {
/// \brief converts a btas::Tensor to a TiledArray::Tensor
template <typename T, typename Allocator, typename Range_, typename Storage>
struct Cast<TiledArray::Tensor<T, Allocator>,
            btas::Tensor<T, Range_, Storage>> {
  auto operator()(const btas::Tensor<T, Range_, Storage>& arg) const {
    TiledArray::Tensor<T, Allocator> result(detail::make_ta_range(arg.range()));
    using std::begin;
    std::copy(btas::cbegin(arg), btas::cend(arg), begin(result));
    return result;
  }
};
}  // namespace TiledArray

namespace madness {
namespace archive {

template <class Archive, typename T>
struct ArchiveLoadImpl<Archive, btas::varray<T>> {
  static inline void load(const Archive& ar, btas::varray<T>& x) {
    typename btas::varray<T>::size_type n{};
    ar& n;
    x.resize(n);
    for (typename btas::varray<T>::value_type& xi : x) ar& xi;
  }
};

template <class Archive, typename T>
struct ArchiveStoreImpl<Archive, btas::varray<T>> {
  static inline void store(const Archive& ar, const btas::varray<T>& x) {
    ar& x.size();
    for (const typename btas::varray<T>::value_type& xi : x) ar& xi;
  }
};

template <class Archive, CBLAS_ORDER _Order, typename _Index>
struct ArchiveLoadImpl<Archive, btas::BoxOrdinal<_Order, _Index>> {
  static inline void load(const Archive& ar,
                          btas::BoxOrdinal<_Order, _Index>& o) {
    typename btas::BoxOrdinal<_Order, _Index>::stride_type stride{};
    typename btas::BoxOrdinal<_Order, _Index>::value_type offset{};
    bool cont{};
    ar& stride& offset& cont;
    o = btas::BoxOrdinal<_Order, _Index>(std::move(stride), std::move(offset),
                                         std::move(cont));
  }
};

template <class Archive, CBLAS_ORDER _Order, typename _Index>
struct ArchiveStoreImpl<Archive, btas::BoxOrdinal<_Order, _Index>> {
  static inline void store(const Archive& ar,
                           const btas::BoxOrdinal<_Order, _Index>& o) {
    ar& o.stride() & o.offset() & o.contiguous();
  }
};

template <class Archive, CBLAS_ORDER _Order, typename _Index, typename _Ordinal>
struct ArchiveLoadImpl<Archive, btas::RangeNd<_Order, _Index, _Ordinal>> {
  static inline void load(const Archive& ar,
                          btas::RangeNd<_Order, _Index, _Ordinal>& r) {
    typedef typename btas::BaseRangeNd<
        btas::RangeNd<_Order, _Index, _Ordinal>>::index_type index_type;
    index_type lobound{}, upbound{};
    _Ordinal ordinal{};
    ar& lobound& upbound& ordinal;
    r = btas::RangeNd<_Order, _Index, _Ordinal>(
        std::move(lobound), std::move(upbound), std::move(ordinal));
  }
};

template <class Archive, CBLAS_ORDER _Order, typename _Index, typename _Ordinal>
struct ArchiveStoreImpl<Archive, btas::RangeNd<_Order, _Index, _Ordinal>> {
  static inline void store(const Archive& ar,
                           const btas::RangeNd<_Order, _Index, _Ordinal>& r) {
    ar& r.lobound() & r.upbound() & r.ordinal();
  }
};

template <class Archive, typename _T, class _Range, class _Store>
struct ArchiveLoadImpl<Archive, btas::Tensor<_T, _Range, _Store>> {
  static inline void load(const Archive& ar,
                          btas::Tensor<_T, _Range, _Store>& t) {
    _Range range{};
    _Store store{};
    ar& range& store;
    t = btas::Tensor<_T, _Range, _Store>(std::move(range), std::move(store));
  }
};

template <class Archive, typename _T, class _Range, class _Store>
struct ArchiveStoreImpl<Archive, btas::Tensor<_T, _Range, _Store>> {
  static inline void store(const Archive& ar,
                           const btas::Tensor<_T, _Range, _Store>& t) {
    ar& t.range() & t.storage();
  }
};
}  // namespace archive
}  // namespace madness

#endif /* TILEDARRAY_EXTERNAL_BTAS_H__INCLUDED */
