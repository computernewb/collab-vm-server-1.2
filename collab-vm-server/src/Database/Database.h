#pragma once
#include <map>
#include <memory>
#include <string>
#include <odb/database.hxx>

#include "Config.h"
#include "VMSettings.h"

namespace CollabVM
{
	class Database
	{
	public:
		Database();

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

		~Database();

		/**
		 * Server Configuration
		 */
		Config Configuration;

		/**
		 * A map of all the virtual machines that the server manages.
		 */
		std::map<std::string, std::shared_ptr<VMSettings>> VirtualMachines;

	private:
		odb::database* db_;
	};
}
