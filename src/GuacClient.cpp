#include "GuacClient.h"
#include "CollabVM.h"
#include "VMControllers/VMController.h"
#include "guacamole/vnc/settings.h"
//#include "guacamole/vnc/user.h"
//#include "guacamole/vnc/vnc.h"
#include "guacamole/client-constants.h"
#include "guacamole/user-constants.h"
#include "guacamole/user-handlers.h"

using std::mutex;
using std::lock_guard;
using std::unique_lock;

static guac_layer __GUAC_DEFAULT_LAYER = { 0 };

guac_layer* GuacClient::GUAC_DEFAULT_LAYER = &__GUAC_DEFAULT_LAYER;

GuacClient::GuacClient(CollabVMServer& server, VMController& controller, UserList& users, const std::string& hostname, uint16_t port, uint16_t frame_duration) :
	controller_(controller),
	users_(users),
	client_state_(ClientState::kStopped),
	broadcast_socket_(server, users),
	hostname_(hostname),
	port_(port),
	frame_duration_(frame_duration),
	last_sent_timestamp(0),
	//users_(NULL),
	connected_users_(0),
	__buffer_pool(NULL),
	__layer_pool(NULL),
	update_thumbnail_(true)
{
	/* Allocate buffer and layer pools */
	__buffer_pool = guac_pool_alloc(GUAC_BUFFER_POOL_INITIAL_SIZE);
	__layer_pool = guac_pool_alloc(GUAC_BUFFER_POOL_INITIAL_SIZE);
}

//void GuacClient::Start()
//{
//	// TODO: If current state is "stopping" wait state to be "stopped"
//	lock_guard<mutex> lock(state_mutex_);
//	if (client_state_ != ClientState::kStopped)
//		return;
//	//InitVNC();
//	client_state_ = ClientState::kIdle;
//}
//
//void GuacClient::Stop()
//{
//	// TODO: Set state to "stopping" first, then wait for thread
//	// to terminate before changing state to "stopped"
//	std::unique_lock<mutex> lock(state_mutex_);
//	if (client_state_ == ClientState::kStopped)
//		return;
//	client_state_ = ClientState::kStopped;
//	lock.unlock();
//	state_wait_.notify_all();
//}

void GuacClient::SetEndpoint(const std::string& hostname, uint16_t port)
{
	hostname_ = hostname;
	port_ = port;
}

void GuacClient::Connect()
{
	std::unique_lock<mutex> lock(state_mutex_);
	if (client_state_ != ClientState::kIdle)
		return;
	client_state_ = ClientState::kConnecting;
	lock.unlock();
	state_wait_.notify_all();
}

void GuacClient::Disconnect()
{
	std::unique_lock<mutex> lock(state_mutex_);
	if (client_state_ != ClientState::kConnecting && client_state_ != ClientState::kConnected)
		return;
	client_state_ = ClientState::kDisconnecting;
	lock.unlock();
	state_wait_.notify_all();
}

void GuacClient::AddUser(GuacUser& user)
{
	user.client_ = this;
	//user.active = true;

	// Call the user join handler if the client is already connected
	lock_guard<mutex> state_lock(state_mutex_);
	if (client_state_ == ClientState::kConnected)
		OnUserJoin(user);
}

void GuacClient::RemoveUser(GuacUser& user)
{
	OnUserLeave(user);
}

void GuacClient::OnConnect()
{
	unique_lock<mutex> lock(state_mutex_);
	if (client_state_ != ClientState::kConnecting)
		return;
	client_state_ = ClientState::kConnected;
	lock.unlock();

	controller_.OnGuacConnect();
}

bool GuacClient::Connected() const
{
	return client_state_ == ClientState::kConnected;
}

guac_layer* GuacClient::AllocLayer()
{
	// TODO: Use new and delete
	/* Init new layer */
	guac_layer* allocd_layer = (guac_layer*)malloc(sizeof(guac_layer));
	allocd_layer->index = guac_pool_next_int(__layer_pool) + 1;

	return allocd_layer;
}

void GuacClient::FreeLayer(guac_layer* layer)
{
	/* Release index to pool */
	guac_pool_free_int(__layer_pool, layer->index);

	/* Free layer */
	free(layer);
}

guac_layer* GuacClient::AllocBuffer()
{
	/* Init new layer */
	guac_layer* allocd_layer = (guac_layer*)malloc(sizeof(guac_layer));
	allocd_layer->index = -guac_pool_next_int(__buffer_pool) - 1;

	return allocd_layer;
}

void GuacClient::FreeBuffer(guac_layer* layer)
{
	/* Release index to pool */
	guac_pool_free_int(__buffer_pool, -layer->index - 1);

	/* Free layer */
	free(layer);
}

static guac_stream* __get_input_stream(GuacUser& user, int stream_index) {

	/* Validate stream index */
	if (stream_index < 0 || stream_index >= GUAC_USER_MAX_STREAMS) {

		guac_stream dummy_stream;
		dummy_stream.index = stream_index;

		guac_protocol_send_ack(user.socket_, &dummy_stream,
			"Invalid stream index", GUAC_PROTOCOL_STATUS_CLIENT_BAD_REQUEST);
		return NULL;
	}

	return &(user.__input_streams[stream_index]);

}

static guac_stream* __get_open_input_stream(GuacUser& user, int stream_index) {

	guac_stream* stream = __get_input_stream(user, stream_index);

	/* Fail if no such stream */
	if (stream == NULL)
		return NULL;

	/* Validate initialization of stream */
	if (stream->index == GUAC_USER_CLOSED_STREAM_INDEX) {

		guac_stream dummy_stream;
		dummy_stream.index = stream_index;

		guac_protocol_send_ack(user.socket_, &dummy_stream,
			"Invalid stream index", GUAC_PROTOCOL_STATUS_CLIENT_BAD_REQUEST);
		return NULL;
	}

	return stream;

}

static guac_stream* __init_input_stream(GuacUser& user, int stream_index) {

	guac_stream* stream = __get_input_stream(user, stream_index);

	/* Fail if no such stream */
	if (stream == NULL)
		return NULL;

	/* Initialize stream */
	stream->index = stream_index;
	stream->data = NULL;
	stream->ack_handler = NULL;
	stream->blob_handler = NULL;
	stream->end_handler = NULL;

	return stream;

}

int64_t __guac_parse_int(const char* str)
{
	int sign = 1;
	int64_t num = 0;

	for (; *str != '\0'; str++)
	{
		if (*str == '-')
			sign = -sign;
		else
			num = num * 10 + (*str - '0');
	}

	return num * sign;
}

void GuacClient::HandleSync(GuacUser& user, std::vector<char*>& args)
{
	if (args.size() != 1)
		return;

	int frame_duration;

	guac_timestamp current = guac_timestamp_current();
	guac_timestamp timestamp = __guac_parse_int(args[0]);

	/* Error if timestamp is in future */
	if (timestamp > user.client_->last_sent_timestamp)
		return;

	/* Update stored timestamp */
	user.last_received_timestamp = timestamp;

	/* Calculate length of frame, including network and processing lag */
	frame_duration = current - timestamp;

	/* Update lag statistics if at least one frame has been rendered */
	if (user.last_frame_duration != 0) {

		/* Approximate processing lag by summing the frame duration deltas */
		int processing_lag = user.processing_lag + frame_duration
			- user.last_frame_duration;

		/* Adjust back to zero if cumulative error leads to a negative value */
		if (processing_lag < 0)
			processing_lag = 0;

		user.processing_lag = processing_lag;

	}

	/* Record duration of frame */
	user.last_frame_duration = frame_duration;

	//if (user.sync_handler)
	//	user.sync_handler(user, timestamp);
}

void GuacClient::HandleMouse(GuacUser& user, std::vector<char*>& args)
{
	if (args.size() == 3)
		MouseHandler(user,
			atoi(args[0]), /* x */
			atoi(args[1]), /* y */
			atoi(args[2])  /* mask */);
}

void GuacClient::HandleKey(GuacUser& user, std::vector<char*>& args)
{
	if (args.size() == 2)
		KeyHandler(user,
			atoi(args[0]), /* keysym */
			atoi(args[1])  /* pressed */);
}

void GuacClient::HandleClipboard(GuacUser& user, std::vector<char*>& args)
{
	if (args.size() != 2)
		return;

	/* Pull corresponding stream */
	int stream_index = atoi(args[0]);
	guac_stream* stream = __init_input_stream(user, stream_index);
	if (stream == NULL)
		return;

	/* If supported, call handler */
	ClipboardHandler(user,
		stream,
		args[1] /* mimetype */);
}

//void GuacClient::HandleFile(GuacUser& user, std::vector<char*>& args)
//{
//}
//
//void GuacClient::HandlePipe(GuacUser& user, std::vector<char*>& args)
//{
//}
//
//void GuacClient::HandleAck(GuacUser& user, std::vector<char*>& args)
//{
//	int result;
//	int stream_index = atoi(args[0]);
//	guac_stream* stream = __get_open_input_stream(user, stream_index);
//
//	/* Fail if no such stream */
//	if (stream == NULL)
//		return;
//
//	/* Call stream handler if defined */
//	if (stream->end_handler)
//		result = stream->end_handler(user, stream);
//
//	/* Fall back to global handler if defined */
//	if (user.end_handler)
//		result = user.end_handler(user, stream);
//
//	/* Mark stream as closed */
//	stream->index = GUAC_USER_CLOSED_STREAM_INDEX;
//}
//
//void GuacClient::HandleBlob(GuacUser& user, std::vector<char*>& args)
//{
//	int stream_index = atoi(args[0]);
//	guac_stream* stream = __get_open_input_stream(user, stream_index);
//
//	/* Fail if no such stream */
//	if (stream == NULL)
//		return;
//
//	/* Call stream handler if defined */
//	if (stream->blob_handler) {
//		int length = guac_protocol_decode_base64(args[1]);
//		stream->blob_handler(user, stream, args[1], length);
//		return;
//	}
//
//	/* Fall back to global handler if defined */
//	if (user.blob_handler) {
//		int length = guac_protocol_decode_base64(args[1]);
//		user.blob_handler(user, stream, args[1], length);
//		return;
//	}
//
//	guac_protocol_send_ack(user.socket_, stream,
//		"File transfer unsupported", GUAC_PROTOCOL_STATUS_UNSUPPORTED);
//}
//
//void GuacClient::HandleEnd(GuacUser& user, std::vector<char*>& args)
//{
//	int result = 0;
//	int stream_index = atoi(args[0]);
//	guac_stream* stream = __get_open_input_stream(user, stream_index);
//
//	/* Fail if no such stream */
//	if (stream == NULL)
//		return;
//
//	/* Call stream handler if defined */
//	if (stream->end_handler)
//		result = stream->end_handler(user, stream);
//
//	/* Fall back to global handler if defined */
//	if (user.end_handler)
//		result = user.end_handler(user, stream);
//
//	/* Mark stream as closed */
//	stream->index = GUAC_USER_CLOSED_STREAM_INDEX;
//}
//
//void GuacClient::HandleSize(GuacUser& user, std::vector<char*>& args)
//{
//}

void GuacClient::HandleDisconnect(GuacUser& user, std::vector<char*>& args)
{
	//user.Stop();
}

void GuacClient::Log(guac_client_log_level level, const char* format, ...)
{

}

GuacClient::~GuacClient()
{
	/* Free layer pools */
	guac_pool_free(__buffer_pool);
	guac_pool_free(__layer_pool);
}
