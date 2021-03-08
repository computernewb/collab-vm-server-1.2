#ifndef COLLAB_VM_SERVER_DATABASE_H
#define COLLAB_VM_SERVER_DATABASE_H

#include <map>
#include <memory>
#include <string>

#include "Config.h"
#include "VMSettings.h"

namespace CollabVM {

	/**
	 * Abstraction structure over the database.
	 */
	struct Database {
		Database();
		~Database();

		/**
		 * Save changes made to the configuration.
		 */
		void Save(Config& config);

		/**
		 * Save a newly created virtual machine to the DB and
		 * add it to the map. The virtual machine will be assigned
		 * a new ID.
		 */
		void AddVM(std::shared_ptr<VMSettings>& vm);

		/**
		 * Update a virtual machine's settings.
		 */
		void UpdateVM(std::shared_ptr<VMSettings>& vm);

		/**
		 * Remove a virtual machine from the DB and map.
		 */
		void RemoveVM(const std::string& name);

		/**
		 * Server Configuration
		 */
		Config Configuration;

		/**
		 * A map of all the virtual machines that the server manages.
		 */
		std::map<std::string, std::shared_ptr<VMSettings>> VirtualMachines;

	   private:
		/**
		 * Database implementation class.
		 */
		struct DbImpl;

		/**
		 * A unique_ptr<> to the underlying implementation of the database.
		 */
		std::unique_ptr<DbImpl> impl;
	};

} // namespace CollabVM

#endif