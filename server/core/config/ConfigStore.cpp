//
// Created by lily on 8/22/22.
//

#include <core/config/ConfigStore.hpp>

namespace collab3::core {

	ConfigStore::ArrayProxy ConfigStore::operator[](const ConfigStore::ConfigKey& key) {
		return ArrayProxy(*this, key);
	}

	ConfigStore::ConfigValue& ConfigStore::ArrayProxy::MaybeFetchValue() const {
		if(!Exists())
			throw NonExistentValue();

		return underlyingStore.valueMap[key];
	}

	void ConfigStore::ArrayProxy::Remove() {
		if(Exists())
			underlyingStore.valueMap.erase(key);
	}

	void ConfigStore::ArrayProxy::SetBase(const ConfigStore::ConfigValue& value) {
		underlyingStore.valueMap[key] = value;
	}

	bool ConfigStore::ArrayProxy::Exists() const noexcept {
		return underlyingStore.valueMap.contains(key);
	}

}