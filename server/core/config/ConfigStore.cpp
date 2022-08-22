//
// Created by lily on 8/22/22.
//

#include <server/core/config/ConfigStore.h>
#include <stdexcept>

namespace collab3::core {

	bool ConfigStore::ValueExists(const ConfigStore::ConfigKey& key) const {
		return valueMap.find(key) != valueMap.end();
	}

	ConfigStore::ArrayProxy ConfigStore::operator[](const ConfigStore::ConfigKey& key) {
		if(!ValueExists(key))
			throw std::out_of_range("Key does not exist");

		return ArrayProxy(valueMap[key]);
	}

}