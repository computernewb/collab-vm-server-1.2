#include "GuacUser.h"
#include "GuacClient.h"
#include "guacamole/user-constants.h"

GuacUser::GuacUser(CollabVMServer* server, std::weak_ptr<websocketmm::websocket_user> handle)
	: socket_(server, handle),
	  client_(nullptr),
	  last_received_timestamp(guac_timestamp_current()),
	  last_frame_duration(0),
	  processing_lag(0) {
	//active(false)
	/* Allocate stream pool */
	__stream_pool = guac_pool_alloc(0);
	/* Initialze streams */
	__input_streams = new guac_stream[GUAC_USER_MAX_STREAMS];
	__output_streams = new guac_stream[GUAC_USER_MAX_STREAMS];

	for(int i = 0; i < GUAC_USER_MAX_STREAMS; i++) {
		__input_streams[i].index = GUAC_USER_CLOSED_STREAM_INDEX;
		__output_streams[i].index = GUAC_USER_CLOSED_STREAM_INDEX;
	}
}

guac_stream* GuacUser::AllocStream() {
	guac_stream* allocd_stream;
	int stream_index;

	/* Refuse to allocate beyond maximum */
	if(__stream_pool->active == GUAC_USER_MAX_STREAMS)
		return NULL;

	/* Allocate stream */
	stream_index = guac_pool_next_int(__stream_pool);

	/* Initialize stream */
	allocd_stream = &(__output_streams[stream_index]);
	allocd_stream->index = stream_index;
	allocd_stream->data = NULL;
	allocd_stream->ack_handler = NULL;
	allocd_stream->blob_handler = NULL;
	allocd_stream->end_handler = NULL;

	return allocd_stream;
}

void GuacUser::FreeStream(guac_stream* stream) {
	/* Release index to pool */
	guac_pool_free_int(__stream_pool, stream->index);

	/* Mark stream as closed */
	stream->index = GUAC_USER_CLOSED_STREAM_INDEX;
}

//void GuacUser::Stop()
//{
//	active = false;
//}

void GuacUser::Abort(guac_protocol_status status, const char* format, ...) {
}

void GuacUser::Log(guac_client_log_level level, const char* format, ...) {
}

GuacUser::~GuacUser() {
	delete __input_streams;
	delete __output_streams;

	guac_pool_free(__stream_pool);
}
