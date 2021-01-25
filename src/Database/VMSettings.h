#pragma once
#include <string>
#include <stdint.h>
#include <odb/core.hxx>

#pragma db object
struct VMSettings
{
	enum HypervisorEnum
	{
		kQEMU,
		kVirtualBox,
		kVMWare
	};

	enum SnapshotMode
	{
		kOff,			// No snapshots
		kVMSnapshots,	// Uses the loadvm command to restore snapshots including
						// the CPU and RAM of the VM
		kHDSnapshots	// Restores snapshots from a hard drive image
	};

	enum FileMode
	{
		kNone,
		kWhiteList,
		kBlackList
	};

	enum SocketType
	{
		kTCP,
		kLocal	// Unix domain socket or named pipe
	};

	VMSettings() :
		Hypervisor(kQEMU),
		AutoStart(false),
		RestoreOnShutdown(false),
		RestoreOnTimeout(false),
		RestoreHeartbeat(false),
		//RestorePeriodically(false),
		//PeriodRestoreTime(3600),
		AgentEnabled(false),
		AgentSocketType(kLocal),
		AgentUseVirtio(false),
		TurnsEnabled(false),
		TurnTime(20), // 20 seconds
		VotesEnabled(false),
		VoteTime(60),
		VoteCooldownTime(600),
		QMPSocketType(kLocal),
		MaxAttempts(5),
		//UploadsEnabled(false),
		//UploadWaitTime(180), // 3 minutes
		//MaxUploadSize(15728640), // 15 MiB
		//FileUploadMode(NONE),
		QEMUSnapshotMode(kOff)
	{
	}

#pragma db id
	std::string Name;
	HypervisorEnum Hypervisor;
	bool AutoStart;
	std::string DisplayName;
	std::string Description;
	bool RestoreOnShutdown;
	bool RestoreOnTimeout;
	bool RestoreHeartbeat;
	uint32_t HeartbeatTimeout; // seconds
	//bool RestorePeriodically;
	//uint32_t PeriodRestoreTime;

	bool AgentEnabled;
	SocketType AgentSocketType;
	bool AgentUseVirtio;
	std::string AgentAddress;
	uint16_t AgentPort;

	bool TurnsEnabled;
	uint16_t TurnTime; // Measured in seconds

	bool VotesEnabled;

	/**
	 * The amount of time a vote lasts (seconds).
	 */
	uint16_t VoteTime;

	/**
	 * The amount of time in between votes (seconds).
	 */
	uint16_t VoteCooldownTime;

	/**
	 * The maximum amount of attempts to connect to the hypervisor
	 * with either the Guacamole client or some other client.
	 */
	uint8_t MaxAttempts;

	bool UploadsEnabled;
	uint32_t UploadCooldownTime; // Measured in seconds
	uint32_t MaxUploadSize; // Measured in bytes
	uint8_t UploadMaxFilename;
	//FileMode FileUploadMode;

	// Possibly hypervisor-specific
	std::string Snapshot;
	std::string VNCAddress;
	uint16_t VNCPort;
	SocketType QMPSocketType;
	std::string QMPAddress;
	uint16_t QMPPort;
	// The command for starting QEMU 
	std::string QEMUCmd;
	SnapshotMode QEMUSnapshotMode;
};
