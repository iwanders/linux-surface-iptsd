// SPDX-License-Identifier: GPL-2.0-or-later

#include "dft.hpp"


using QuadraticCoefficients = std::array<f64, 3>;
using Data = std::array<f64, IPTS_DFT_NUM_COMPONENTS>;
using Weights = std::array<f64, IPTS_DFT_NUM_COMPONENTS>;
[[nodiscard]] std::optional<QuadraticCoefficients> fitQuadratic(const Data &data, const Weights &weights){

  // Convert data to the y vector.
  auto y = Vector<f64, IPTS_DFT_NUM_COMPONENTS>(data.data());
  //  std::cout << "y:" << y << std::endl;

  // First, make the vandermonde matrix, with x ranging from 0 to 8, and three terms per row.
  Matrix<f64, IPTS_DFT_NUM_COMPONENTS, 3> V;
  for (std::size_t m = 0; m < IPTS_DFT_NUM_COMPONENTS; m++)
  {
    for (std::size_t n = 0; n < 3; n++) {
      V(m, (3 - 1)-n) = std::pow(static_cast<f64>(m), n);
    }
  }
  //  std::cout << "V:" << V << std::endl;

  // Next, apply the weights.
  Eigen::DiagonalMatrix<f64, IPTS_DFT_NUM_COMPONENTS> diag_weights;
  for (std::size_t m = 0; m < IPTS_DFT_NUM_COMPONENTS; m++)
  {
    diag_weights.diagonal()[m] = weights[m];
  }

  V = diag_weights * V;
  //  std::cout << "V:" << V << std::endl;
  y = diag_weights * y;
  //  std::cout << "y:" << y << std::endl;

  // Finally, all setup is complete, do the least squares fit.
  //  std::cout << "Squared: " << ((V.transpose() * V)) << std::endl;
  const auto left = V.transpose() * V;
  //  std::cout << "left: " << left << std::endl;
  const auto right = V.transpose()  * y ;
  //  std::cout << "right: " << right << std::endl;
  auto solver = left.ldlt();
  auto result = solver.solve(right );
  //  std::cout << "result:" << result << std::endl;

  QuadraticCoefficients coefficients;
  for (std::size_t i = 0 ; i < 3; i++) {
    coefficients[i] = result(i);
    //  coefficients[i] = 1;
  }
  
  return coefficients;
}

[[nodiscard]] std::optional<f32> interpolate_position_poly(struct ipts_pen_dft_window_row &row)
{
  static const Weights gaussian_0_7_stddev_at_4{0.12992260830505947, 0.3172836267015646, 0.6003730411984044, 0.8802485040505603, 1.0, 0.8802485040505603, 0.6003730411984044, 0.3172836267015646, 0.12992260830505947};

  Data data;
  for (std::size_t i = 0; i < IPTS_DFT_NUM_COMPONENTS; i++) {
    data[i] = std::sqrt(row.real[i]* row.real[i] + row.imag[i] * row.imag[i]);
  }

  const auto fitted = fitQuadratic(data, gaussian_0_7_stddev_at_4);
  if (!fitted.has_value()) {
    return {};
  }

  const auto& coeff = fitted.value();
  // Take the derivative
  const auto peak = -coeff[1] / (2.0 * coeff[0]);
  return peak + row.first;
}


int main(const int /*argc*/, const char **/*argv*/)
{
	iptsd::core::Config config;
	struct ipts_pen_dft_window_row row;
	// x[ 0]: freq: 1187205120 mag:     85289 first:    28 last: 36 mid: 32 zero: 0 IQ: [(    -8,    -3),(    -6,    -3),(     3,     2),(   202,   103),(   260,   133),(    -3,     1),(   -15,    -7),(   -13,    -6),(   -10,    -7),]
	// print("\n".join(f"row.real[{i}] = {I};\nrow.imag[{i}] = {Q};" for (i, (I, Q)) in enumerate(IQ)))

	// This row, should not be nan...
	row.frequency = 1187205120;
	row.magnitude = 85289;
	row.first = 28;
	row.last = 36;
	row.mid = 32;

	row.real[0] = -8;
	row.imag[0] = -3;
	row.real[1] = -6;
	row.imag[1] = -3;
	row.real[2] = 3;
	row.imag[2] = 2;

	// 226.74434943345335
	row.real[3] = 202;
	row.imag[3] = 103;

	// 292.04280508172087
	row.real[4] = 260;
	row.imag[4] = 133;

	// 3.1622776601683795, why is it -2.21?
	row.real[5] = -3;
	row.imag[5] = 1;
	row.real[6] = -15;
	row.imag[6] = -7;
	row.real[7] = -13;
	row.imag[7] = -6;
	row.real[8] = -10;
	row.imag[8] = -7;

	const auto res = iptsd::core::DftStylus::interpolate_position(row, config);
	std::cout << "Res:" << res << std::endl;

        const auto res2 = interpolate_position_poly(row);
	std::cout << "res2:" << res2.value() << std::endl; 
	return 0;
}
