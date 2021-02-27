#pragma once
#include <string>
#include <stdint.h>
#include <odb/core.hxx>

#pragma db object
struct Config
{
	// File extensions supported by Windows Photo Viewer and Windows Media Player
#define IMAGE_EXTS "bmp;dib;ico;jpg;jpeg;jpe;jif;jfif;jfi;tiff;tif;png;"
#define VIDEO_EXTS "asf;wma;wmv;wm;asx;wax;wvx;wmx;wpl;dvr-ms;wmd;avi;wdp;" \
					"mpg;mpeg;m1v;mp2;mp3;mpa;mpe;m3u;mid;midi;rmi;aif;aifc;" \
					"aiff;au;snd;wav;cda;ivf;wmz;wms;mov;m4a;mp4;m4v;mp4v;" \
					"3g2;3gp2;3gp;3gpp;aac;adt;adts;m2ts"
	// Executable file types
#define EXEC_EXTS "exe;com;scr;pif;bat;cmd;vb;vbs;vbe;js;jse;application;" \
					"gadget;msi;msp;hta;cpl;msc;jar;ws;wsf;wsc;wsh;ps1;ps1xml;" \
					"ps2;ps2xml;psc1;psc2;msh;msh1;msh2;mshxml;msh1xml;msh2xml;" \
					"scf;lnk;inf;reg;dll;sys;txt;"

	Config() :
		ID(0),
		MasterPassword("collabvm"), // You're gonna wanna change this because any password we put here is gonna be insecure by default.
		ModPassword("cvmmod"), // Change this one too if you enable the Moderator rank
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
		MaxUploadTime(120),
		BanCommand(""),
#ifdef USE_JPEG
		JPEGQuality(75),
#else
		JPEGQuality(255),
#endif
		ModEnabled(false),
		ModPerms(0)
	{
	}

#pragma db id
	uint8_t ID;

	/**
	 * Admin panel password.
	 */
	std::string MasterPassword;

	/**
	 * Moderator password.
	 */
	std::string ModPassword;

	//unsigned char FrameRate; // Measured in frames per second

	/**
	 * Max number of connections from a single IP.
	 */
	uint8_t MaxConnections;

	/**
	 * Maximum number of chat messages that can be send in the 
	 * specified amount of time.
	 */
	uint8_t ChatRateCount;
	uint8_t ChatRateTime;

	/**
	 * The amount of time a user will be muted for if the attempt
	 * to exceed the chat rate.
	 */
	uint8_t ChatMuteTime;

	/**
	 * Max number of chat messages to remember.
	 */
	uint8_t ChatMsgHistory;

	/**
	 * Maximum number of turns that can be sent in the 
	 * specified amount of time.
	 */
	uint8_t TurnRateCount;
	uint8_t TurnRateTime;

	/**
         * The amount of time a user will be turn limited for if the attempt
         * to exceed the chat rate.
         */
	uint8_t TurnMuteTime;

	/* name shit */
	uint8_t NameRateCount;
	uint8_t NameRateTime;
	uint8_t NameMuteTime;

	/**
	 * The max amount of time the user has to upload a file in seconds.
	 */
	uint16_t MaxUploadTime;
	
	/**
	 * The command used when an admin bans a user.
	 */
	std::string BanCommand;
	
	/**
	 * JPEG compression quality.
	 */
	uint8_t JPEGQuality;

	/**
	 * Moderator rank enabled?
	 */
	bool ModEnabled;

	/**
	 * Moderator permissions.
	 */
	uint16_t ModPerms;


	//FileMode FileUploadMode;
	//std::string BlackList;
	//std::string WhiteList;

};
