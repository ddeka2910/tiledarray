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
 *  lapack.cpp
 *  Created:    16 October, 2020
 *
 */

#include <TiledArray/algebra/lapack/lapack.h>
#include <TiledArray/algebra/lapack/util.h>
#include <Eigen/Core>

namespace TiledArray::lapack {

template <typename T>
void cholesky(Matrix<T>& A) {
  char uplo = 'L';
  integer n = A.rows();
  auto* a = A.data();
  integer lda = n;
  integer info = 0;
  TA_LAPACK_POTRF(&uplo, &n, a, &lda, &info);
  if (info != 0) TA_EXCEPTION("lapack::cholesky failed");
}

template <typename T>
void cholesky_linv(Matrix<T>& A) {
  char uplo = 'L';
  char diag = 'N';
  integer n = A.rows();
  auto* l = A.data();
  integer lda = n;
  integer info = 0;
  TA_LAPACK_TRTRI(&uplo, &diag, &n, l, &lda, &info);
  if (info != 0) TA_EXCEPTION("lapack::cholesky_linv failed");
}

template <typename T>
void cholesky_solve(Matrix<T>& A, Matrix<T>& X) {
  char uplo = 'L';
  integer n = A.rows();
  integer nrhs = X.cols();
  auto* a = A.data();
  auto* b = X.data();
  integer lda = n;
  integer ldb = n;
  integer info = 0;
  TA_LAPACK_POSV(&uplo, &n, &nrhs, a, &lda, b, &ldb, &info);
  if (info != 0) TA_EXCEPTION("lapack::cholesky_solve failed");
}

template <typename T>
void cholesky_lsolve(TransposeFlag transpose, Matrix<T>& A, Matrix<T>& X) {
  char uplo = 'L';
  char trans = transpose == TransposeFlag::Transpose
                   ? 'T'
                   : (transpose == TransposeFlag::NoTranspose ? 'N' : 'C');
  char diag = 'N';
  integer n = A.rows();
  integer nrhs = X.cols();
  auto* a = A.data();
  auto* b = X.data();
  integer lda = n;
  integer ldb = n;
  integer info = 0;
  TA_LAPACK_TRTRS(&uplo, &trans, &diag, &n, &nrhs, a, &lda, b, &ldb, &info);
  if (info != 0) TA_EXCEPTION("lapack::cholesky_lsolve failed");
}

template <typename T>
void heig(Matrix<T>& A, std::vector<T>& W) {
  char jobz = 'V';
  char uplo = 'L';
  integer n = A.rows();
  T* a = A.data();
  integer lda = A.rows();
  W.resize(n);
  T* w = W.data();
  integer lwork = -1;
  integer info;
  T lwork_dummy;
  TA_LAPACK_SYEV(&jobz, &uplo, &n, a, &lda, w, &lwork_dummy, &lwork, &info);
  lwork = integer(lwork_dummy);
  std::vector<T> work(lwork);
  TA_LAPACK_SYEV(&jobz, &uplo, &n, a, &lda, w, work.data(), &lwork, &info);
  if (info != 0) TA_EXCEPTION("lapack::heig failed");
}

template <typename T>
void heig(Matrix<T>& A, Matrix<T>& B, std::vector<T>& W) {
  integer itype = 1;
  char jobz = 'V';
  char uplo = 'L';
  integer n = A.rows();
  T* a = A.data();
  integer lda = A.rows();
  T* b = B.data();
  integer ldb = B.rows();
  W.resize(n);
  T* w = W.data();
  integer lwork = -1;
  integer info;
  T lwork_dummy;
  TA_LAPACK_SYGV(&itype, &jobz, &uplo, &n, a, &lda, b, &ldb, w, &lwork_dummy,
                 &lwork, &info);
  lwork = integer(lwork_dummy);
  std::vector<T> work(lwork);
  TA_LAPACK_SYGV(&itype, &jobz, &uplo, &n, a, &lda, b, &ldb, w, work.data(),
                 &lwork, &info);
  if (info != 0) TA_EXCEPTION("lapack::heig failed");
}

template <typename T>
void svd(Matrix<T>& A, std::vector<T>& S, Matrix<T>* U, Matrix<T>* VT) {
  integer m = A.rows();
  integer n = A.cols();
  T* a = A.data();
  integer lda = A.rows();

  S.resize(std::min(m, n));
  T* s = S.data();

  char jobu = 'N';
  T* u = nullptr;
  integer ldu = m;
  if (U) {
    jobu = 'A';
    U->resize(m, n);
    u = U->data();
    ldu = U->rows();
  }

  char jobvt = 'N';
  T* vt = nullptr;
  integer ldvt = n;
  if (VT) {
    jobvt = 'A';
    VT->resize(n, m);
    vt = VT->data();
    ldvt = VT->rows();
  }

  integer lwork = -1;
  integer info;
  T lwork_dummy;

  TA_LAPACK_GESVD(&jobu, &jobvt, &m, &n, a, &lda, s, u, &ldu, vt, &ldvt,
                  &lwork_dummy, &lwork, &info);
  lwork = integer(lwork_dummy);
  std::vector<T> work(lwork);
  TA_LAPACK_GESVD(&jobu, &jobvt, &m, &n, a, &lda, s, u, &ldu, vt, &ldvt,
                  work.data(), &lwork, &info);
  if (info != 0) TA_EXCEPTION("lapack::svd failed");
}

template <typename T>
void lu_solve(Matrix<T>& A, Matrix<T>& B) {
  integer n = A.rows();
  integer nrhs = B.cols();
  T* a = A.data();
  integer lda = A.rows();
  T* b = B.data();
  integer ldb = B.rows();
  std::vector<integer> ipiv(n);
  integer info;
  TA_LAPACK_GESV(&n, &nrhs, a, &lda, ipiv.data(), b, &ldb, &info);
  if (info != 0) TA_EXCEPTION("lapack::lu_solve failed");
}

template <typename T>
void lu_inv(Matrix<T>& A) {
  integer n = A.rows();
  T* a = A.data();
  integer lda = A.rows();
  integer lwork = -1;
  std::vector<T> work(1);
  std::vector<integer> ipiv(n);
  integer info;
  TA_LAPACK_GETRF(&n, &n, a, &lda, ipiv.data(), &info);
  if (info != 0) TA_EXCEPTION("lapack::lu_inv failed");
  TA_LAPACK_GETRI(&n, a, &lda, ipiv.data(), work.data(), &lwork, &info);
  lwork = (integer)work[0];
  work.resize(lwork);
  TA_LAPACK_GETRI(&n, a, &lda, ipiv.data(), work.data(), &lwork, &info);
  if (info != 0) TA_EXCEPTION("lapack::lu_inv failed");
}

#define TA_LAPACK_EXPLICIT(MATRIX, VECTOR)                        \
  template void cholesky(MATRIX&);                                \
  template void cholesky_linv(MATRIX&);                           \
  template void cholesky_solve(MATRIX&, MATRIX&);                 \
  template void cholesky_lsolve(TransposeFlag, MATRIX&, MATRIX&); \
  template void heig(MATRIX&, VECTOR&);                           \
  template void heig(MATRIX&, MATRIX&, VECTOR&);                  \
  template void svd(MATRIX&, VECTOR&, MATRIX*, MATRIX*);          \
  template void lu_solve(MATRIX&, MATRIX&);                       \
  template void lu_inv(MATRIX&);

TA_LAPACK_EXPLICIT(lapack::Matrix<double>, std::vector<double>);
// TA_LAPACK_EXPLICIT(lapack::Matrix<float>, std::vector<float>);

}  // namespace TiledArray::lapack
