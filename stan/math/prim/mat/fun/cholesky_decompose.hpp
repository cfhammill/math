#ifndef STAN_MATH_PRIM_MAT_FUN_CHOLESKY_DECOMPOSE_HPP
#define STAN_MATH_PRIM_MAT_FUN_CHOLESKY_DECOMPOSE_HPP

#include <stan/math/prim/mat/fun/Eigen.hpp>
#include <stan/math/prim/mat/err/check_pos_definite.hpp>
#include <stan/math/prim/mat/err/check_square.hpp>
#include <stan/math/prim/mat/err/check_symmetric.hpp>

#ifdef STAN_GPU
#include <stan/math/prim/mat/fun/ViennaCL.hpp>
#endif
#include <algorithm>
namespace stan {
  namespace math {

    /**
     * Return the lower-triangular Cholesky factor (i.e., matrix
     * square root) of the specified square, symmetric matrix.  The return
     * value \f$L\f$ will be a lower-traingular matrix such that the
     * original matrix \f$A\f$ is given by
     * <p>\f$A = L \times L^T\f$.
     * @param m Symmetrix matrix.
     * @return Square root of matrix.
     * @throw std::domain_error if m is not a symmetric matrix or
     *   if m is not positive definite (if m has more than 0 elements)
     */
    template <typename T>
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>
    cholesky_decompose(const Eigen::Matrix
                       <T, Eigen::Dynamic, Eigen::Dynamic>& m) {
      check_square("cholesky_decompose", "m", m);
      check_symmetric("cholesky_decompose", "m", m);
      #ifdef STAN_GPU
        if (m.rows()  > 70) {
          viennacl::matrix<double>  vcl_m(m.rows(), m.cols());
          viennacl::copy(m, vcl_m);
          viennacl::linalg::lu_factorize(vcl_m);
          Eigen::Matrix<double, -1, -1> m_l(m.rows(), m.cols());
          viennacl::copy(vcl_m, m_l);
          // TODO(Steve/Sean): Where should this check go?
          // check_pos_definite("cholesky_decompose", "m", L_A);
          m_l =
            Eigen::MatrixXd(m_l.triangularView<Eigen::Upper>()).transpose();
          for (int i = 0; i < m_l.rows(); i++) {
            m_l.col(i) /= std::sqrt(m_l(i, i));
          }
          return m_l;
        } else {
          Eigen::LLT<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> >
            llt(m.rows());
          llt.compute(m);
          check_pos_definite("cholesky_decompose", "m", llt);
          return llt.matrixL();
        }
      #else
        Eigen::LLT<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> >
          llt(m.rows());
        llt.compute(m);
        check_pos_definite("cholesky_decompose", "m", llt);
        return llt.matrixL();
      #endif
    }

  }
}
#endif


