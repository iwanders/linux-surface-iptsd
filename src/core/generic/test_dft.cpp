// SPDX-License-Identifier: GPL-2.0-or-later

#include "dft.hpp"

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

	return 0;
}
