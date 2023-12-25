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
#include <filesystem>
#include <fstream>

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

template <typename T>
struct Entry {
	const char* m_str;
	const T m_v;
	bool m_add_comma;
	Entry(const char* key, const T value, bool add_comma=true) : m_str{key}, m_v{value}, m_add_comma{add_comma} {}
	
	
	friend std::ostream &operator<<(std::ostream &os, const Entry &a)
	{
		return os << "\"" << a.m_str << "\":" << a.m_v << (a.m_add_comma ? "," : "");
	}
};

static std::string json_window_row_iq(const ipts_pen_dft_window_row &row) {
	std::stringstream ss;
	ss << "[";
	for (std::size_t i = 0; i < IPTS_DFT_NUM_COMPONENTS; i++) {
		ss << "[" << row.real[i] << "," << row.imag[i] << "]";
		if (i != IPTS_DFT_NUM_COMPONENTS - 1) {
			ss << ",";
		}
	}
	ss << "]";
	return ss.str();
}

static std::string json_window_row(const ipts_pen_dft_window_row &row)
{
	std::stringstream ss;
	ss << "{";
	ss << Entry("freq", row.frequency);
	ss << Entry("mag", row.magnitude);
	ss << Entry<u32>("first", row.first);
	ss << Entry<u32>("last", row.last);
	ss << Entry<u32>("mid", row.mid);
	ss << Entry<u32>("zero", row.zero);
	ss << Entry<std::string>("iq", json_window_row_iq(row), false);
	ss << "}";

	return ss.str();
}

static std::string json_dft_window(const ipts::DftWindow &data)
{
	std::stringstream ss;
	ss << "{";
	ss << Entry<u32>("rows", data.rows);
	ss << Entry<u32>("type", data.type);
	std::stringstream ss_x;
	std::stringstream ss_y;
	ss_x << "[\n";
	ss_y << "[\n";
	for (i32 i = 0; i < data.rows; i++) {
		ss_x << "    " << json_window_row(data.x[i]) << (i != data.rows - 1 ? "," : "") << "\n";
		ss_y << "    " << json_window_row(data.y[i]) << (i != data.rows - 1 ? "," : "") << "\n";
	}
	ss << Entry("x", ss_x.str() + " ]");
	ss << Entry("y", ss_y.str() + " ]\n", false);
	ss << "}";

	return ss.str();
}

static std::string json_entry(const std::string &type, const std::string& payload) {
	std::stringstream ss;
	ss << "{";
	ss << Entry("type", "\""  + type + "\"") << "\n";
	ss << Entry("payload", payload, false);
	ss << "}";
	return ss.str();
} 

struct PrintConfig {
	bool stylus_status {false};
	bool dft_id_button {false};
	bool dft_id_pressure {false};
	bool dft_id_position {false};
	bool dft_id_position2 {false};
	bool dft_id_unknown {false};
	std::string output_json;
};

class PrintDft : public core::Application {
private:
	// The last known state of the stylus.
	ipts::StylusData m_recent_stylus;
	PrintConfig m_config;

	std::string m_collected_json;

public:
	PrintDft(const core::Config &config,
		 const core::DeviceInfo &info,
		 std::optional<const ipts::Metadata> metadata)
		: core::Application(config, info, metadata) {
	};


	void on_stop() override
	{
		if (is_logging_json()) {
			// dump the file.
			const auto path = std::filesystem::path{m_config.output_json};
			std::filesystem::create_directories(path.parent_path());
			std::ofstream file{path};
			file << "[\n";
			file << m_collected_json;
			file << "\n]\n";
		}
	}

	void set_config(const PrintConfig &config)
	{
		m_config = config;
	}

	bool is_logging_json() const {
		return !m_config.output_json.empty();
	}

	void append_json(const std::string &v) {
		if (is_logging_json()) {
			if (!m_collected_json.empty()) {
				m_collected_json += ",\n";
			}
			m_collected_json += v;
		}
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
			append_json(json_entry(datatype, json_dft_window(data)));
			std::cout << std::endl;
		};
		switch (data.type) {
		case IPTS_DFT_ID_POSITION:
			if (m_config.dft_id_position) {
				common("IPTS_DFT_ID_POSITION");
			}
			break;
		case IPTS_DFT_ID_POSITION2:
			if (m_config.dft_id_position2) {
				common("IPTS_DFT_ID_POSITION2");
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
