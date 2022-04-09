//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#include "PluginManager.h"

#include <spdlog/spdlog.h>

namespace collab3::core {

	bool PluginManager::Init() {
		// TODO: Gotta rethink this

		if(!std::filesystem::is_directory(std::filesystem::current_path() / "plugins") || !std::filesystem::exists(std::filesystem::current_path() / "plugins")) {
			spdlog::info("PluginManager: Plugins folder not found. Creating folder.");

			try {
				std::filesystem::create_directory(std::filesystem::current_path() / "plugins");
			} catch(...) {
				spdlog::error("PluginManager: Could not create plugins folder");
				return false;
			}
		}

		for(auto& it : std::filesystem::directory_iterator(std::filesystem::current_path() / "plugins")) {
			spdlog::info("Going to load plugin {}", it.path().string());
			if(!this->LoadPlugin(it.path())) {
				spdlog::warn("Plugin {} failed to load :(", it.path().string());
				continue;
			}
		}

		// FIXME: would there ever be a fatal case?
		return true;
	}

	void PluginManager::UnloadPlugins() {
		// TODO: implement me
		spdlog::info("PluginManager::UnloadPlugins: TODO");
	}

	bool PluginManager::LoadPlugin(const std::filesystem::path& path) {
		spdlog::info("PluginManager::LoadPlugin: TODO: Need to implement JS engine stuff");
		return true;
	}

} // namespace collab3::core
