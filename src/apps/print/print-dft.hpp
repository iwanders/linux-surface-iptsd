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
};

} // namespace iptsd::apps::print

#endif // IPTSD_APPS_PRINT_DFT_PRINT_DFT_HPP
