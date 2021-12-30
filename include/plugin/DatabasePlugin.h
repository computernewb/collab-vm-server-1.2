//
// Created by lily on 12/9/21.
//

#ifndef COLLAB_VM_SERVER_DATABASEPLUGIN_H
#define COLLAB_VM_SERVER_DATABASEPLUGIN_H

#include "PluginAbi.h"

namespace collabvm::plugin {

	// Datatype for snowflakes.
	using snowflake_t = std::uint64_t;

	// base class for ORM objects created by
	// an implementation of IDatabasePlugin
	struct IDbObject {
		// delete self. Only use to collect the object once it's done being used.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IDbObject, void, DestroySelf);

		// should probably be an error code or something

		// Try updating the DB.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IDbObject, bool, TryUpdating);
	};

	// unique_ptr like thing for holding a db object
	// in a safe fashion for a one-owner model
	template<class T>
	struct UniqueDbObject {
		constexpr UniqueDbObject() = default;

		constexpr UniqueDbObject(T* handle)
			: m_handle(handle) {
		}

		// do not copy or move
		// actually, moving might be nice
		// (could just be m_handle = other.Release();)
		UniqueDbObject(const UniqueDbObject&) = delete;
		UniqueDbObject(UniqueDbObject&&) = delete;

		constexpr ~UniqueDbObject() {
			Reset(nullptr);
		}

		constexpr void Reset(T* ptr) {
			if(m_handle != nullptr)
				m_handle->DestroySelf();

			m_handle = ptr;
		}

		/**
		 * Release the pointer. You will have to clean it up yourself
		 */
		constexpr T* Release() {
			auto h = m_handle;
			m_handle = nullptr;
			return h;
		}

		constexpr T* Get() {
			return m_handle;
		}

		constexpr const T* Get() const {
			return m_handle;
		}

		constexpr explicit operator T*() {
			return m_handle;
		}

		constexpr explicit operator const T*() const {
			return m_handle;
		}

	   private:
		T* m_handle {};
	};

	// Interface for a user inside of the database
	struct IUserEntry : public IDbObject {
		// simple getters
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUserEntry, snowflake_t, GetSnowflake);

		// Get the hashed password for this user.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUserEntry, const utf8char*, GetHashedPassword);

		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUserEntry, const utf8char*, GetUserName);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUserEntry, void, SetUserName, const utf8char*);

		// Change the password. Server takes care of hashing the password securely,
		// the database plugin just needs to store it.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUserEntry, void, ChangePassword, const utf8char* newPassword);

		// role management
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUserEntry, void, AddRole, snowflake_t roleSnowflake);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUserEntry, void, RemoveRole, snowflake_t roleSnowflake);
	};

	// VM ORM repressentation
	struct IVMEntry : public IDbObject {
		// data type for hypervisor "enum"
		using Hypervisor = std::uint16_t;

		// reserved value for invalid hypervisor.
		// If writing a hv plugin, please don't use this value.
		constexpr static Hypervisor INVALID_HYPERVISOR = -1;

		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVMEntry, const utf8char*, GetName);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVMEntry, Hypervisor, GetHypervisorCode);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVMEntry, const utf8char*, GetDescription);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVMEntry, const utf8char*, GetMOTD);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVMEntry, const utf8char*, GetJsonBlob);

		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVMEntry, void, SetName, const utf8char* newName);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVMEntry, void, SetHypervisorCode, Hypervisor hypervisor);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVMEntry, void, SetDescription, const utf8char*);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVMEntry, void, SetMOTD, const utf8char* newMotd);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVMEntry, void, SetJsonBlob, const utf8char*);
	};

	struct IIpDataEntry : public IDbObject {
		enum class CoolDownType {
			Temporary,
			Perm
		};

		enum class BanType {
			None, // not banned at all!
			Cooldown, // cooldown for hitting a rate-limit
			Forever // banned forever
		};

		// TODO: Getters and setters for this data
	};

	/**
	 * Interface for CollabVM Database plugins
	 */
	struct IDatabasePlugin {


		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IDatabasePlugin, void, InitSchema);

		// Create a new VM controller entry in the database
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IDatabasePlugin, IVMEntry*, MaybeCreateVMController);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IDatabasePlugin, IUserEntry*, MaybeCreateUser);

		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IDatabasePlugin, IVMEntry*, MaybeGetVMControllerById, const utf8char* id);

		// Maybe get a user by snowflake.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IDatabasePlugin, IUserEntry*, MaybeGetUserEntryBySnow, snowflake_t snowflake);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IDatabasePlugin, IUserEntry*, MaybeGetUserEntryByName, const utf8char* username);

		// Try deleting a user from the database.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IDatabasePlugin, bool, TryDeleteUser, snowflake_t snowflake);
	};

} // namespace collabvm::plugin

#endif //COLLAB_VM_SERVER_DATABASEPLUGIN_H
