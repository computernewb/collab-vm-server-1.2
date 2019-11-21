#pragma once
#include "GuacClient.h"
#include "GuacUser.h"
#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>
// Prevent libvncserver from redefining max macro
#undef max
#include "guacamole/guac_surface.h"
#include "guacamole/guac_cursor.h"
#include <chrono>
#include <sstream>

/**
* The maximum duration of a frame in milliseconds.
*/
#define GUAC_VNC_FRAME_DURATION 40

/**
* The amount of time to allow per message read within a frame, in
* milliseconds. If the server is silent for at least this amount of time, the
* frame will be considered finished.
*/
#define GUAC_VNC_FRAME_TIMEOUT 0

class CollabVMServer;
class VMController;
class GuacBroadcastSocket;
class GuacUser;

class GuacVNCClient : public GuacClient
{
public:
	GuacVNCClient(CollabVMServer& server, VMController& controller, UserList& users, const std::string& hostname, uint16_t port/*, uint16_t frame_duration*/);
	void Start() override;
	void Stop() override;
	void CleanUp() override;
	~GuacVNCClient();

private:

	void OnUserJoin(GuacUser& user);
	void OnUserLeave(GuacUser& user);
	void MouseHandler(GuacUser& user, int x, int y, int button_mask);
	void KeyHandler(GuacUser& user, int keysym, int pressed);
	void ClipboardHandler(GuacUser& user, guac_stream* stream, char* mimetype);

	static void guac_vnc_update(rfbClient* client, int x, int y, int w, int h);
	static void guac_vnc_copyrect(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y);
	static void guac_vnc_set_pixel_format(rfbClient* client, int color_depth);
	static rfbBool guac_vnc_malloc_framebuffer(rfbClient* rfb_client);
	static char* guac_vnc_get_password(rfbClient* rfb_client);
	static void guac_vnc_cursor(rfbClient* client, int x, int y, int w, int h, int bpp);
	int EndFrame();
	int GetProcessingLag();
	rfbClient* GetVNCClient();
	void VNCThread();
	void GenerateThumbnail();

	std::thread vnc_thread_;

	/**
	 * Pointer to the RFB client which is used by the mouse and
	 * key handlers for sending input to the VNC server.
	 */
	rfbClient* rfb_client_;

	/**
	* The original framebuffer malloc procedure provided by the initialized
	* rfbClient.
	*/
	MallocFrameBufferProc rfb_MallocFrameBuffer_;

	/**
	* Whether copyrect  was used to produce the latest update received
	* by the VNC server.
	*/
	int copy_rect_used_;

	/**
	* Client settings, parsed from args.
	*/
	/**
	* The password given in the arguments.
	*/
	char* password_;

	/**
	* Space-separated list of encodings to use within the VNC session.
	*/
	char* encodings_;

	/**
	* Whether the red and blue components of each color should be swapped.
	* This is mainly used for VNC servers that do not properly handle
	* colors.
	*/
	bool swap_red_blue_;

	/**
	* The color depth to request, in bits.
	*/
	int color_depth_;

	/**
	* Whether this connection is read-only, and user input should be dropped.
	*/
	bool read_only_;

	/**
	* Whether the cursor should be rendered on the server (remote) or on the
	* client (local).
	*/
	bool remote_cursor_;

	/**
	* Whether audio is enabled.
	*/
	bool audio_enabled_;

	/**
	* Mouse cursor.
	*/
	guac_common_cursor* cursor_;

	/**
	* Default surface.
	*/
	guac_common_surface* default_surface_;

	char* vnc_settings_[9];

	static char* GUAC_VNC_CLIENT_KEY;

	const CollabVMServer& server_;

};

