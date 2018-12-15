#pragma once
#include "GuacBroadcastSocket.h"
#include "GuacUser.h"
#include "guacamole/timestamp.h"
#include "guacamole/pool.h"
#include "guacamole/layer.h"
#include "guacamole/stream.h"
#include "guacamole/protocol.h"
#include "UserList.h"
#include <string>
#include <stdint.h>
#include <mutex>
#include <thread>
#include <vector>
#include <condition_variable>

class CollabVMServer;
class VMController;
class GuacBroadcastSocket;
class GuacUser;

/**
 * Abstract class for a Guacamole client that contains a list of users
 * and connects to an endpoint using a protocol that is defined in derived
 * classes.
 */
class GuacClient
{
	friend GuacBroadcastSocket;
public:
	enum ClientState
	{
		kStopped,		// VNC thread stopped
		kStarting,		// VNC thread is starting
		kIdle,			// Waiting for VNC server to start
		kConnecting,	// Attempting to connect to the VNC server
		kConnected,		// Connected to the VNC server
		kDisconnecting	// Disconnecting
	};

	enum DisconnectReason
	{
		kFailed,		// Failed to connect
		kClient,		// Client disconnected
		kServer,		// Server disconnected
		kProtocolError	// Protocol error
	};

	GuacClient(CollabVMServer& server, VMController& controller, UserList& users, const std::string& hostname, uint16_t port, uint16_t frame_duration);

	/**
	 * Start the client thread.
	 */
	virtual void Start() = 0;

	/**
	 * Stop the client thread.
	 */
	virtual void Stop() = 0;

	virtual void SetEndpoint(const std::string& hostname, uint16_t port);

	/**
	* Disconnects all users and frees any resources used
	* by the controller. This function is called from the processing thread to
	* prevent
	*/
	virtual void CleanUp() = 0;

	/**
	 * Attempt to connect to the server.
	 */
	void Connect();

	/**
	 * Disconnect from the server.
	 */
	void Disconnect();

	/**
	 * Add the user to the Guacamole client.
	 */
	void AddUser(GuacUser& user);

	/**
	 * Remove the user from the Guacamole client.
	 */
	void RemoveUser(GuacUser& user);

	/**
	 * Returns true if the client is connected.
	 */
	bool Connected() const;

	inline ClientState GetState() const
	{
		return client_state_;
	}

	inline void SetFrameDuration(uint16_t duration)
	{
		frame_duration_ = std::chrono::duration<uint16_t, std::milli>(duration);
	}

	inline DisconnectReason GetDisconnectReason()
	{
		return disconnect_reason_;
	}

	guac_layer* AllocLayer();
	void FreeLayer(guac_layer* layer);

	guac_layer* AllocBuffer();
	void FreeBuffer(guac_layer* layer);

	/**
	 * User instruction handlers
	 */
	void HandleSync(GuacUser& user, std::vector<char*>& args);
	void HandleMouse(GuacUser& user, std::vector<char*>& args);
	void HandleKey(GuacUser& user, std::vector<char*>& args);
	void HandleClipboard(GuacUser& user, std::vector<char*>& args);
	//void HandleFile(GuacUser& user, std::vector<char*>& args);
	//void HandlePipe(GuacUser& user, std::vector<char*>& args);
	//void HandleAck(GuacUser& user, std::vector<char*>& args);
	//void HandleBlob(GuacUser& user, std::vector<char*>& args);
	//void HandleEnd(GuacUser& user, std::vector<char*>& args);
	//void HandleSize(GuacUser& user, std::vector<char*>& args);
	void HandleDisconnect(GuacUser& user, std::vector<char*>& args);

	void Log(guac_client_log_level level, const char* format, ...);

	void UpdateThumbnail()
	{
		update_thumbnail_ = true;
	}

	virtual ~GuacClient();

	GuacBroadcastSocket broadcast_socket_;

	/**
	* The time (in milliseconds) that the last sync message was sent to the
	* client.
	*/
	guac_timestamp last_sent_timestamp;

	static guac_layer* GUAC_DEFAULT_LAYER;

protected:
	/**
	* Callback in derived classes for when a user first connects
	* to the Guacamole client.
	*/
	virtual void OnUserJoin(GuacUser& user) = 0;

	/**
	* Callback in derived classes for when a user first connects
	* to the Guacamole client.
	*/
	virtual void OnUserLeave(GuacUser& user) = 0;

	/**
	 * Handler for mouse events sent by the Gaucamole web-client.
	 */
	virtual void MouseHandler(GuacUser& user, int x, int y, int button_mask) = 0;

	/**
	 * Handler for key events sent by the Guacamole web-client.
	 */
	virtual void KeyHandler(GuacUser& user, int keysym, int pressed) = 0;

	/**
	 * Handler for Guacamole clipboard events.
	 */
	virtual void ClipboardHandler(GuacUser& user, guac_stream* stream, char* mimetype) = 0;

	/**
	* Called by derived classes when a connection is made to the server.
	*/
	void OnConnect();

	std::string hostname_;
	uint16_t port_;

	VMController& controller_;

	std::chrono::duration<uint16_t, std::milli> frame_duration_;

	/**
	* The current state of the client. The state_mutex_ must be locked
	* before changing the state.
	*/
	ClientState client_state_;
	std::mutex state_mutex_;
	std::condition_variable state_wait_;

	/**
	* The first user within the list of all connected users, or NULL if no
	* users are currently connected.
	*/
	//GuacUser* users_;

	/**
	* Lock which is acquired when the users list is being manipulated.
	*/
	//std::mutex users_lock_;

	UserList& users_;

	DisconnectReason disconnect_reason_;

	/**
	 * Set to true when the client thread should generate a thumbnail.
	 * This member is used for cross-thread notification so it could
	 * be benefit from being atomic.
	 */
	bool update_thumbnail_;

private:
	/**
	 * The number of currently-connected users. This value may include inactive
	 * users if cleanup of those users has not yet finished.
	 */
	int connected_users_;

	/**
	 * Pool of buffer indices. Buffers are simply layers with negative indices.
	 * Note that because guac_pool always gives non-negative indices starting
	 * at 0, the output of this guac_pool will be adjusted.
	*/
	guac_pool* __buffer_pool;

	/**
	* Pool of layer indices. Note that because guac_pool always gives
	* non-negative indices starting at 0, the output of this guac_pool will
	* be adjusted.
	*/
	guac_pool* __layer_pool;

};

