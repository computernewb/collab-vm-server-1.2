#include "GuacVNCClient.h"
#include "VMControllers/VMController.h"
#include "CollabVM.h"
#include "guacamole/protocol.h"
#include <cairo/cairo.h>

#ifdef _WIN32
#define strdup _strdup
#endif

using std::unique_lock;
using std::lock_guard;
using std::mutex;
using std::chrono::steady_clock;
using std::chrono::duration;
using std::chrono::milliseconds;
typedef std::chrono::time_point<steady_clock, milliseconds> time_point;

GuacVNCClient::GuacVNCClient(CollabVMServer& server, VMController& controller, UserList& users, const std::string& hostname,
	uint16_t port/*, uint16_t frame_duration*/) :
	GuacClient(server, controller, users, hostname, port, /*frame_duration*/200),
	server_(server),
	rfb_client_(NULL),
	rfb_MallocFrameBuffer_(NULL),
	copy_rect_used_(0),
	password_(NULL),
	encodings_(NULL),
	swap_red_blue_(false),
	color_depth_(0),
	read_only_(false),
	remote_cursor_(false),
	audio_enabled_(false),
	cursor_(guac_common_cursor_alloc(*this)),
	default_surface_(NULL)
{
	password_ = strdup(""); // NOTE: freed by libvncclient
}

void GuacVNCClient::Start()
{
	unique_lock<mutex> lock(state_mutex_);
	// Check if the thread is already running and call the event if it is
	if (client_state_ == ClientState::kIdle)
	{
		lock.unlock();
		controller_.OnGuacStarted();
		return;
	}

	if (client_state_ != ClientState::kStopped)
		return;
	// Change to state to starting to prevent more than one thread from
	// being created if this function is called multiple times
	client_state_ = ClientState::kStarting;
	// Start guacamole VNC client thread
	vnc_thread_ = std::thread(std::bind(&GuacVNCClient::VNCThread, this));
}

void GuacVNCClient::Stop()
{
	unique_lock<mutex> lock(state_mutex_);
	if (client_state_ == ClientState::kStopped)
		return;
	client_state_ = ClientState::kStopped;
	lock.unlock();
	state_wait_.notify_all();
	// Wait for VNC thread to stop
	vnc_thread_.detach();
}

void GuacVNCClient::CleanUp()
{
	// Call the leave handler for each user
	// TODO: Is it required to lock the users list?
	users_.ForEachUserLock([this](CollabVMUser& user)
	{
		OnUserLeave(*user.guac_user);

		user.guac_user->client_ = nullptr;
		//user.user->active = false;
	});

	/* Free memory not free'd by libvncclient's rfbClientCleanup() */
	if (rfb_client_->frameBuffer != NULL)
		free(rfb_client_->frameBuffer);
	if (rfb_client_->raw_buffer != NULL)
		free(rfb_client_->raw_buffer);
	if (rfb_client_->rcSource != NULL)
		free(rfb_client_->rcSource);

	/* Free VNC rfbClientData linked list (not free'd by rfbClientCleanup()) */
	while (rfb_client_->clientData != NULL)
	{
		rfbClientData* next = rfb_client_->clientData->next;
		free(rfb_client_->clientData);
		rfb_client_->clientData = next;
	}

	/* Clean up VNC client*/
	rfbClientCleanup(rfb_client_);

	rfb_client_ = NULL;
}

void GuacVNCClient::guac_vnc_update(rfbClient* client, int x, int y, int w, int h)
{
	GuacVNCClient* vnc_client = (GuacVNCClient*)rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);

	int dx, dy;

	/* Cairo image buffer */
	int stride;
	unsigned char* buffer;
	unsigned char* buffer_row_current;
	cairo_surface_t* surface;

	/* VNC framebuffer */
	unsigned int bpp;
	unsigned int fb_stride;
	unsigned char* fb_row_current;

	/* Ignore extra update if already handled by copyrect */
	if (vnc_client->copy_rect_used_) {
		vnc_client->copy_rect_used_ = 0;
		return;
	}

	/* Init Cairo buffer */
	stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, w);
	buffer = (unsigned char*)malloc(h*stride);
	buffer_row_current = buffer;

	bpp = client->format.bitsPerPixel / 8;
	fb_stride = bpp * client->width;
	fb_row_current = client->frameBuffer + (y * fb_stride) + (x * bpp);

	/* Copy image data from VNC client to PNG */
	for (dy = y; dy < y + h; dy++) {

		unsigned int*  buffer_current;
		unsigned char* fb_current;

		/* Get current buffer row, advance to next */
		buffer_current = (unsigned int*)buffer_row_current;
		buffer_row_current += stride;

		/* Get current framebuffer row, advance to next */
		fb_current = fb_row_current;
		fb_row_current += fb_stride;

		for (dx = x; dx < x + w; dx++) {

			unsigned char red, green, blue;
			unsigned int v;

			switch (bpp)
			{
			case 4:
				v = *((unsigned int*)fb_current);
				break;

			case 2:
				v = *((unsigned short*)fb_current);
				break;

			default:
				v = *((unsigned char*)fb_current);
			}

			/* Translate value to RGB */
			red = (v >> client->format.redShift) * 0x100 / (client->format.redMax + 1);
			green = (v >> client->format.greenShift) * 0x100 / (client->format.greenMax + 1);
			blue = (v >> client->format.blueShift) * 0x100 / (client->format.blueMax + 1);

			/* Output RGB */
			if (vnc_client->swap_red_blue_)
				*(buffer_current++) = (blue << 16) | (green << 8) | red;
			else
				*(buffer_current++) = (red << 16) | (green << 8) | blue;

			fb_current += bpp;

		}
	}

	/* For now, only use default layer */
	surface = cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_RGB24, w, h, stride);

	guac_common_surface_draw(vnc_client->default_surface_, x, y, surface);

	/* Free surface */
	cairo_surface_destroy(surface);
	free(buffer);

}

void GuacVNCClient::guac_vnc_copyrect(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y)
{
	GuacVNCClient* vnc_client = (GuacVNCClient*)rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);

	/* For now, only use default layer */
	guac_common_surface_copy(vnc_client->default_surface_, src_x, src_y, w, h,
		vnc_client->default_surface_, dest_x, dest_y);

	vnc_client->copy_rect_used_ = 1;
}

void GuacVNCClient::guac_vnc_set_pixel_format(rfbClient* client, int color_depth)
{
	switch (color_depth)
	{
	case 8:
		client->format.depth = 8;
		client->format.bitsPerPixel = 8;
		client->format.blueShift = 6;
		client->format.redShift = 0;
		client->format.greenShift = 3;
		client->format.blueMax = 3;
		client->format.redMax = 7;
		client->format.greenMax = 7;
		break;

	case 16:
		client->format.depth = 16;
		client->format.bitsPerPixel = 16;
		client->format.blueShift = 0;
		client->format.redShift = 11;
		client->format.greenShift = 5;
		client->format.blueMax = 0x1f;
		client->format.redMax = 0x1f;
		client->format.greenMax = 0x3f;
		break;

	case 24:
	case 32:
	default:
		client->format.depth = 24;
		client->format.bitsPerPixel = 32;
		client->format.blueShift = 0;
		client->format.redShift = 16;
		client->format.greenShift = 8;
		client->format.blueMax = 0xff;
		client->format.redMax = 0xff;
		client->format.greenMax = 0xff;
	}
}

rfbBool GuacVNCClient::guac_vnc_malloc_framebuffer(rfbClient* rfb_client)
{
	GuacVNCClient* client = (GuacVNCClient*)rfbClientGetClientData(rfb_client, GUAC_VNC_CLIENT_KEY);

	/* Resize surface */
	if (client->default_surface_ != NULL)
		guac_common_surface_resize(client->default_surface_, rfb_client->width, rfb_client->height);

	/* Use original, wrapped proc */
	return client->rfb_MallocFrameBuffer_(rfb_client);
}

char* GuacVNCClient::guac_vnc_get_password(rfbClient* rfb_client)
{
	GuacVNCClient* client = (GuacVNCClient*)rfbClientGetClientData(rfb_client, GUAC_VNC_CLIENT_KEY);
	return client->password_;
}

void GuacVNCClient::guac_vnc_cursor(rfbClient* client, int x, int y, int w, int h, int bpp)
{
	GuacVNCClient* vnc_client = (GuacVNCClient*)rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);

	/* Cairo image buffer */
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
	unsigned char* buffer = (unsigned char*)malloc(h*stride);
	unsigned char* buffer_row_current = buffer;

	/* VNC image buffer */
	unsigned int fb_stride = bpp * w;
	unsigned char* fb_row_current = client->rcSource;
	unsigned char* fb_mask = client->rcMask;

	int dx, dy;

	/* Copy image data from VNC client to RGBA buffer */
	for (dy = 0; dy < h; dy++) {

		unsigned int*  buffer_current;
		unsigned char* fb_current;

		/* Get current buffer row, advance to next */
		buffer_current = (unsigned int*)buffer_row_current;
		buffer_row_current += stride;

		/* Get current framebuffer row, advance to next */
		fb_current = fb_row_current;
		fb_row_current += fb_stride;

		for (dx = 0; dx < w; dx++) {

			unsigned char alpha, red, green, blue;
			unsigned int v;

			/* Read current pixel value */
			switch (bpp) {
			case 4:
				v = *((unsigned int*)fb_current);
				break;

			case 2:
				v = *((unsigned short*)fb_current);
				break;

			default:
				v = *((unsigned char*)fb_current);
			}

			/* Translate mask to alpha */
			if (*(fb_mask++)) alpha = 0xFF;
			else              alpha = 0x00;

			/* Translate value to RGB */
			red = (v >> client->format.redShift) * 0x100 / (client->format.redMax + 1);
			green = (v >> client->format.greenShift) * 0x100 / (client->format.greenMax + 1);
			blue = (v >> client->format.blueShift) * 0x100 / (client->format.blueMax + 1);

			/* Output ARGB */
			if (vnc_client->swap_red_blue_)
				*(buffer_current++) = (alpha << 24) | (blue << 16) | (green << 8) | red;
			else
				*(buffer_current++) = (alpha << 24) | (red << 16) | (green << 8) | blue;

			/* Next VNC pixel */
			fb_current += bpp;

		}
	}

	/* Update stored cursor information */
	guac_common_cursor_set_argb(vnc_client->cursor_, x, y, buffer, w, h, stride);

	/* Free surface */
	free(buffer);

	/* libvncclient does not free rcMask as it does rcSource */
	free(client->rcMask);
}

int GuacVNCClient::EndFrame()
{
	/* Update and send timestamp */
	last_sent_timestamp = guac_timestamp_current();
	return guac_protocol_send_sync(broadcast_socket_, last_sent_timestamp);
}

int GuacVNCClient::GetProcessingLag()
{
	int processing_lag = 0;

	/* Simply find maximum */
	users_.ForEachUserLock([&processing_lag](CollabVMUser& user)
	{
		if (user.guac_user->processing_lag > processing_lag)
			processing_lag = user.guac_user->processing_lag;
	});

	return processing_lag;
}

char* GuacVNCClient::GUAC_VNC_CLIENT_KEY = "GUAC_VNC";

rfbClient* GuacVNCClient::GetVNCClient()
{
	rfbClient* rfb_client = rfbGetClient(8, 3, 4); /* 32-bpp client */

	/* Store Guac client in rfb client */
	rfbClientSetClientData(rfb_client, GUAC_VNC_CLIENT_KEY, this);

	/* Framebuffer update handler */
	rfb_client->GotFrameBufferUpdate = guac_vnc_update;
	rfb_client->GotCopyRect = guac_vnc_copyrect;

	/* Do not handle clipboard and local cursor if read-only */
	if (read_only_ == 0)
	{
		/* Clipboard */
		//rfb_client->GotXCutText = guac_vnc_cut_text;

		/* Set remote cursor */
		if (remote_cursor_) {
			rfb_client->appData.useRemoteCursor = false;
		}

		else {
			/* Enable client-side cursor */
			rfb_client->appData.useRemoteCursor = true;
			rfb_client->GotCursorShape = guac_vnc_cursor;
		}

	}

	/* Password */
	rfb_client->GetPassword = guac_vnc_get_password;

	/* Depth */
	guac_vnc_set_pixel_format(rfb_client, color_depth_);

	/* Hook into allocation so we can handle resize. */
	rfb_MallocFrameBuffer_ = rfb_client->MallocFrameBuffer;
	rfb_client->MallocFrameBuffer = guac_vnc_malloc_framebuffer;
	rfb_client->canHandleNewFBSize = 1;

	/* Set hostname and port */
	rfb_client->serverHost = strdup(hostname_.c_str());
	rfb_client->serverPort = port_;

#ifdef ENABLE_VNC_REPEATER
	/* Set repeater parameters if specified */
	if (vnc_settings->dest_host)
	{
		rfb_client->destHost = strdup(vnc_settings->dest_host);
		rfb_client->destPort = vnc_settings->dest_port;
	}
#endif

#ifdef ENABLE_VNC_LISTEN
	/* If reverse connection enabled, start listening */
	if (vnc_settings->reverse_connect)
	{
		guac_client_log(client, GUAC_LOG_INFO, "Listening for connections on port %i", vnc_settings->port);

		/* Listen for connection from server */
		rfb_client->listenPort = vnc_settings->port;
		if (listenForIncomingConnectionsNoFork(rfb_client, vnc_settings->listen_timeout * 1000) <= 0)
			return NULL;

	}
#endif

	/* Set encodings if provided */
	if (encodings_)
		rfb_client->appData.encodingsString = strdup(encodings_);

	/* Connect */
	if (rfbInitClient(rfb_client, NULL, NULL))
		return rfb_client;

	/* If connection fails, return NULL */
	return NULL;

}

/**
 * Sleeps for the given number of milliseconds.
 *
 * @param msec
 *     The number of milliseconds to sleep;
 */
static void guac_vnc_msleep(int msec)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

void GuacVNCClient::VNCThread()
{
	// If the mutex is locked by the state_mutex_ object then it means
	// that we do not want to connect to the VNC server yet
	unique_lock<mutex> lock(state_mutex_);
	client_state_ = ClientState::kIdle;
	lock.unlock();
	controller_.OnGuacStarted();
	lock.lock();

	while (client_state_ != ClientState::kStopped)
	{
		// Wait for the state to be set to connecting
		while (client_state_ != ClientState::kConnecting)
		{
			state_wait_.wait(lock);

			if (client_state_ == ClientState::kStopped)
				return;
		}
		lock.unlock();

		rfbClient* rfb_client = GetVNCClient();

		/* If the connect attempt fails, try again */
		if (!rfb_client)
		{
			lock.lock();
			//if (client_state_ == ClientState::kConnected)
				//client_state_ = ClientState::kDisconnecting;
			client_state_ = ClientState::kIdle;
			lock.unlock();

			disconnect_reason_ = DisconnectReason::kFailed;
			controller_.OnGuacDisconnect(false);

			//guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Unable to connect to VNC server.");
			//std::cout << "Could not to connect to VNC server. Retrying in one second..." << std::endl;
			//std::this_thread::sleep_for(std::chrono::seconds(1));
			lock.lock();
			continue;
		}

#ifdef ENABLE_PULSE
		/* If an encoding is available, load an audio stream */
		if (guac_client_data->audio_enabled)
		{
			guac_client_data->audio = guac_audio_stream_alloc(client, NULL);

			/* Load servername if specified */
			if (argv[IDX_AUDIO_SERVERNAME][0] != '\0')
				guac_client_data->pa_servername =
				strdup(argv[IDX_AUDIO_SERVERNAME]);
			else
				guac_client_data->pa_servername = NULL;

			/* If successful, init audio system */
			if (guac_client_data->audio != NULL)
			{
				guac_client_log(client, GUAC_LOG_INFO,
					"Audio will be encoded as %s",
					guac_client_data->audio->encoder->mimetype);

				/* Require threadsafe sockets if audio enabled */
				guac_socket_require_threadsafe(broadcast_socket_);

				/* Start audio stream */
				guac_pa_start_stream(client);

			}

			/* Otherwise, audio loading failed */
			else
				guac_client_log(client, GUAC_LOG_INFO,
					"No available audio encoding. Sound disabled.");

		} /* end if audio enabled */
#endif

		  /* Set remaining client data */
		rfb_client_ = rfb_client;

		/* If not read-only, set an appropriate cursor */
		if (read_only_ == 0)
		{
			if (remote_cursor_)
				guac_common_cursor_set_dot(cursor_);
			else
				guac_common_cursor_set_pointer(cursor_);
		}

		/* Send name */
		guac_protocol_send_name(broadcast_socket_, rfb_client->desktopName);

		/* Create default surface */
		default_surface_ = guac_common_surface_alloc(broadcast_socket_, GuacClient::GUAC_DEFAULT_LAYER,
			rfb_client->width, rfb_client->height);

		broadcast_socket_.Flush();

		// Call join handler for each user if there are already
		// users in the list
		users_.ForEachUserLock([this](CollabVMUser& user)
		{
			user.guac_user->client_ = this;
			OnUserJoin(*user.guac_user);
		});

		// Callback on connected
		OnConnect();

		disconnect_reason_ = DisconnectReason::kClient;
		//guac_timestamp last_frame_end = guac_timestamp_current();

		/* Handle messages from VNC server while client is running */
		while (client_state_ == ClientState::kConnected)
		{
			// Wait a maximum of one second for an RFB message to be
			// received from the VNC server
			int wait_result = WaitForMessage(rfb_client, 1000000);
			if (wait_result > 0)
			{
				//guac_timestamp frame_start = guac_timestamp_current();

				///* Calculate time since last frame */
				//int time_elapsed = frame_start - last_frame_end;
				//int processing_lag = GetProcessingLag();

				///* Force roughly-equal length of server and client frames */
				//if (time_elapsed < processing_lag)
				//	guac_vnc_msleep(processing_lag - time_elapsed);

				/* Read server messages until frame is built */
				time_point frame_start = std::chrono::time_point_cast<milliseconds>(steady_clock::now());
				do
				{

					/* Handle any message received */
					if (!HandleRFBServerMessage(rfb_client)) {
						disconnect_reason_ = DisconnectReason::kProtocolError;
						//guac_client_abort(client,
						//        GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
						//        "Error handling message from VNC server.");
						lock.lock();
						if (client_state_ == ClientState::kConnected)
							client_state_ = ClientState::kDisconnecting;
						lock.unlock();
						break;
					}

					/* Calculate time remaining in frame */
					time_point frame_end = std::chrono::time_point_cast<milliseconds>(steady_clock::now());
					milliseconds frame_remaining = frame_start + frame_duration_ - frame_end;

					/* Wait again if frame remaining */
					if (frame_remaining.count() > 0)
						wait_result = WaitForMessage(rfb_client, GUAC_VNC_FRAME_TIMEOUT * 1000);
					else
						break;

				} while (wait_result > 0);

				/* Record end of frame */
				//last_frame_end = guac_timestamp_current();
			}

			/* If an error occurs, log it and fail */
			if (wait_result < 0)
			{
				disconnect_reason_ = DisconnectReason::kServer;
				//guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Connection closed.");
				lock.lock();
				if (client_state_ == ClientState::kConnected)
					client_state_ = ClientState::kDisconnecting;
				lock.unlock();
			}

			// If there were any updates to the surface, flush them to the clients
			// and send a sync message to them
			if (default_surface_->dirty || default_surface_->png_queue_length)
			{
				guac_common_surface_flush(default_surface_);
				EndFrame();
				broadcast_socket_.Flush();
			}

			if (update_thumbnail_)
			{
				GenerateThumbnail();
				update_thumbnail_ = false;
			}
		}

		//guac_client_log(client, GUAC_LOG_INFO, "Internal VNC client disconnected");
		std::cout << "Disconnected from VNC server" << std::endl;

		// Call the disconnect handler so CleanUp() will be called
		controller_.OnGuacDisconnect(true);
		lock.lock();

		// Reset the state to idle
		if (client_state_ != ClientState::kStopped)
			client_state_ = ClientState::kIdle;
	}

	controller_.OnGuacStopped();
}

void GuacVNCClient::OnUserJoin(GuacUser& user)
{
	/* If not owner, synchronize with current display */
	guac_common_surface_dup(default_surface_, user.socket_);
	guac_common_cursor_dup(cursor_, user.socket_);

	user.socket_.Flush();

	guac_protocol_send_sync(user.socket_, guac_timestamp_current());
}

void GuacVNCClient::OnUserLeave(GuacUser& user)
{
	guac_common_cursor_remove_user(cursor_, user);
}

void GuacVNCClient::MouseHandler(GuacUser& user, int x, int y, int button_mask)
{
	/* Store current mouse location */
	guac_common_cursor_move(cursor_, user, x, y);

	SendPointerEvent(rfb_client_, x, y, button_mask);
}

void GuacVNCClient::KeyHandler(GuacUser& user, int keysym, int pressed)
{
	SendKeyEvent(rfb_client_, keysym, pressed);
}

void GuacVNCClient::ClipboardHandler(GuacUser& user, guac_stream* stream, char* mimetype)
{
	guac_protocol_send_ack(user.socket_, stream,
		"Clipboard unsupported", GUAC_PROTOCOL_STATUS_UNSUPPORTED);
}

static cairo_status_t WriteThumbnail(void* closure, const unsigned char* data, unsigned int length)
{
	Base64* base64 = static_cast<Base64*>(closure);
	base64->WriteBase64(data, length);
	return CAIRO_STATUS_SUCCESS;
}

void GuacVNCClient::GenerateThumbnail()
{
	guac_common_surface* surface = default_surface_;

	int width;
	int height;
	float scale_xy;
	if (surface->width > surface->height)
	{
		width = 400;
		scale_xy = 400.0 / surface->width;
		height = scale_xy * surface->height;
	}
	else
	{
		height = 400;
		scale_xy = 400.0 / surface->height;
		width = scale_xy * surface->width;
	}

	cairo_surface_t* target = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);

	cairo_surface_t* rect = cairo_image_surface_create_for_data(
		surface->buffer, CAIRO_FORMAT_RGB24,
		surface->width, surface->height, surface->stride);

	cairo_t* cr = cairo_create(target);
	//cairo_translate(cr, 0, 0);
	cairo_scale(cr, scale_xy, scale_xy);

	cairo_set_source_surface(cr, rect, 0, 0);
	cairo_paint(cr);

	std::ostringstream ss;
	Base64 base64(ss);

	cairo_surface_write_to_png_stream(target, WriteThumbnail, &base64);

	cairo_destroy(cr);
	cairo_surface_destroy(rect);
	cairo_surface_destroy(target);

	controller_.NewThumbnail(new std::string(ss.str()));
}

GuacVNCClient::~GuacVNCClient()
{
}