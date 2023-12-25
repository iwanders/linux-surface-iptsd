// SPDX-License-Identifier: GPL-2.0-or-later

#include "print-dft.hpp"

#include <common/types.hpp>
#include <core/linux/device-runner.hpp>
#include <core/linux/file-runner.hpp>
#include <core/linux/signal-handler.hpp>

#include <CLI/CLI.hpp>
#include <gsl/gsl>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>

namespace iptsd::apps::print {
namespace {

template <typename ApplicationType>
int runApplication(const std::filesystem::path &path, const PrintConfig &config)
{
	ApplicationType app {path};
	app.application().set_config(config);

	const auto _sigterm = core::linux::signal<SIGTERM>([&](int) { app.stop(); });
	const auto _sigint = core::linux::signal<SIGINT>([&](int) { app.stop(); });

	if (!app.run())
		return EXIT_FAILURE;

	return 0;
}

int run(const int argc, const char **argv)
{
	CLI::App app {"Utility for printing dft values."};

	std::filesystem::path path {};
	app.add_option("DEVICE_OR_FILE", path)
		->description("The hidraw device node of the touchscreen.")
		->type_name("FILE")
		->required();

	PrintConfig config;

	app.add_flag("--stylus-status", config.stylus_status)
		->description("Print the stylus status with each window.");
	app.add_flag("--dft-button", config.dft_id_button)
		->description("Print IPTS_DFT_ID_BUTTON type dft windows.");
	app.add_flag("--dft-position", config.dft_id_position)
		->description("Print IPTS_DFT_ID_POSITION type dft windows.");
	app.add_flag("--dft-position2", config.dft_id_position2)
		->description("Print IPTS_DFT_ID_POSITION2 type dft windows.");
	app.add_flag("--dft-pressure", config.dft_id_pressure)
		->description("Print IPTS_DFT_ID_PRESSURE type dft windows.");
	app.add_flag("--dft-unknown", config.dft_id_unknown)
		->description("Print unknown type dft windows, use for example with "
			      "'... --dft-unknown | grep -A 20 IPTS_DFT_ID_8");

	app.add_option("--output-json", config.output_json)
		->description("Dump collected to a json file at the end.");

	CLI11_PARSE(app, argc, argv);

	// Quick and dirty...
	if (path.parent_path() == "/dev") {
		return runApplication<core::linux::DeviceRunner<PrintDft>>(path, config);
	} else {
		return runApplication<core::linux::FileRunner<PrintDft>>(path, config);
	}
}

} // namespace
} // namespace iptsd::apps::print

int main(const int argc, const char **argv)
{
	spdlog::set_pattern("[%X.%e] [%^%l%$] %v");

	try {
		return iptsd::apps::print::run(argc, argv);
	} catch (std::exception &e) {
		spdlog::error(e.what());
		return EXIT_FAILURE;
	}
}
