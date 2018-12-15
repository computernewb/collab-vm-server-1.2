#include <iostream>

#include <odb/connection.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include "Database.h"
#include "Config-odb.hxx"
#include "VMSettings-odb.hxx"

namespace CollabVM
{
	using odb::transaction;

	Database::Database()
	{
		db_ = new odb::sqlite::database(
			"collab-vm.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

		odb::connection_ptr c(db_->connection());
		c->execute("PRAGMA foreign_keys=OFF");

		transaction t(c->begin());

		// Test to see if a Config object exists in the database
		// If it doesn't then an exception will be thrown and the
		// database schema should be created.
		try
		{
			db_->query<Config>(false);
		}
		catch (const odb::exception& e)
		{
			odb::schema_catalog::create_schema(*db_);

			std::cout << "A new database has been created" << std::endl;
		}

		// Load server configuration from database
		if (!db_->query_one(Configuration))
		{
			// Create a new configuration object
			db_->persist(Configuration);
		}

		// Load all the VMs and add them to the map
		odb::result<VMSettings> vms = db_->query<VMSettings>(true);
		for (auto it = vms.begin(); it != vms.end(); it++)
		{
			VirtualMachines[it->Name] = std::make_shared<VMSettings>(*it);
		}

		t.commit();

		c->execute("PRAGMA foreign_keys=ON");
	}

	void Database::Save(Config& config)
	{
		transaction t(db_->begin());

		Configuration = config;
		db_->update(Configuration);

		t.commit();
	}

	void Database::AddVM(std::shared_ptr<VMSettings>& vm)
	{
		transaction t(db_->begin());

		db_->persist(*vm);

		t.commit();

		// Add the VM to the map
		VirtualMachines[vm->Name] = vm;
	}

	void Database::UpdateVM(std::shared_ptr<VMSettings>& vm)
	{
		transaction t(db_->begin());

		db_->update(*vm);

		t.commit();
	}

	void Database::RemoveVM(const std::string& name)
	{
		auto it = VirtualMachines.find(name);
		if (it == VirtualMachines.end())
			return;

		// The name parameter could be a reference to the key in the map
		// so the VM should be erased from the database first, and then
		// from the map
		transaction t(db_->begin());

		db_->erase<VMSettings>(name);

		t.commit();
		
		VirtualMachines.erase(it);
	}

	Database::~Database()
	{
		delete db_;
	}
}