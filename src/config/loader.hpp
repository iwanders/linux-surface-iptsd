// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef IPTSD_CONFIG_LOADER_HPP
#define IPTSD_CONFIG_LOADER_HPP

#include "config.hpp"

#include <common/types.hpp>

#include <INIReader.h>
#include <configure.h>
#include <filesystem>
#include <fmt/format.h>
#include <optional>
#include <stdexcept>
#include <string>

namespace iptsd::config {

class Loader {
private:
	Config m_config {};

	// The vendor ID of the device for which the config should be loaded.
	u16 m_vendor;

	// The product ID of the device for which the config should be loaded.
	u16 m_product;

public:
	Loader(i16 vendor, i16 product, std::optional<const ipts::Metadata> metadata)
		: m_vendor {static_cast<u16>(vendor)},
		  m_product {static_cast<u16>(product)}
	{
		namespace filesystem = std::filesystem;

		if (metadata.has_value()) {
			m_config.width = gsl::narrow<f32>(metadata->size.width) / 1e3f;
			m_config.height = gsl::narrow<f32>(metadata->size.height) / 1e3f;
			m_config.invert_x = metadata->transform.xx < 0;
			m_config.invert_y = metadata->transform.yy < 0;
		}

		this->load_dir(IPTSD_PRESET_DIR, true);
		this->load_dir("./etc/presets", true);

		/*
		 * Load configuration file from custom location.
		 *
		 * Mainly for developers to debug their work without touching their
		 * known working main system configuration.
		 */
		if (const char *config_file_path = std::getenv("IPTSD_CONFIG_FILE")) {
			this->load_file(config_file_path);
			return;
		}

		if (filesystem::exists(IPTSD_CONFIG_FILE))
			this->load_file(IPTSD_CONFIG_FILE);

		this->load_dir(IPTSD_CONFIG_DIR, false);
	}

	/*!
	 * The loaded config object.
	 *
	 * @return The configuration data that was loaded for the given device.
	 */
	[[nodiscard]] Config config() const
	{
		return m_config;
	}

private:
	/*!
	 * Load all configuration files from a directory.
	 *
	 * @param[in] path The path to the directory.
	 * @param[in] check_device If true, check if the config is meant for the current device.
	 */
	void load_dir(const std::string &path, bool check_device)
	{
		namespace filesystem = std::filesystem;

		if (!filesystem::exists(path))
			return;

		for (const filesystem::directory_entry &p : filesystem::directory_iterator(path)) {
			if (!p.is_regular_file())
				continue;

			if (check_device) {
				u16 vendor = 0;
				u16 product = 0;

				this->load_device(p.path(), vendor, product);

				// Ignore this file if it is meant for a different device.
				if (m_vendor != vendor || m_product != product)
					continue;
			}

			this->load_file(p.path());
		}
	}

	/*!
	 * Determines for which device a config file is meant.
	 *
	 * @param[in] path The path to the config file.
	 * @param[out] vendor The vendor ID the config is targeting.
	 * @param[out] vendor The product ID the config is targeting.
	 */
	void load_device(const std::string &path, u16 &vendor, u16 &product)
	{
		INIReader ini {path};

		if (ini.ParseError() != 0)
			throw std::runtime_error(fmt::format("Failed to parse {}", path));

		vendor = 0;
		product = 0;

		this->get(ini, "Device", "Vendor", vendor);
		this->get(ini, "Device", "Product", product);
	}

	/*!
	 * Loads configuration data from a single file.
	 *
	 * @param[in] path The file to load and parse.
	 */
	void load_file(const std::string &path)
	{
		INIReader ini {path};

		if (ini.ParseError() != 0)
			throw std::runtime_error(fmt::format("Failed to parse {}", path));

		// clang-format off

		this->get(ini, "Config", "InvertX", m_config.invert_x);
		this->get(ini, "Config", "InvertY", m_config.invert_y);
		this->get(ini, "Config", "Width", m_config.width);
		this->get(ini, "Config", "Height", m_config.height);

		this->get(ini, "Touch", "Disable", m_config.touch_disable);
		this->get(ini, "Touch", "CheckCone", m_config.touch_check_cone);
		this->get(ini, "Touch", "CheckStability", m_config.touch_check_stability);
		this->get(ini, "Touch", "DisableOnPalm", m_config.touch_disable_on_palm);
		this->get(ini, "Touch", "DisableOnStylus", m_config.touch_disable_on_stylus);

		this->get(ini, "Contacts", "Detection", m_config.contacts_detection);
		this->get(ini, "Contacts", "Neutral", m_config.contacts_neutral);
		this->get(ini, "Contacts", "NeutralValue", m_config.contacts_neutral_value);
		this->get(ini, "Contacts", "ActivationThreshold", m_config.contacts_activation_threshold);
		this->get(ini, "Contacts", "DeactivationThreshold", m_config.contacts_deactivation_threshold);
		this->get(ini, "Contacts", "TemporalWindow", m_config.contacts_temporal_window);
		this->get(ini, "Contacts", "SizeMin", m_config.contacts_size_min);
		this->get(ini, "Contacts", "SizeMax", m_config.contacts_size_max);
		this->get(ini, "Contacts", "AspectMin", m_config.contacts_aspect_max);
		this->get(ini, "Contacts", "AspectMax", m_config.contacts_aspect_max);
		this->get(ini, "Contacts", "SizeThreshold", m_config.contacts_size_thresh);
		this->get(ini, "Contacts", "PositionThresholdMin", m_config.contacts_position_thresh_min);
		this->get(ini, "Contacts", "PositionThresholdMax", m_config.contacts_position_thresh_max);
		this->get(ini, "Contacts", "DistanceThreshold", m_config.contacts_distance_thresh);

		this->get(ini, "Stylus", "Disable", m_config.stylus_disable);

		this->get(ini, "Cone", "Angle", m_config.cone_angle);
		this->get(ini, "Cone", "Distance", m_config.cone_distance);

		this->get(ini, "DFT", "PositionMinAmp", m_config.dft_position_min_amp);
		this->get(ini, "DFT", "PositionMinMag", m_config.dft_position_min_mag);
		this->get(ini, "DFT", "PositionExp", m_config.dft_position_exp);
		this->get(ini, "DFT", "ButtonMinMag", m_config.dft_button_min_mag);
		this->get(ini, "DFT", "FreqMinMag", m_config.dft_freq_min_mag);
		this->get(ini, "DFT", "TiltMinMag", m_config.dft_tilt_min_mag);
		this->get(ini, "DFT", "TiltDistance", m_config.dft_tilt_distance);
		this->get(ini, "DFT", "TipDistance", m_config.dft_tip_distance);

		// clang-format on
	}

	template <class T>
	// NOLINTNEXTLINE(modernize-use-equals-delete)
	void get(const INIReader &ini, const std::string &section, const std::string &name,
		 T &value) const = delete;

	/*!
	 * Loads a boolean value from a config file.
	 *
	 * @param[in] ini The loaded file.
	 * @param[in] section The section where the option is found.
	 * @param[in] name The name of the config option.
	 * @param[in,out] value The default value as well as the destination of the new value.
	 */
	template <>
	void get<bool>(const INIReader &ini, const std::string &section, const std::string &name,
		       bool &value) const
	{
		value = ini.GetBoolean(section, name, value);
	}

	/*!
	 * Loads an integer value from a config file.
	 *
	 * @param[in] ini The loaded file.
	 * @param[in] section The section where the option is found.
	 * @param[in] name The name of the config option.
	 * @param[in,out] value The default value as well as the destination of the new value.
	 */
	template <>
	void get<u16>(const INIReader &ini, const std::string &section, const std::string &name,
		      u16 &value) const
	{
		value = gsl::narrow<u16>(ini.GetInteger(section, name, value));
	}

	/*!
	 * Loads an integer value from a config file.
	 *
	 * @param[in] ini The loaded file.
	 * @param[in] section The section where the option is found.
	 * @param[in] name The name of the config option.
	 * @param[in,out] value The default value as well as the destination of the new value.
	 */
	template <>
	void get<u32>(const INIReader &ini, const std::string &section, const std::string &name,
		      u32 &value) const
	{
		value = gsl::narrow<u32>(ini.GetInteger(section, name, value));
	}

	/*!
	 * Loads a floating point value from a config file.
	 *
	 * @param[in] ini The loaded file.
	 * @param[in] section The section where the option is found.
	 * @param[in] name The name of the config option.
	 * @param[in,out] value The default value as well as the destination of the new value.
	 */
	template <>
	void get<f32>(const INIReader &ini, const std::string &section, const std::string &name,
		      f32 &value) const
	{
		value = gsl::narrow_cast<f32>(ini.GetReal(section, name, value));
	}

	/*!
	 * Loads a floating point value from a config file.
	 *
	 * @param[in] ini The loaded file.
	 * @param[in] section The section where the option is found.
	 * @param[in] name The name of the config option.
	 * @param[in,out] value The default value as well as the destination of the new value.
	 */
	template <>
	void get<f64>(const INIReader &ini, const std::string &section, const std::string &name,
		      f64 &value) const
	{
		value = ini.GetReal(section, name, value);
	}

	/*!
	 * Loads a string value from a config file.
	 *
	 * @param[in] ini The loaded file.
	 * @param[in] section The section where the option is found.
	 * @param[in] name The name of the config option.
	 * @param[in,out] value The default value as well as the destination of the new value.
	 */
	template <>
	void get<std::string>(const INIReader &ini, const std::string &section,
			      const std::string &name, std::string &value) const
	{
		value = ini.GetString(section, name, value);
	}
};

} // namespace iptsd::config

#endif // IPTSD_CONFIG_LOADER_HPP