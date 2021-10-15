#ifndef COLLAB_VM_SERVER_CONFIG_H
#define COLLAB_VM_SERVER_CONFIG_H

#include <string>

// TODO: use <cstdint>
#include <stdint.h>

struct Config {
	// Default-constructor provides default server configuration.

	Config()
		: ID(0),
		  MasterPassword("collabvm"), // You're gonna wanna change this because any password we put here is gonna be insecure by default.
		  ModPassword("cvmmod"),	  // Change this one too if you enable the Moderator rank
		  MaxConnections(5),
		  ChatRateCount(4),
		  ChatRateTime(3),
		  ChatMuteTime(30),
		  TurnRateCount(80),
		  TurnRateTime(3),
		  TurnMuteTime(5),
		  NameRateCount(20),
		  NameRateTime(3),
		  NameMuteTime(10),
		  ChatMsgHistory(10),
		  BanCommand(""),
#ifdef USE_JPEG
		  JPEGQuality(75),
#else
		  JPEGQuality(255),
#endif
		  ModEnabled(false),
		  ModPerms(0) {
	}

	uint8_t ID;

	/* TODO: Add encryption to Master and Mod Passwords */
	std::string MasterPassword;
	std::string ModPassword;

	uint8_t MaxConnections;

	uint8_t ChatRateCount;
	uint8_t ChatRateTime;
	uint8_t ChatMuteTime;

	uint8_t ChatMsgHistory;

	uint8_t TurnRateCount;
	uint8_t TurnRateTime;
	uint8_t TurnMuteTime;

	uint8_t NameRateCount;
	uint8_t NameRateTime;
	uint8_t NameMuteTime;

	std::string BanCommand;

	uint8_t JPEGQuality;

	bool ModEnabled;

	uint16_t ModPerms;

	std::string BlacklistedNames;
};

#endif