// SPDX-License-Identifier: GPL-2.0-or-later

#include "config.hpp"
#include "context.hpp"
#include "devices.hpp"
#include "stylus.hpp"
#include "touch.hpp"

#include <common/signal.hpp>
#include <common/types.hpp>
#include <ipts/control.hpp>
#include <ipts/ipts.h>
#include <ipts/parser.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fmt/format.h>
#include <functional>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <thread>

using namespace std::chrono;

namespace iptsd::daemon {

static bool iptsd_loop(Context &ctx)
{
	u32 doorbell = ctx.control.doorbell();
	u32 diff = doorbell - ctx.control.current_doorbell;

	while (doorbell > ctx.control.current_doorbell) {
		ctx.control.read(ctx.parser.buffer());

		try {
			ctx.parser.parse();
		} catch (std::out_of_range &e) {
			spdlog::error(e.what());
		}

		ctx.control.send_feedback();
	}

	return diff > 0;
}

static int main()
{
	Context ctx {};

	std::atomic_bool should_exit {false};

	auto const _sigterm = common::signal<SIGTERM>([&](int) { should_exit = true; });
	auto const _sigint = common::signal<SIGINT>([&](int) { should_exit = true; });

	struct ipts_device_info info = ctx.control.info;
	system_clock::time_point timeout = system_clock::now() + 5s;

	spdlog::info("Connected to device {:04X}:{:04X}", info.vendor, info.product);

	ctx.parser.on_stylus = [&](const auto &data) { iptsd_stylus_input(ctx, data); };
	ctx.parser.on_heatmap = [&](const auto &data) { iptsd_touch_input(ctx, data); };

	while (true) {
		if (iptsd_loop(ctx))
			timeout = system_clock::now() + 5s;

		std::this_thread::sleep_for(timeout > system_clock::now() ? 10ms : 200ms);

		if (should_exit) {
			spdlog::info("Stopping");

			return EXIT_FAILURE;
		}
	}

	return 0;
}

} // namespace iptsd::daemon

int main()
{
	spdlog::set_pattern("[%X.%e] [%^%l%$] %v");

	try {
		return iptsd::daemon::main();
	} catch (std::exception &e) {
		spdlog::error(e.what());
		return EXIT_FAILURE;
	}
}
