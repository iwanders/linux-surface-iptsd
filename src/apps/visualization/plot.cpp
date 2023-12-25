// SPDX-License-Identifier: GPL-2.0-or-later

#include "visualize-png.hpp"

#include <common/types.hpp>
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

namespace iptsd::apps::visualization::plot {
namespace {

int run(const int argc, const char **argv)
{
	CLI::App app {"Utility for rendering captured touchscreen inputs to PNG frames."};

	std::filesystem::path path {};
	app.add_option("DATA", path)
		->description("A binary data file containing touch reports.")
		->type_name("FILE")
		->required();

	std::filesystem::path output {};
	app.add_option("OUTPUT", output)
		->description("The directory where the rendered frames are saved.")
		->type_name("DIR")
		->required();


	iptsd::apps::visualization::PlotConfig config;

	app.add_option("--start-index", config.start_index)
		->description("Only plot frames after this index.")
		->default_val(config.start_index);
	app.add_option("--end-index", config.end_index)
		->description("Only plot frames before this index.")
		->default_val(config.end_index);
	app.add_option("--plot-nth", config.plot_nth)
		->description("Only plot frames that are a multiple of this")
		->default_val(config.plot_nth);


	CLI11_PARSE(app, argc, argv);

	// Create a plotting application that reads from a file.
	core::linux::FileRunner<VisualizePNG> visualize {path, output};
	visualize.application().set_config(config);

	const auto _sigterm = core::linux::signal<SIGTERM>([&](int) { visualize.stop(); });
	const auto _sigint = core::linux::signal<SIGINT>([&](int) { visualize.stop(); });

	if (!visualize.run())
		return EXIT_FAILURE;

	return 0;
}

} // namespace
} // namespace iptsd::apps::visualization::plot

int main(const int argc, const char **argv)
{
	spdlog::set_pattern("[%X.%e] [%^%l%$] %v");

	try {
		return iptsd::apps::visualization::plot::run(argc, argv);
	} catch (std::exception &e) {
		spdlog::error(e.what());
		return EXIT_FAILURE;
	}
}
