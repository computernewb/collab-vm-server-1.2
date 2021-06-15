#include <iostream>
#include <sqlite_orm/sqlite_orm.h>

#include "Database.h"

namespace CollabVM {

	/**
	 * Factory function to build the storage type.
	 *
	 * \param[in] filename Filename of the database file. Is empty when doing decltype()... so we can do that
	 */
	static auto MakeCollabVMStorage(const char* filename = "") {
		using namespace sqlite_orm;
		return make_storage(filename,
							// Main configuration table
							make_table("Config",
									   make_column("ID", &Config::ID, autoincrement(), primary_key()),
									   make_column("MasterPassword", &Config::MasterPassword),
									   make_column("ModPassword", &Config::ModPassword),
									   make_column("MaxConnections", &Config::MaxConnections),
									   make_column("ChatRateCount", &Config::ChatRateCount),
									   make_column("ChatRateTime", &Config::ChatRateTime),
									   make_column("ChatMuteTime", &Config::ChatMuteTime),
									   make_column("ChatMsgHistory", &Config::ChatMsgHistory),
									   make_column("TurnRateCount", &Config::TurnRateCount),
									   make_column("TurnRateTime", &Config::TurnRateTime),
									   make_column("TurnMuteTime", &Config::TurnMuteTime),
									   make_column("NameRateCount", &Config::NameRateCount),
									   make_column("NameRateTime", &Config::NameRateTime),
									   make_column("NameMuteTime", &Config::NameMuteTime),
									   make_column("MaxUploadTime", &Config::MaxUploadTime),
									   make_column("BanCommand", &Config::BanCommand),
									   make_column("JPEGQuality", &Config::JPEGQuality),
									   make_column("ModEnabled", &Config::ModEnabled),
									   make_column("ModPerms", &Config::ModPerms),
									   make_column("BlacklistedNames", &Config::BlacklistedNames)),
							// VMSettings table
							make_table("VMSettings",
									   make_column("Name", &VMSettings::Name, primary_key()),
									   make_column("Hypervisor", &VMSettings::Hypervisor),
									   make_column("AutoStart", &VMSettings::AutoStart),
									   make_column("DisplayName", &VMSettings::DisplayName),
									   make_column("MOTD", &VMSettings::MOTD),
									   make_column("Description", &VMSettings::Description),
									   make_column("RestoreOnShutdown", &VMSettings::RestoreOnShutdown),
									   make_column("RestoreOnTimeout", &VMSettings::RestoreOnTimeout),
									   make_column("RestoreHeartbeat", &VMSettings::RestoreHeartbeat),
									   make_column("AgentEnabled", &VMSettings::AgentEnabled),
									   make_column("AgentSocketType", &VMSettings::AgentSocketType),
									   make_column("AgentUseVirtio", &VMSettings::AgentUseVirtio),
									   make_column("AgentAddress", &VMSettings::AgentAddress),
									   make_column("AgentPort", &VMSettings::AgentPort),
									   make_column("TurnsEnabled", &VMSettings::TurnsEnabled),
									   make_column("TurnTime", &VMSettings::TurnTime),
									   make_column("VotesEnabled", &VMSettings::VotesEnabled),
									   make_column("VoteTime", &VMSettings::VoteTime),
									   make_column("VoteCooldownTime", &VMSettings::VoteCooldownTime),
									   make_column("MaxAttempts", &VMSettings::MaxAttempts),
									   make_column("UploadsEnabled", &VMSettings::UploadsEnabled),
									   make_column("UploadCooldownTime", &VMSettings::UploadCooldownTime),
									   make_column("MaxUploadSize", &VMSettings::MaxUploadSize),
									   make_column("UploadMaxFilename", &VMSettings::UploadMaxFilename),
									   make_column("Snapshot", &VMSettings::Snapshot),
									   make_column("VNCAddress", &VMSettings::VNCAddress),
									   make_column("VNCPort", &VMSettings::VNCPort),
									   make_column("QMPSocketType", &VMSettings::QMPSocketType),
									   make_column("QMPAddress", &VMSettings::QMPAddress),
									   make_column("QMPPort", &VMSettings::QMPPort),
									   make_column("QEMUCmd", &VMSettings::QEMUCmd),
									   make_column("QEMUSnapshotMode", &VMSettings::QEMUSnapshotMode),
									   make_column("VMPassword", &VMSettings::VMPassword)));
	}

	struct Database::DbImpl {
		/**
		 * Storage datatype returned from our sqlite_orm "factory" function.
		 */
		using Storage = decltype(MakeCollabVMStorage());

		/**
		 * Constructor. Makes the storage type for us.
		 */
		DbImpl()
			: storage(MakeCollabVMStorage("collab-vm.db")) {
		}

		Storage storage;
	};

	Database::Database() {
		impl = std::make_unique<Database::DbImpl>();

		try {
			// attempt to fetch the config first
			impl->storage.get<Config>(1);
		} catch(...) {
			impl->storage.sync_schema(false);

			// create a Config in the database from the sample, default-constructed
			// configuration in Config.h
			impl->storage.transaction([&]() {
				impl->storage.insert(Configuration);
				return true;
			});

			// An iconic message, that will ripple past and future generations
			// together to remember our founding fathers
			std::cout << "A new database has been created\n";
		}

		// There should only be one Config in the database
		Configuration = impl->storage.get<Config>(1);

		// Get all VMSettings...

		try {
			auto vms = impl->storage.get_all<VMSettings>();
			for(auto& vm : vms) {
				VirtualMachines[vm.Name] = std::make_shared<VMSettings>(vm);
			}
		} catch(...) {
			// well you too then buddy
		}
	}

	// TODO: some of this is a littttle bit wonky

	void Database::Save(Config& config) {
		impl->storage.transaction([&]() {
			impl->storage.update(config);
			Configuration = config;
			return true;
		});
	}

	void Database::AddVM(std::shared_ptr<VMSettings>& vm) {
		if(!vm)
			return;

		impl->storage.transaction([&]() {
			impl->storage.replace<VMSettings>(*vm);

			// Add the VMSettings to the map.
			VirtualMachines[vm->Name] = vm;
			return true;
		});
	}

	void Database::UpdateVM(std::shared_ptr<VMSettings>& vm) {
		if(!vm)
			return;

		impl->storage.transaction([&]() {
			impl->storage.update<VMSettings>(*vm);
			return true;
		});
	}

	void Database::RemoveVM(const std::string& name) {
		auto it = VirtualMachines.find(name);
		if(it == VirtualMachines.end())
			return;

		impl->storage.transaction([&]() {
			impl->storage.remove<VMSettings>(it->second->Name);

			// Erase the map
			VirtualMachines.erase(it);
			return true;
		});
	}

	// Implement the default destructor in this TU so that
	// the unique_ptr<DbImpl> can actually function
	Database::~Database() = default;
} // namespace CollabVM