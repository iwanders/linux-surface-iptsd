// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef IPTSD_APPS_PRINT_DFT_PRINT_DFT_HPP
#define IPTSD_APPS_PRINT_DFT_PRINT_DFT_HPP

#include <common/casts.hpp>
#include <common/types.hpp>
#include <contacts/contact.hpp>
#include <core/generic/application.hpp>
#include <core/generic/config.hpp>
#include <core/generic/device.hpp>
#include <ipts/data.hpp>

#include <iomanip>
#include <string>

namespace iptsd::apps::print {

static std::string stringify_window_row(const ipts_pen_dft_window_row &row)
{
	std::stringstream ss;
	ss << "freq: " << std::setw(9) << row.frequency << " ";
	ss << "mag: " << std::setw(9) << row.magnitude << " ";
	ss << "first: " << std::setw(5) << static_cast<int>(row.first) << " ";
	ss << "last: " << static_cast<int>(row.last) << " ";
	ss << "mid: " << static_cast<int>(row.mid) << " ";
	ss << "zero: " << static_cast<int>(row.zero) << " ";
	ss << "IQ: [";
	for (std::size_t i = 0; i < IPTS_DFT_NUM_COMPONENTS; i++) {
		ss << "(" << std::setw(6) << row.real[i] << "," << std::setw(6) << row.imag[i]
		   << "),";
	}
	ss << "]";
	return ss.str();
}

static std::string stringify_dft_window(const ipts::DftWindow &data)
{
	std::stringstream ss;
	for (std::size_t i = 0; i < data.rows; i++) {
		ss << "x[" << std::setw(2) << i << "]: ";
		ss << stringify_window_row(data.x[i]) << "\n";
		ss << "y[" << std::setw(2) << i << "]: ";
		ss << stringify_window_row(data.y[i]) << "\n";
	}
	return ss.str();
};

struct PrintConfig {
	bool stylus_status {false};
	bool dft_id_button {false};
	bool dft_id_pressure {false};
	bool dft_id_position {false};
	bool dft_id_unknown {false};
};

class PrintDft : public core::Application {
private:
	// The last known state of the stylus.
	ipts::StylusData m_recent_stylus;
	PrintConfig m_config;

public:
	PrintDft(const core::Config &config,
		 const core::DeviceInfo &info,
		 std::optional<const ipts::Metadata> metadata)
		: core::Application(config, info, metadata) {};

	void set_config(const PrintConfig &config)
	{
		m_config = config;
	}

	void on_stylus(const ipts::StylusData &data) override
	{
		m_recent_stylus = data;
	}

	void on_dft(const ipts::DftWindow &data) override
	{
		const auto common = [&](const std::string &datatype) {
			std::cout << datatype << std::endl;
			if (m_config.stylus_status) {
				std::cout
					<< "Stylus "
					<< "proximity: " << (m_recent_stylus.proximity ? "Y" : "N")
					<< ", contact: " << (m_recent_stylus.contact ? "Y" : "N")
					<< ", button: " << (m_recent_stylus.button ? "Y" : "N")
					<< ", rubber: " << (m_recent_stylus.rubber ? "Y" : "N")
					<< "\n";
			}
			std::cout << stringify_dft_window(data);
			std::cout << std::endl;
		};
		switch (data.type) {
		case IPTS_DFT_ID_POSITION:
			if (m_config.dft_id_position) {
				common("IPTS_DFT_ID_POSITION");
			}
			break;
		case IPTS_DFT_ID_BUTTON:
			if (m_config.dft_id_button) {
				common("IPTS_DFT_ID_BUTTON");
			}
			break;
		case IPTS_DFT_ID_PRESSURE:
			if (m_config.dft_id_pressure) {
				common("IPTS_DFT_ID_PRESSURE");
			}
			break;
		default:
			if (m_config.dft_id_unknown) {
				common("IPTS_DFT_ID_" + std::to_string(data.type));
			}
			break;
		}
	}

	void on_pen_magnitude(const struct ipts_pen_magnitude &msg) override {
		if (true) {
			std::cout << "x_lower: " << static_cast<u32>(msg.x_lower) << "\n";
			std::cout << "y_lower: " << static_cast<u32>(msg.y_lower) << "\n";
			std::cout << "x_upper: " << static_cast<u32>(msg.x_upper) << "\n";
			std::cout << "y_upper: " << static_cast<u32>(msg.y_upper) << "\n";

			std::cout << "unknown_0: " << static_cast<u32>(msg.unknown_0) << "\n";
			std::cout << "unknown_1: " << static_cast<u32>(msg.unknown_1) << "\n";
			std::cout << "unknown_2: " << static_cast<u32>(msg.unknown_2) << "\n";
			std::cout << "unknown_3: " << static_cast<u32>(msg.unknown_3) << "\n";

			for (std::size_t i = 0 ; i < IPTS_PEN_MAGNITUDE_ENTRIES_X; i++ ){
				std::cout << "" << std::setw(6) << msg.data_x[i] << " ";
			}
			std::cout << std::endl;
			for (std::size_t i = 0 ; i < IPTS_PEN_MAGNITUDE_ENTRIES_Y; i++ ){
				std::cout << "" << std::setw(6) << msg.data_y[i] << " ";
			}
			std::cout << std::endl;
		}
		// Do some smarts to find the peak... >_<
		/*
			x = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 5, 65, 1937, 311981, 22613, 338, 10, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
			y = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 4, 130, 7018, 478948, 9469, 85, 13, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0]

			# Normalize the data
			def find_peak(a):
			    maxval = max(a)
			    sumval = sum(a)
			    x = [xi / sumval for xi in a]
			    total = 0.0
			    for (i, w) in enumerate(x):
				total += float(i) * w
			    print(total)

			find_peak(x)
			find_peak(y)
		*/
		const auto find_peak = [](const auto& d, const u32 size) {
			std::vector<f32> scaled;
			f32 sum = 0;
			for (std::size_t i = 0; i < size; i++) {
				sum += static_cast<f32>(d[i]);
			}
			for (std::size_t i = 0; i < size; i++) {
				scaled.push_back(static_cast<f32>(d[i]) / sum);
			}
			sum = 0.0;
			for (std::size_t i = 0; i < size; i++) {
				sum += static_cast<f32>(i) * scaled[i];
			}
			return sum;
		};
		const auto x_peak = find_peak(msg.data_x, IPTS_PEN_MAGNITUDE_ENTRIES_X);
		const auto y_peak = find_peak(msg.data_y, IPTS_PEN_MAGNITUDE_ENTRIES_Y);
		std::cout << "x: " << std::setw(6) << x_peak << " " << std::endl;
		std::cout << "y: " << std::setw(6) << y_peak << " " << std::endl;
		
	}
};

} // namespace iptsd::apps::print

#endif // IPTSD_APPS_PRINT_DFT_PRINT_DFT_HPP
