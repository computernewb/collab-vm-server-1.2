#ifndef COLLAB_VM_SERVER_VMSETTINGS_H
#define COLLAB_VM_SERVER_VMSETTINGS_H

#include <string>
#include <stdint.h>

struct VMSettings {
	enum HypervisorEnum {
		kQEMU /* Only QEMU is supported right now */
	};

	enum SnapshotMode {
		kOff,		  // No snapshots
		kVMSnapshots, // Use loadvm command to load the snapshots
		kHDSnapshots  // Use hard disk snapshots
	};

	enum FileMode {
		kNone,
		kWhiteList,
		kBlackList
	};

	enum SocketType {
		kTCP,
		kLocal
	};

	std::string Name;

	uint8_t Hypervisor { HypervisorEnum::kQEMU };

	bool AutoStart = false;

	std::string DisplayName;
	std::string MOTD;
	std::string Description;

	bool RestoreOnShutdown = false;
	bool RestoreOnTimeout = false;

	bool TurnsEnabled = false;
	uint16_t TurnTime = 20;

	bool VotesEnabled = false;

	uint16_t VoteTime = 60;

	uint16_t VoteCooldownTime = 600;

	uint8_t MaxAttempts = 5;

	std::string Snapshot;
	std::string VNCAddress;
	uint16_t VNCPort {};

	// TODO: These settings will be moved into a controller specific JSON blob.

	uint8_t QMPSocketType { SocketType::kLocal };
	std::string QMPAddress;

	uint16_t QMPPort {};
	std::string QEMUCmd;
	uint8_t QEMUSnapshotMode { SnapshotMode::kOff };
};

#endif