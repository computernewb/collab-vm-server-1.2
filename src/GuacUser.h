#pragma once
//#include "GuacClient.h"
#include "GuacWebSocket.h"
#include "guacamole/timestamp.h"
#include "guacamole/pool.h"
#include "guacamole/pool-types.h"
#include "guacamole/stream.h"
#include "guacamole/client-types.h"

class CollabVMServer;
class GuacClient;
class GuacWebSocket;

struct guac_user_info {
	/**
	 * The number of pixels the remote client requests for the display width.
	 * This need not be honored by a client plugin implementation, but if the
	 * underlying protocol of the client plugin supports dynamic sizing of the
	 * screen, honoring the display size request is recommended.
	 */
	int optimal_width;

	/**
	 * The number of pixels the remote client requests for the display height.
	 * This need not be honored by a client plugin implementation, but if the
	 * underlying protocol of the client plugin supports dynamic sizing of the
	 * screen, honoring the display size request is recommended.
	 */
	int optimal_height;

	/**
	 * NULL-terminated array of client-supported audio mimetypes. If the client
	 * does not support audio at all, this will be NULL.
	 */
	const char** audio_mimetypes;

	/**
	 * NULL-terminated array of client-supported video mimetypes. If the client
	 * does not support video at all, this will be NULL.
	 */
	const char** video_mimetypes;

	/**
	 * The DPI of the physical remote display if configured for the optimal
	 * width/height combination described here. This need not be honored by
	 * a client plugin implementation, but if the underlying protocol of the
	 * client plugin supports dynamic sizing of the screen, honoring the
	 * stated resolution of the display size request is recommended.
	 */
	int optimal_resolution;
};

/**
 * A user that is connected to a Guacamole client and receives
 * messages containing screen data.
 */
class GuacUser {
	friend class GuacClient;
	friend class GuacVNCClient;

   public:
	GuacUser(CollabVMServer* server, std::weak_ptr<websocketmm::websocket_user> handle);

	guac_stream* AllocStream();
	void FreeStream(guac_stream* stream);
	//void Stop();
	void Abort(guac_protocol_status status, const char* format, ...);
	void Log(guac_client_log_level level, const char* format, ...);

	~GuacUser();

	/**
	 * This user's actual socket. Data written to this socket will
	 * be received by this user alone, and data sent by this specific
	 * user will be received by this socket.
	 */
	GuacWebSocket socket_;

	/**
	 * The guac_client to which this user belongs. It can be null
	 * if the user is not currently viewing a VM.
	 */
	GuacClient* client_;

	/**
	 * The time (in milliseconds) of receipt of the last sync message from
	 * the user.
	 */
	guac_timestamp last_received_timestamp;

	/**
	 * The duration of the last frame rendered by the user, in milliseconds.
	 * This duration will include network and processing lag, and thus should
	 * be slightly higher than the true frame duration.
	 */
	int last_frame_duration;

	/**
	 * The overall lag experienced by the user relative to the stream of
	 * frames, roughly excluding network lag.
	 */
	int processing_lag;

	//private:
	/**
	 * The unique identifier allocated for this user, which may be used within
	 * the Guacamole protocol to refer to this user.  This identifier is
	 * guaranteed to be unique from all existing connections and users, and
	 * will not collide with any available protocol names.
	 */
	//char* user_id;

	/**
	 * Non-zero if this user is the owner of the associated connection, zero
	 * otherwise. The owner is the user which created the connection.
	 */
	//int owner;

	/**
	 * True if this user is active (connected), and false otherwise. When
	 * the user is created, this will be set to true. If an event
	 * occurs which requires that the user disconnect, or the user has
	 * disconnected, this will be reset to false.
	 */
	//bool active;

	/**
	 * The previous user in the group of users within the same logical
	 * connection.  This is currently only used internally by guac_client to
	 * track its set of connected users. To iterate connected users, use
	 * guac_client_foreach_user().
	 */
	GuacUser* __prev;

	/**
	 * The next user in the group of users within the same logical connection.
	 * This is currently only used internally by guac_client to track its set
	 * of connected users. To iterate connected users, use
	 * guac_client_foreach_user().
	 */
	GuacUser* __next;

	/**
	 * Information structure containing properties exposed by the remote
	 * user during the initial handshake process.
	 */
	guac_user_info info;

	/**
	 * Pool of stream indices.
	 */
	guac_pool* __stream_pool;

	/**
	 * All available output streams (data going to connected user).
	 */
	guac_stream* __output_streams;

	/**
	 * All available input streams (data coming from connected user).
	 */
	guac_stream* __input_streams;

	/**
	 * Arbitrary user-specific data.
	 */
	void* data;

	/**
	 * Handler for mouse events sent by the Gaucamole web-client.
	 *
	 * The handler takes the integer mouse X and Y coordinates, as well as
	 * a button mask containing the bitwise OR of all button values currently
	 * being pressed. Those values are:
	 *
	 * <table>
	 *     <tr><th>Button</th>          <th>Value</th></tr>
	 *     <tr><td>Left</td>            <td>1</td></tr>
	 *     <tr><td>Middle</td>          <td>2</td></tr>
	 *     <tr><td>Right</td>           <td>4</td></tr>
	 *     <tr><td>Scrollwheel Up</td>  <td>8</td></tr>
	 *     <tr><td>Scrollwheel Down</td><td>16</td></tr>
	 * </table>

	 * Example:
	 * @code
	 *     int mouse_handler(guac_user* user, int x, int y, int button_mask);
	 *
	 *     int guac_user_init(guac_user* user, int argc, char** argv) {
	 *         user->mouse_handler = mouse_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_mouse_handler* mouse_handler;

	/**
	 * Handler for key events sent by the Guacamole web-client.
	 *
	 * The handler takes the integer X11 keysym associated with the key
	 * being pressed/released, and an integer representing whether the key
	 * is being pressed (1) or released (0).
	 *
	 * Example:
	 * @code
	 *     int key_handler(guac_user* user, int keysym, int pressed);
	 *
	 *     int guac_user_init(guac_user* user, int argc, char** argv) {
	 *         user->key_handler = key_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_key_handler* key_handler;

	/**
	 * Handler for clipboard events sent by the Guacamole web-client. This
	 * handler will be called whenever the web-client sets the data of the
	 * clipboard.
	 *
	 * The handler takes a guac_stream, which contains the stream index and
	 * will persist through the duration of the transfer, and the mimetype
	 * of the data being transferred.
	 *
	 * Example:
	 * @code
	 *     int clipboard_handler(guac_user* user, guac_stream* stream,
	 *             char* mimetype);
	 *
	 *     int guac_user_init(guac_user* user, int argc, char** argv) {
	 *         user->clipboard_handler = clipboard_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_clipboard_handler* clipboard_handler;

	/**
	 * Handler for size events sent by the Guacamole web-client.
	 *
	 * The handler takes an integer width and integer height, representing
	 * the current visible screen area of the client.
	 *
	 * Example:
	 * @code
	 *     int size_handler(guac_user* user, int width, int height);
	 *
	 *     int guac_user_init(guac_user* user, int argc, char** argv) {
	 *         user->size_handler = size_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_size_handler* size_handler;

	/**
	 * Handler for file events sent by the Guacamole web-client.
	 *
	 * The handler takes a guac_stream which contains the stream index and
	 * will persist through the duration of the transfer, the mimetype of
	 * the file being transferred, and the filename.
	 *
	 * Example:
	 * @code
	 *     int file_handler(guac_user* user, guac_stream* stream,
	 *             char* mimetype, char* filename);
	 *
	 *     int guac_user_init(guac_user* user, int argc, char** argv) {
	 *         user->file_handler = file_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_file_handler* file_handler;

	/**
	 * Handler for pipe events sent by the Guacamole web-client.
	 *
	 * The handler takes a guac_stream which contains the stream index and
	 * will persist through the duration of the transfer, the mimetype of
	 * the data being transferred, and the pipe name.
	 *
	 * Example:
	 * @code
	 *     int pipe_handler(guac_user* user, guac_stream* stream,
	 *             char* mimetype, char* name);
	 *
	 *     int guac_user_init(guac_user* user, int argc, char** argv) {
	 *         user->pipe_handler = pipe_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_pipe_handler* pipe_handler;

	/**
	 * Handler for ack events sent by the Guacamole web-client.
	 *
	 * The handler takes a guac_stream which contains the stream index and
	 * will persist through the duration of the transfer, a string containing
	 * the error or status message, and a status code.
	 *
	 * Example:
	 * @code
	 *     int ack_handler(guac_user* user, guac_stream* stream,
	 *             char* error, guac_protocol_status status);
	 *
	 *     int guac_user_init(guac_user* user, int argc, char** argv) {
	 *         user->ack_handler = ack_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_ack_handler* ack_handler;

	/**
	 * Handler for blob events sent by the Guacamole web-client.
	 *
	 * The handler takes a guac_stream which contains the stream index and
	 * will persist through the duration of the transfer, an arbitrary buffer
	 * containing the blob, and the length of the blob.
	 *
	 * Example:
	 * @code
	 *     int blob_handler(guac_user* user, guac_stream* stream,
	 *             void* data, int length);
	 *
	 *     int guac_user_init(guac_user* user, int argc, char** argv) {
	 *         user->blob_handler = blob_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_blob_handler* blob_handler;

	/**
	 * Handler for stream end events sent by the Guacamole web-client.
	 *
	 * The handler takes only a guac_stream which contains the stream index.
	 * This guac_stream will be disposed of immediately after this event is
	 * finished.
	 *
	 * Example:
	 * @code
	 *     int end_handler(guac_user* user, guac_stream* stream);
	 *
	 *     int guac_user_init(guac_user* user, int argc, char** argv) {
	 *         user->end_handler = end_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_end_handler* end_handler;

	/**
	 * Handler for sync events sent by the Guacamole web-client. Sync events
	 * are used to track per-user latency.
	 *
	 * The handler takes only a guac_timestamp which contains the timestamp
	 * received from the user. Latency can be determined by comparing this
	 * timestamp against the last_sent_timestamp of guac_client.
	 *
	 * Example:
	 * @code
	 *     int sync_handler(guac_user* user, guac_timestamp timestamp);
	 *
	 *     int guac_user_init(guac_user* user, int argc, char** argv) {
	 *         user->sync_handler = sync_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_sync_handler* sync_handler;

	/**
	 * Handler for leave events fired by the guac_client when a guac_user
	 * is leaving an active connection.
	 *
	 * The handler takes only a guac_user which will be the guac_user that
	 * left the connection. This guac_user will be disposed of immediately
	 * after this event is finished.
	 *
	 * Example:
	 * @code
	 *     int leave_handler(guac_user* user);
	 *
	 *     int my_join_handler(guac_user* user, int argv, char** argv) {
	 *         user->leave_handler = leave_handler;
	 *     }
	 * @endcode
	 */
	//guac_user_leave_handler* leave_handler;
};
