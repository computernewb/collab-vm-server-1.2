//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#include <core/config/ConfigStore.hpp>

namespace collab3::core {

	ConfigStore::ArrayProxy ConfigStore::operator[](const ConfigStore::ConfigKey& key) noexcept {
		return ArrayProxy(*this, key);
	}

	ConfigStore::Result<ConfigStore::ConfigValue> ConfigStore::ArrayProxy::MaybeFetchValue() const noexcept {
		if(!Exists())
			return tl::unexpected(ConfigStoreErrorCode::ValueNonExistent);

		return underlyingStore.valueMap[key];
	}

	void ConfigStore::ArrayProxy::Remove() noexcept {
		if(Exists())
			underlyingStore.valueMap.erase(key);
	}

	void ConfigStore::ArrayProxy::SetBase(const ConfigStore::ConfigValue& value) noexcept {
		underlyingStore.valueMap[key] = value;
	}

	bool ConfigStore::ArrayProxy::Exists() const noexcept {
		return underlyingStore.valueMap.contains(key);
	}

}