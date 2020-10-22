/*
Collab VM
Created By:
Cosmic Sans
Dartz
Geodude

Special Thanks:

CHOCOLATEMAN
Colonel Seizureton/Colonel Munchkin
CtrlAltDel
FluffyVulpix
modeco80
hannah
LoveEevee
Matthew
Vanilla
and the rest of the Collab VM Community for all their help over the years,
including, but of course not limited to:
Donating, using the site, telling their friends/family, being a great help, and more.

A special shoutout to the Collab VM community for being such a great help.
You've made the last 5 years great.
Here is the team's special thanks to you - the Collab VM Server Source Code.
We really hope you enjoy and we hope you continue using the website. Thank you all.
The Webframe source code is here: https://github.com/computernewb/collab-vm-web-app

Please email rightowner@gmail.com for any assistance.

~Cosmic Sans, Dartz, and Geodude
*/
#include "CollabVM.h"
#include "GuacInstructionParser.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#ifdef _WIN32
#include <sys/types.h>
#endif
#include <sys/stat.h>

#include "guacamole/user-handlers.h"
#include "guacamole/unicode.h"
#include <ossp/uuid.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>

#define STR_LEN(str) sizeof(str)-1

using std::string;
using std::ostringstream;
using std::chrono::steady_clock;
using std::shared_ptr;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

using rapidjson::Document;
using rapidjson::Value;
using rapidjson::GenericMemberIterator;

const uint8_t CollabVMServer::kIPDataTimerInterval = 1;

enum class ClientFileOp : char
{
	kBegin = '0',
	kMiddle = '1',
	kEnd = '2',
	kStop = '3'
};

enum class ServerFileOp : char
{
	kBegin = '0',
	kAck = '1',
	kFinished = '2',
	kStop = '3',
	kWaitTime = '4',
	kFailed = '5',
	kUploadInProgress = '6',
	kTimedOut = '7'
};

/**
 * The functions that can be performed from the admin panel.
 */
enum CONFIG_FUNCTIONS
{
	kSettings,		// Update server settings
	kPassword,		// Change master password
	kAddVM,			// Add a new VM
	kUpdateVM,		// Update a VM's settings
	kDeleteVM		// Delete a VM
};

/**
 * The names of functions that can be performed from the admin panel.
 */
static const std::string config_functions_[] = {
	"settings",
	"password",
	"add-vm",
	"update-vm",
	"del-vm"
};

enum admin_opcodes_ {
	kStop,	// Stop an admin connection
	kSeshID,		// Authenticate with a session ID
	kMasterPwd,		// Authenticate with the master password
	kGetSettings,	// Get current server settings
	kSetSettings,	// Update server settings
	kQEMU,			// Execute QEMU monitor command
	kStartController,		// Start one or more VM controllers
	kStopController,		// Stop one or more VM controllers
	kRestoreVM,		// Restore one or more VMs
	kRebootVM,		// Reboot one or more VMs
	kResetVM,		// Reset one or more VMs
	kRestartVM		// Restart one or more VM hypervisors
};

enum SERVER_SETTINGS
{
	kChatRateCount,
	kChatRateTime,
	kChatMuteTime,
	kMaxConnections,
	kMaxUploadTime
};

static const std::string server_settings_[] = {
	"chat-rate-count",
	"chat-rate-time",
	"chat-mute-time",
	"max-cons",
	"max-upload-time"
};

enum VM_SETTINGS
{
	kName,
	kAutoStart,
	kDisplayName,
	kHypervisor,
	kStatus,
	kRestoreShutdown,
	kRestoreTimeout,
	kRestorePeriodic,
	kPeriodRestoreTime,
	kVNCAddress,
	kVNCPort,
	kQMPSocketType,
	kQMPAddress,
	kQMPPort,
	kQEMUCmd,
	kQEMUSnapshotMode,
	kTurnsEnabled,
	kTurnTime,
	kVotesEnabled,
	kVoteTime,
	kVoteCooldownTime,
	kAgentEnabled,
	kAgentSocketType,
	kAgentUseVirtio,
	kAgentAddress,
	kAgentPort,
	kRestoreHeartbeat,
	kHeartbeatTimeout,
	kUploadsEnabled,
	kUploadCooldownTime,
	kUploadMaxSize,
	kUploadMaxFilename
};

static const std::string vm_settings_[] = {
	"name",
	"auto-start",
	"display-name",
	"hypervisor",
	"status",
	"restore-shutdown",
	"restore-timeout",
	"restore-periodic",
	"restore-periodic-time",
	"vnc-address",
	"vnc-port",
	"qmp-socket-type",
	"qmp-address",
	"qmp-port",
	"qemu-cmd",
	"qemu-snapshot-mode",
	"turns-enabled",
	"turn-time",
	"votes-enabled",
	"vote-time",
	"vote-cooldown-time",
	"agent-enabled",
	"agent-socket-type",
	"agent-use-virtio",
	"agent-address",
	"agent-port",
	"restore-heartbeat",
	"heartbeat-timeout",
	"uploads-enabled",
	"upload-cooldown-time",
	"upload-max-size",
	"upload-max-filename"
};

static const std::string hypervisor_names_[] {
	"qemu",
	"virtualbox",
	"vmware"
};

static const std::string snapshot_modes_[]{
	"off",
	"vm",
	"hd"
};


// Whole-file TODO:
// - Remove shared_ptr<T>& (const shared_ptr<>T&/move refs are OK)
// - try to fix memleaks

void IgnorePipe();

CollabVMServer::CollabVMServer(boost::asio::io_service& service) :
	service_(service),
	stopping_(false),
	process_thread_running_(false),
	keep_alive_timer_(service),
	vm_preview_timer_(service),
	ip_data_timer(service),
	ip_data_timer_running_(false),
	guest_rng_(1000, 99999),
	rng_(steady_clock::now().time_since_epoch().count()),
	upload_count_(0)
{
	// Create VMControllers for all VMs that will be auto-started
	for (auto it = database_.VirtualMachines.begin(); it != database_.VirtualMachines.end(); it++)
	{
		std::shared_ptr<VMSettings>& vm = it->second;
		if (vm->AutoStart)
		{
			CreateVMController(vm);
		}
	}

	// set up access channels to only log interesting things
	server_.clear_access_channels(websocketpp::log::alevel::all);
	server_.clear_error_channels(websocketpp::log::elevel::all);

	// Initialize Asio Transport
	try
	{
		server_.init_asio(&service);
	}
	catch (const websocketpp::exception& ex)
	{
		std::cout << "Failed to initialize ASIO" << std::endl;
		throw ex;
	}
}

CollabVMServer::~CollabVMServer()
{
	delete[] chat_history_;
}

std::shared_ptr<VMController> CollabVMServer::CreateVMController(const std::shared_ptr<VMSettings>& vm)
{
	std::shared_ptr<VMController> controller;
	switch (vm->Hypervisor)
	{
	case VMSettings::HypervisorEnum::kQEMU:
	{
		controller = std::dynamic_pointer_cast<VMController>(std::make_shared<QEMUController>(*this, service_, vm));
		break;
	}
	default:
		throw std::runtime_error("Error: unsupported hypervisor in database");
	}
	vm_controllers_[vm->Name] = controller;
	return controller;
}

void CollabVMServer::Run(uint16_t port, string doc_root)
{
	// The path shouldn't end in a slash
	char last = doc_root[doc_root.length() - 1];
	if (last == '/' || last == '\\')
		doc_root = doc_root.substr(0, doc_root.length() - 1);
	doc_root_ = doc_root;

	// Register handler callbacks
	server_.set_validate_handler(bind(&CollabVMServer::OnValidate, shared_from_this(), std::placeholders::_1));
	server_.set_http_handler(bind(&CollabVMServer::OnHttp, shared_from_this(), std::placeholders::_1));
	server_.set_open_handler(bind(&CollabVMServer::OnOpen, shared_from_this(), std::placeholders::_1));
	server_.set_close_handler(bind(&CollabVMServer::OnClose, shared_from_this(), std::placeholders::_1));
	server_.set_message_handler(bind(&CollabVMServer::OnMessageFromWS, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

	server_.set_open_handshake_timeout(0);

	// Start WebSocket server listening on specified port
	websocketpp::lib::error_code ec;
	server_.listen(port, ec);
	if (ec)
	{
		std::cout << "Error listening on port " << port << ": " << ec.message() << std::endl;
		return;
	}

	// Start the server accept loop
	server_.start_accept(ec);
	if (ec)
	{
		std::cout << "start_accept failed: " << ec.message() << std::endl;
		return;
	}

	asio_work_ = std::unique_ptr<boost::asio::io_service::work>(new boost::asio::io_service::work(service_));

	boost::system::error_code asio_ec;
	vm_preview_timer_.expires_from_now(std::chrono::seconds(kVMPreviewInterval), asio_ec);
	vm_preview_timer_.async_wait(std::bind(&CollabVMServer::VMPreviewTimerCallback, shared_from_this(), std::placeholders::_1));

	// Start all of the VMs that should be auto-started
	for (auto it = vm_controllers_.begin(); it != vm_controllers_.end(); it++)
	{
		it->second->Start();
	}

	// Start message processing thread
	process_thread_running_ = true;
	process_thread_ = std::thread(bind(&CollabVMServer::ProcessingThread, shared_from_this()));

	// Start the ASIO io_service run loop
	server_.run();

	// Wait for processing thread to terminate
	process_thread_.join();
}

void CollabVMServer::Send404Page(Server::connection_ptr& con, std::string& path)
{
	ostringstream ss;
	ss << "<!DOCTYPE html><html><head>"
		"<title>Error 404 (Resource not found)</title><body>"
		"<h1>Error 404</h1>"
		"<p>The requested URL " << path << " was not found on this server.</p>"
		"</body></head></html>";

	con->set_body(ss.str());
	con->set_status(websocketpp::http::status_code::not_found);
}

void CollabVMServer::SendHTTPFile(Server::connection_ptr& con, std::string& path, std::string& full_path)
{
	using std::ifstream;
	ifstream file(full_path, ifstream::in | ifstream::binary | ifstream::ate);
	if (file.is_open())
	{
		// Read the entire file into a string
		string resp_body;
		size_t size = file.tellg();
		if (size >= 0)
		{
			if (size)
			{
				resp_body.reserve();
				file.seekg(0, ifstream::beg);
				resp_body.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			}
			con->set_body(resp_body);
			con->set_status(websocketpp::http::status_code::ok);
			return;
		}
	}
	Send404Page(con, path);
}

/**
	* Handle HTTP requests.
	*/
void CollabVMServer::OnHttp(connection_hdl handle)
{
	websocketpp::lib::error_code ec;
	Server::connection_ptr con = server_.get_con_from_hdl(handle, ec);
	if (ec)
		return;

	// Check if the request had a body
	unique_lock<std::mutex> lock(http_connections_lock_);
	auto it = http_connections_.find(handle);
	if (it != http_connections_.end())
	{
		HttpBodyData* body = it->second;
		http_connections_.erase(it);
		lock.unlock();

		body->DoResponse(con);
		return;
	}
	lock.unlock();

	std::cout << "Requested resource: " << con->get_resource() << std::endl;
	if (con->get_request_body().length() > 0)
		std::cout << "Request body: " << con->get_request_body() << std::endl;
	// Get path to resource
	std::string path = con->get_resource();
	// The path should begin with a slash
	if (path[0] != '/')
		path = '/' + path;

#define UPLOAD_PREFIX "/upload"

	if (!path.compare(0, sizeof(UPLOAD_PREFIX)-1, UPLOAD_PREFIX, sizeof(UPLOAD_PREFIX)-1))
	{
		con->append_header("Allow", "POST");
		con->append_header("Access-Control-Allow-Origin", "*");

		const Server::connection_type::request_type& request = con->get_request();
		if (request.get_method() == "OPTIONS")
		{
			const std::string& request_method = con->get_request_header("Access-Control-Request-Method");
			const std::string& request_headers = con->get_request_header("Access-Control-Request-Headers");

			con->append_header("Access-Control-Allow-Methods", request_method.empty() ? "POST" : request_method);
			con->append_header("Access-Control-Allow-Headers",
				request_headers.empty() ? "Content-Type" : request_headers);
			con->set_status(websocketpp::http::status_code::ok);
		}
		else
		{
			Send404Page(con, path);
		}
		return;
	}
	
	// Prevent path traversal attacks
	if (path.find("..") != std::string::npos)
	{
		Send404Page(con, path);
	}

	// If the path ends in a slash, remove it
	bool requested_dir;
	if (path[path.length() - 1] == '/')
	{
		path.erase(path.length() - 1, 1);
		requested_dir = true;
	}
	else
	{
		requested_dir = false;
	}

	std::string full_path = doc_root_;
	full_path += path;

	// Determine whether the path is a directory or a file
	struct stat st_buf;
	if (!stat(full_path.c_str(), &st_buf))
	{
		if (st_buf.st_mode & S_IFDIR)
		{
			// The path is a directory
			if (requested_dir)
			{
				full_path += "/index.html";

				// Everything in the admin directory, besides the index page,
				// requires a session ID
				const char admin_dir[] = "/admin/";
				const char index_page[] = "index.html";
				if (!full_path.compare(0, sizeof(admin_dir) - 1, admin_dir) &&
					full_path.compare(sizeof(admin_dir) - 1, sizeof(index_page) - 1, index_page) &&
					!ValidateSessionId(con->get_request_header("Cookie")))
				{
					// Redirect the user to the index page so they can login
					con->append_header("Location", "/admin/");
					con->set_status(websocketpp::http::status_code::see_other);
				}
				else
				{
					SendHTTPFile(con, path, full_path);
				}
			}
			else
			{
				// Redirect to the user the directory
				path += '/';
				con->append_header("Location", path);
				con->set_status(websocketpp::http::status_code::moved_permanently);
			}
			return;
		}
		else if (st_buf.st_mode & S_IFREG)
		{
			// The path is a file
			if (!requested_dir)
			{
				SendHTTPFile(con, path, full_path);
			}
			else
			{
				// Redirect to the user the file
				con->append_header("Location", path);
				con->set_status(websocketpp::http::status_code::moved_permanently);
			}
			return;
		}
	}
	// Send 404 page if the file or directory wasn't found
	Send404Page(con, path);
}

// Probably unneeded, but kills off a warning
#undef UPLOAD_PREFIX

bool CollabVMServer::FindIPv4Data(const boost::asio::ip::address_v4& addr, IPData*& ip_data)
{
	auto it = ipv4_data_.find(addr.to_ulong());
	if (it == ipv4_data_.end())
		return false;
	ip_data = it->second;
	return true;
}

bool CollabVMServer::FindIPData(const boost::asio::ip::address& addr, IPData*& ip_data)
{
	if (addr.is_v4())
	{
		return FindIPv4Data(addr.to_v4(), ip_data);
	}
	else
	{
		const boost::asio::ip::address_v6& addr_v6 = addr.to_v6();
		if (addr_v6.is_v4_mapped())
		{
			return FindIPv4Data(addr_v6.to_v4(), ip_data);
		}
		else
		{
			typedef boost::asio::detail::array<unsigned char, 16> bytes_type;
			auto it = ipv6_data_.find(addr_v6.to_bytes());
			if (it == ipv6_data_.end())
				return false;
			ip_data = it->second;
		}
	}
	return true;
}

IPData* CollabVMServer::CreateIPv4Data(const boost::asio::ip::address_v4& addr, bool one_connection)
{
	unsigned long ipv4 = addr.to_ulong();
	IPData* ip_data = new IPv4Data(ipv4, one_connection);
	ipv4_data_[ipv4] = ip_data;
	return ip_data;
}

IPData* CollabVMServer::CreateIPData(const boost::asio::ip::address& addr, bool one_connection)
{
	if (addr.is_v4())
	{
		return CreateIPv4Data(addr.to_v4(), one_connection);
	}
	else
	{
		const boost::asio::ip::address_v6& addr_v6 = addr.to_v6();
		if (addr_v6.is_v4_mapped())
		{
			return CreateIPv4Data(addr_v6.to_v4(), one_connection);
		}
		else
		{
			const std::array<unsigned char, 16>& ipv6 = addr_v6.to_bytes();
			IPData* ip_data = new IPv6Data(ipv6, one_connection);
			ipv6_data_[ipv6] = ip_data;
			return ip_data;
		}
	}
}

void CollabVMServer::DeleteIPData(IPData& ip_data)
{
	if (ip_data.type == IPData::IPType::kIPv4)
		ipv4_data_.erase(static_cast<IPv4Data&>(ip_data).addr);
	else
		ipv6_data_.erase(static_cast<IPv6Data&>(ip_data).addr);

	delete &ip_data;
}

void CollabVMServer::OnHttpPartial(connection_hdl handle, const std::string& res, const char* buf, size_t len)
{
	HttpBodyData* data;
	unique_lock<std::mutex> lock(http_connections_lock_);
	auto it = http_connections_.find(handle);
	if (it == http_connections_.end())
	{
		websocketpp::lib::error_code ec;
		Server::connection_ptr con = server_.get_con_from_hdl(handle, ec);
		if (ec)
			return;
		const string& contentType = con->get_request_header("Content-Type");
		const string& contentLength = con->get_request_header("Content-Length");
		bool error = true;
		const string& cookies = con->get_request_header("Cookie");

// Why does this not work on Windows?
		if (res == "/admin/login")
		{
			if (GetBodyContentType(contentType) == HttpContentType::kUrlEncoded)
			{
				char* end;
				UrlEncodedHttpBody<HttpLogin>* body = new UrlEncodedHttpBody<HttpLogin>(
															std::strtoul(contentLength.c_str(), &end, 10));
				body->Init(shared_from_this());
				data = static_cast<HttpBodyData*>(body);
				http_connections_[handle] = data;
				error = false;
			}
		}
#define UPLOAD_PREFIX "/upload?"
		else if (!res.compare(0, sizeof(UPLOAD_PREFIX) - 1, UPLOAD_PREFIX, sizeof(UPLOAD_PREFIX) - 1) &&
										GetBodyContentType(contentType) == HttpContentType::kOctetStream)
		{
			std::string upload_id = res.substr(sizeof(UPLOAD_PREFIX) - 1);
			unique_lock<std::mutex> lock(upload_lock_);
			auto it = upload_ids_.find(upload_id);
			if (it != upload_ids_.end())
			{
				std::shared_ptr<UploadInfo> upload_info = it->second;
				upload_ids_.erase(it);
				upload_info->upload_it = upload_ids_.end();
				boost::asio::steady_timer* timer = upload_info->timeout_timer;
				UploadInfo::HttpUploadState expected = UploadInfo::HttpUploadState::kNotStarted;
				bool started = upload_info->http_state.compare_exchange_strong(
					expected, UploadInfo::HttpUploadState::kNotWriting);
				lock.unlock();

				if (started)
				{
					if (auto user = upload_info->user.lock())
					{
						char* end;
						unsigned long file_size = std::strtoul(contentLength.c_str(), &end, 10);
						if (file_size == upload_info->file_size)
						{
							OctetStreamHttpBody* body = new OctetStreamHttpBody(shared_from_this(),
								file_size, upload_info);
							data = static_cast<HttpBodyData*>(body);
							http_connections_[handle] = data;
							error = false;

							if (uint16_t timeout = database_.Configuration.MaxUploadTime)
							{
								boost::system::error_code ec;
								timer->expires_from_now(std::chrono::seconds(timeout), ec);
								timer->async_wait(std::bind(&CollabVMServer::OnUploadTimeout, shared_from_this(),
									std::placeholders::_1, upload_info));
							}
						}
					}
				}
			}
		}

		if (error)
		{
			// Throw an exception and close the connection because the
			// upload request is invalid
			throw websocketpp::http::exception("Invalid request body",
				websocketpp::http::status_code::bad_request);
		}
	}
	else
	{
		data = it->second;
	}

	lock.unlock();

	if (len > 0)
		data->Receive(handle, buf, len);
}

CollabVMServer::HttpContentType CollabVMServer::GetBodyContentType(const std::string& contentType)
{
#define MULTIPART_STR "multipart/form-data"
#define URLENCODED_STR "application/x-www-form-urlencoded"
#define BOUNDARY "boundary="
#define OCTET_STREAM_STR "application/octet-stream"

	if (contentType.length() >= sizeof(URLENCODED_STR) - 1 &&
		!strncasecmp(contentType.c_str(), URLENCODED_STR, sizeof(URLENCODED_STR) - 1))
	{
		return HttpContentType::kUrlEncoded;
	}
	if (contentType.length() >= sizeof(OCTET_STREAM_STR) - 1 &&
		!strncasecmp(contentType.c_str(), OCTET_STREAM_STR, sizeof(OCTET_STREAM_STR) - 1))
	{
		return HttpContentType::kOctetStream;
	}
	return HttpContentType::kNone;
}

void CollabVMServer::BodyHeaderCallback(std::map<string, string> disposition)
{
	std::cout << "Header Callback:\n";
	for (std::map<string, string>::iterator it = disposition.begin(); it != disposition.end(); it++)
		std::cout << it->first << " = " << it->second << "\n";
	std::cout << std::endl;
}

void CollabVMServer::BodyDataCallback(const char* buf, size_t len)
{
	std::cout << "Data Callback:\n";
	std::cout.write(buf, len);
	std::cout << std::endl;
}

bool CollabVMServer::ValidateSessionId(const string& cookies)
{
	if (!admin_session_id_.length())
		return false;

	const char seshId[] = "sessionID=";
	size_t seshIdIndex;
	return (seshIdIndex = cookies.find(seshId, 0, sizeof(seshId) - 1)) != string::npos &&
		cookies.length() - seshIdIndex - sizeof(seshId) + 2 >= admin_session_id_.length() && 
		!cookies.compare(seshIdIndex + sizeof(seshId) - 1, admin_session_id_.length(), admin_session_id_);
}

bool CollabVMServer::OnValidate(connection_hdl handle)
{
	websocketpp::lib::error_code ec;
	Server::connection_ptr con = server_.get_con_from_hdl(handle, ec);
	if (ec)
		return false;

	// Only accept protocol named "guacamole"
	const std::vector<string>& protocols = con->get_requested_subprotocols();
	for (auto it = protocols.begin(); it != protocols.end(); it++)
	{
		if (*it == "guacamole")
		{
			con->select_subprotocol(*it);

			// Create new IPData object
			boost::system::error_code ec;
			const boost::asio::ip::tcp::endpoint& ep = con->get_raw_socket().remote_endpoint(ec);
			if (ec)
				return false;

			const boost::asio::ip::address& addr = ep.address();

			unique_lock<std::mutex> ip_lock(ip_lock_);
			IPData* ip_data;
			if (FindIPData(addr, ip_data))
			{
				if (ip_data->connections >= database_.Configuration.MaxConnections)
					return false;
				// Reuse existing IPData
				ip_data->connections++;
			}
			else
			{
				// Create new IPData
				ip_data = CreateIPData(addr, true);
			}
			ip_lock.unlock();

			con->user = std::make_shared<CollabVMUser>(handle, *ip_data);

			return true;
		}
	}
	return false;
}

void CollabVMServer::OnOpen(connection_hdl handle)
{
	websocketpp::lib::error_code ec;
	Server::connection_ptr con = server_.get_con_from_hdl(handle, ec);
	if (ec)
		return;
		
	// Add the CollabVMUser to the connection-data map
	unique_lock<std::mutex> lock(process_queue_lock_);
	// Add the add connection action to the queue
	process_queue_.push(new UserAction(*con->user, ActionType::kAddConnection));
	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::OnClose(connection_hdl handle)
{
	unique_lock<std::mutex> lock(process_queue_lock_);

	websocketpp::lib::error_code ec;
	Server::connection_ptr con = server_.get_con_from_hdl(handle, ec);
	if (ec)
		return;

	// Add the remove connection action to the queue
	process_queue_.push(new UserAction(*con->user, ActionType::kRemoveConnection));
	lock.unlock();
	process_wait_.notify_one();
}

std::string CollabVMServer::GenerateUuid()
{
	char buffer[UUID_LEN_STR + 1];

	uuid_t* uuid;

	/* Attempt to create UUID object */
	if (uuid_create(&uuid) != UUID_RC_OK) {
		throw std::string("Could not allocate memory for UUID");
	}

	/* Generate random UUID */
	if (uuid_make(uuid, UUID_MAKE_V4) != UUID_RC_OK) {
		uuid_destroy(uuid);
		throw std::string("UUID generation failed");
	}

	char* identifier = buffer;
	size_t identifier_length = sizeof(buffer);

	/* Build connection ID from UUID */
	if (uuid_export(uuid, UUID_FMT_STR, &identifier, &identifier_length) != UUID_RC_OK) {
		uuid_destroy(uuid);
		throw std::string("Conversion of UUID to connection ID failed");
	}

	uuid_destroy(uuid);

	return std::string(buffer, identifier_length-1);
}

/**
	* Callback for messages received from websockets.
	*/
void CollabVMServer::OnMessageFromWS(connection_hdl handle, Server::message_ptr msg)
{
	unique_lock<std::mutex> lock(process_queue_lock_);
	// Add the message to the processing queue
	websocketpp::lib::error_code ec;
	Server::connection_ptr con = server_.get_con_from_hdl(handle, ec);
	if (ec)
		return;

	process_queue_.push(new MessageAction(msg, *con->user, ActionType::kMessage));

	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::SendWSMessage(CollabVMUser& user, const std::string& str)
{
	websocketpp::lib::error_code ec;
	server_.send(user.handle, str, websocketpp::frame::opcode::text, ec);
	if (ec)
	{
		server_.close(user.handle, websocketpp::close::status::normal, std::string(), ec);
		if (user.connected)
		{
			// Disconnect the client if an error occurs
			unique_lock<std::mutex> lock(process_queue_lock_);
			process_queue_.push(new UserAction(user, ActionType::kRemoveConnection));
			lock.unlock();
			process_wait_.notify_one();
		}
	}
}

void CollabVMServer::TimerCallback(const boost::system::error_code& ec, ActionType action)
{
	if (ec)
		return;

	unique_lock<std::mutex> lock(process_queue_lock_);
	// Let the processing thread handle the next turn
	process_queue_.push(new Action(action));

	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::VMPreviewTimerCallback(const boost::system::error_code ec)
{
	if (ec)
		return;

	unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new Action(ActionType::kUpdateThumbnails));

	lock.unlock();
	process_wait_.notify_one();

	boost::system::error_code asio_ec;
	vm_preview_timer_.expires_from_now(std::chrono::seconds(kVMPreviewInterval), asio_ec);
	vm_preview_timer_.async_wait(std::bind(&CollabVMServer::VMPreviewTimerCallback, shared_from_this(), std::placeholders::_1));
}

void CollabVMServer::IPDataTimerCallback(const boost::system::error_code& ec)
{
	if (ec)
		return;

	std::lock_guard<std::mutex> lock(ip_lock_);
	// The IP data clean up timer should continue to run if there
	// are IPData objects without any connections
	bool should_run = false;
	IPData* ip_data;
	auto ipv4_it = ipv4_data_.begin();
	while (ipv4_it != ipv4_data_.end())
	{
		ip_data = ipv4_it->second;
		if (!ip_data->connections)
		{
			if (ShouldCleanUpIPData(*ip_data))
			{
				ipv4_it = ipv4_data_.erase(ipv4_it);
				continue;
			}
			else
			{
				should_run = true;
			}
		}
		ipv4_it++;
	}

	auto ipv6_it = ipv6_data_.begin();
	while (ipv6_it != ipv6_data_.end())
	{
		ip_data = ipv6_it->second;
		if (!ip_data->connections)
		{
			if (ShouldCleanUpIPData(*ip_data))
			{
				ipv6_it = ipv6_data_.erase(ipv6_it);
				continue;
			}
			else
			{
				should_run = true;
			}
		}
		ipv6_it++;
	}

	if (ip_data_timer_running_ = should_run)
	{
		boost::system::error_code ec;
		ip_data_timer.expires_from_now(std::chrono::minutes(kIPDataTimerInterval), ec);
		ip_data_timer.async_wait(std::bind(&CollabVMServer::IPDataTimerCallback, shared_from_this(), std::placeholders::_1));
	}
}

bool CollabVMServer::ShouldCleanUpIPData(IPData& ip_data) const
{
	//if (ip_data.connections)
	//	return false;

	auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());

	if ((now - ip_data.last_chat_msg).count() < database_.Configuration.ChatRateTime)
		return false;

	if (ip_data.chat_muted)
	{
		if ((now - ip_data.last_chat_msg).count() >= database_.Configuration.ChatMuteTime)
		{
			ip_data.chat_muted = false;
		}
		else
		{
			return false;
		}
	}
		
	if (ip_data.failed_logins)
	{
		if ((now - ip_data.failed_login_time).count() >= kLoginIPBlockTime)
		{
			// Reset login attempts after block time has elapsed
			ip_data.failed_logins = 0;
		}
		else
		{
			return false;
		}
	}

	for (auto&& vote : ip_data.votes)
		if (vote.second != IPData::VoteDecision::kNotVoted)
			return false;

	if (ip_data.upload_in_progress || (ip_data.next_upload_time - now).count() > 0)
		return false;

	return true;
}

void CollabVMServer::RemoveConnection(std::shared_ptr<CollabVMUser>& user)
{
	//if (!user->connected)
	//	return;

	std::cout << "[WebSocket Disconnect] IP: " << user->ip_data.GetIP();
	if (user->username)
		std::cout << " Username: \"" << *user->username << '"';
	std::cout << std::endl;

	if (user->admin_connected)
	{
		admin_connections_.erase(user);
		user->admin_connected = false;
	}

	// CancelFileUpload should be called before setting vm_controller
	// to a nullptr (which is done by VMController::RemoveUser)
	CancelFileUpload(*user);

	if (user->vm_controller)
		user->vm_controller->RemoveUser(user);

	if (user->guac_user)
	{
		delete user->guac_user;
	}

	if (user->username)
	{
		// Remove the connection data from the map

		// Send a remove user instruction to everyone
		ostringstream ss("7.remuser,1.1,", ostringstream::in | ostringstream::out | ostringstream::ate);
		ss << user->username->length() << '.' << *user->username << ';';
		string instr = ss.str();

		IterateUserList(*user, [&](CollabVMUser& u) {
			SendWSMessage(u, instr);	
		});

		user->username.reset();
	}

	unique_lock<std::mutex> ip_lock(ip_lock_);
	if (!--user->ip_data.connections && ShouldCleanUpIPData(user->ip_data))
	{
		DeleteIPData(user->ip_data);
	}
	else if (!ip_data_timer_running_)
	{
		// Start the IP data clean up timer if it's not already running
		ip_data_timer_running_ = true;
		boost::system::error_code ec;
		ip_data_timer.expires_from_now(std::chrono::minutes(kIPDataTimerInterval), ec);
		ip_data_timer.async_wait(std::bind(&CollabVMServer::IPDataTimerCallback, shared_from_this(), std::placeholders::_1));
	}
	ip_lock.unlock();

	user->connected = false;
}

void CollabVMServer::UpdateVMStatus(const std::string& vm_name, VMController::ControllerState state)
{
	if (admin_connections_.empty())
		return;

	string state_str = std::to_string(static_cast<uint32_t>(state));
	ostringstream ss("5.admin,1.3,", ostringstream::in | ostringstream::out | ostringstream::ate);
	ss << vm_name.length() << '.' << vm_name << ',' << state_str.length() << '.' << state_str << ';';
	string instr = ss.str();
	for (auto it = admin_connections_.begin(); it != admin_connections_.end(); it++)
	{
		assert((*it)->admin_connected);
		SendWSMessage(**it, instr);
	}
}

void CollabVMServer::ProcessingThread()
{
	IgnorePipe();
	while (true)
	{
		std::unique_lock<std::mutex> lock(process_queue_lock_);
		while (process_queue_.empty())
		{
			process_wait_.wait(lock);
		}

		Action* action = process_queue_.front();
		process_queue_.pop();

		lock.unlock();

		switch (action->action)
		{
		case ActionType::kMessage:
		{
			MessageAction* msg_action = static_cast<MessageAction*>(action);
			if (msg_action->user->connected)
			{
				const std::string& instr = msg_action->message->get_payload();
				GuacInstructionParser::ParseInstruction(*this, msg_action->user, instr);
			}
			break;
		}
		case ActionType::kAddConnection:
		{
			const std::shared_ptr<CollabVMUser>& user = static_cast<UserAction*>(action)->user;
			assert(!user->connected);

			connections_.insert(user);

			// Start the keep-alive timer after the first client connects
			if (connections_.size() == 1)
			{
				boost::system::error_code ec;
				keep_alive_timer_.expires_from_now(std::chrono::seconds(kKeepAliveInterval), ec);
				keep_alive_timer_.async_wait(std::bind(&CollabVMServer::TimerCallback, shared_from_this(), ::_1, ActionType::kKeepAlive));
			}

			user->connected = true;

			std::cout << "[WebSocket Connect] IP: " << user->ip_data.GetIP() << std::endl;

			break;
		}
		case ActionType::kRemoveConnection:
		{
			std::shared_ptr<CollabVMUser>& connection = static_cast<UserAction*>(action)->user;
			if (connection->connected)
			{
				connections_.erase(connection);
				RemoveConnection(connection);
			}
			break;
		}
		case ActionType::kTurnChange:
		{
			const std::shared_ptr<VMController>& controller = static_cast<VMAction*>(action)->controller;
			controller->NextTurn();
			break;
		}
		case ActionType::kVoteEnded:
		{
			const std::shared_ptr<VMController>& controller = static_cast<VMAction*>(action)->controller;
			controller->EndVote();
			break;
		}
		case ActionType::kAgentConnect:
		{
			AgentConnectAction* agent_action = static_cast<AgentConnectAction*>(action);
			const std::shared_ptr<VMController>& controller = agent_action->controller;
			controller->agent_connected_ = true;
			controller->agent_os_name_ = agent_action->os_name;
			controller->agent_service_pack_ = agent_action->service_pack;
			controller->agent_pc_name_ = agent_action->pc_name;
			controller->agent_username_ = agent_action->username;
			controller->agent_max_filename_ = std::min(static_cast<uint32_t>(controller->GetSettings().UploadMaxFilename),
														agent_action->max_filename);
			controller->agent_upload_in_progress_ = false;

			std::cout << "Agent Connected, OS: \"" << agent_action->os_name <<
						"\", SP: \"" << agent_action->service_pack <<
						"\", PC: \"" << agent_action->pc_name <<
						"\", Username: \"" << agent_action->username << "\"" << std::endl;

			SendActionInstructions(*controller, controller->GetSettings());
			break;
		}
		case ActionType::kAgentDisconnect:
		{
			const std::shared_ptr<VMController>& controller = static_cast<VMAction*>(action)->controller;
			controller->agent_connected_ = false;
			controller->agent_os_name_.clear();
			controller->agent_service_pack_.clear();
			controller->agent_pc_name_.clear();
			controller->agent_username_.clear();
			controller->agent_upload_in_progress_ = false;

			SendActionInstructions(*controller, controller->GetSettings());
			break;
		}
		case ActionType::kHttpUploadTimedout:
		{
			const std::shared_ptr<UploadInfo>& upload_info = static_cast<HttpAction*>(action)->upload_info;
			if (upload_info->canceled)
				break;
			upload_info->canceled = true;

			unique_lock<std::mutex> lock(upload_lock_);
			if (upload_info->upload_it != upload_ids_.end())
			    upload_ids_.erase(upload_info->upload_it);
			lock.unlock();
			if (auto user = upload_info->user.lock())
			{
				user->upload_info.reset();
				FileUploadEnded(upload_info, &user, FileUploadResult::kHttpUploadTimedOut);
			}
			else
			{
				FileUploadEnded(upload_info, nullptr, FileUploadResult::kHttpUploadTimedOut);
			}
			break;
		}
		case ActionType::kHttpUploadFinished:
		{
			const std::shared_ptr<UploadInfo>& upload_info = static_cast<HttpAction*>(action)->upload_info;
			if (upload_info->canceled)
				break;
			upload_info->canceled = true;

			if (boost::asio::steady_timer* timer = upload_info->timeout_timer)
			{
				upload_info->timeout_timer = nullptr;
				boost::system::error_code ec;
				timer->cancel(ec);
				delete timer;
			}

			VMController& vm_controller = *upload_info->vm_controller;
			if (auto user = upload_info->user.lock())
			{
				user->upload_info.reset();

				// Cancel the upload if the user switched to another VM
				if (user->vm_controller == &vm_controller)
				{
					if (vm_controller.agent_upload_in_progress_)
					{
						vm_controller.agent_upload_queue_.push_back(upload_info);
					}
					else
					{
						vm_controller.UploadFile(upload_info);
						vm_controller.agent_upload_in_progress_ = true;
					}
					break;
				}
				FileUploadEnded(upload_info, &user, FileUploadResult::kHttpUploadFailed);
				break;
			}
			FileUploadEnded(upload_info, nullptr, FileUploadResult::kHttpUploadFailed);
			break;
		}
		case ActionType::kHttpUploadFailed:
		{
			const std::shared_ptr<UploadInfo>& upload_info = static_cast<HttpAction*>(action)->upload_info;
			if (upload_info->canceled)
				break;
			upload_info->canceled = true;

			if (auto user = upload_info->user.lock())
			{
				user->upload_info.reset();
				FileUploadEnded(upload_info, &user, FileUploadResult::kHttpUploadFailed);
			}
			else
			{
				FileUploadEnded(upload_info, nullptr, FileUploadResult::kHttpUploadFailed);
			}
			break;
		}
		case ActionType::kUploadEnded:
		{
			FileUploadAction& upload_action = *static_cast<FileUploadAction*>(action);
			FileUploadResult result = upload_action.upload_result == FileUploadAction::UploadResult::kSuccess ||
										upload_action.upload_result == FileUploadAction::UploadResult::kSuccessNoExec ?
										FileUploadResult::kAgentUploadSucceeded : FileUploadResult::kAgentUploadFailed;
			if (auto user = upload_action.upload_info->user.lock())
			{
				user->upload_info.reset();
				FileUploadEnded(upload_action.upload_info, &user, result);
			}
			else
			{
				FileUploadEnded(upload_action.upload_info, nullptr, result);
			}
			break;
		}
		case ActionType::kKeepAlive:
			if (!connections_.empty())
			{
				// Disconnect all clients that haven't responded within the timeout period
				// and broadcast a nop instruction to all clients
				using std::chrono::steady_clock;
				using std::chrono::seconds;
				using std::chrono::time_point;
				time_point<steady_clock, seconds> now(std::chrono::time_point_cast<seconds>(steady_clock::now()));
				for (auto it = connections_.begin(); it != connections_.end();)
				{
					std::shared_ptr<CollabVMUser> user = *it;

					if ((now - user->last_nop_instr).count() > kKeepAliveTimeout)
					{
						// Disconnect the websocket client
						websocketpp::lib::error_code ec;
						server_.close(user->handle, websocketpp::close::status::normal, "", ec);
						it = connections_.erase(it);
						if (user->connected)
							RemoveConnection(user);
					}
					else
					{
						SendWSMessage(*user, "3.nop;");
						it++;
					}
				}
				// Schedule another keep-alive instruction
				if (!connections_.empty())
				{
					boost::system::error_code ec;
					keep_alive_timer_.expires_from_now(std::chrono::seconds(kKeepAliveInterval), ec);
					keep_alive_timer_.async_wait(std::bind(&CollabVMServer::TimerCallback, shared_from_this(), ::_1, ActionType::kKeepAlive));
				}
			}
			break;
		case ActionType::kUpdateThumbnails:
			for (auto it = vm_controllers_.begin(); it != vm_controllers_.end(); it++)
			{
				it->second->UpdateThumbnail();
			}
			break;
		case ActionType::kVMThumbnail:
		{
			VMThumbnailUpdate* thumbnail = static_cast<VMThumbnailUpdate*>(action);
			thumbnail->controller->SetThumbnail(thumbnail->thumbnail);
			break;
		}
		case ActionType::kVMStateChange:
		{
			VMStateChange* state_change = static_cast<VMStateChange*>(action);
			std::shared_ptr<VMController>& controller = state_change->controller;

			if (state_change->state == VMController::ControllerState::kStopped)
			{
				VMController::StopReason reason = controller->GetStopReason();

				if (!stopping_ && reason == VMController::StopReason::kRestart)
				{
					controller->Start();
				}
				else
				{
					if (reason == VMController::StopReason::kError)
					{
						std::cout << "VM \"" << controller->GetSettings().Name << "\" was stopped. " <<
							controller->GetErrorMessage() << std::endl;
					}

					UpdateVMStatus(controller->GetSettings().Name, VMController::ControllerState::kStopped);

					auto vm_it = vm_controllers_.find(controller->GetSettings().Name);
					if (vm_it != vm_controllers_.end())
						vm_controllers_.erase(vm_it);
					controller.reset();

					if (stopping_ && vm_controllers_.empty())
					{
						// Free the io_service's work so it can stop
						asio_work_.reset();
						delete action;
						// Exit the processing loop
						goto stop;
					}
				}
			}
			else
			{
				if (state_change->state == VMController::ControllerState::kStopping)
					controller->ClearTurnQueue();

				UpdateVMStatus(controller->GetSettings().Name, static_cast<VMStateChange*>(action)->state);
			}
			break;
		}
		case ActionType::kVMCleanUp:
		{
			const std::shared_ptr<VMController>& controller = static_cast<VMAction*>(action)->controller;
			controller->CleanUp();
			break;
		}
		case ActionType::kShutdown:
			websocketpp::lib::error_code ec;
			// Disconnect all active clients
			for (auto it = connections_.begin(); it != connections_.end(); it++)
			{
				server_.close((*it)->handle, websocketpp::close::status::normal, "", ec);
			}

			// If there are no VM controllers running, exit the processing thread
			if (vm_controllers_.empty())
			{
				asio_work_.reset();
				delete action;
				// Exit the processing loop
				goto stop;
			}
			std::cout << "Stopping all VM Controllers..." << std::endl;
			// Stop all VM controllers
			for (auto it = vm_controllers_.begin(); it != vm_controllers_.end();)
			{
				const std::shared_ptr<VMController>& controller = it->second;
				if (controller->IsRunning())
				{
					it->second->Stop(VMController::StopReason::kRemove);
					it++;
				}
				else
				{
					it = vm_controllers_.erase(it);
				}
			}

			if (vm_controllers_.empty())
			{
				asio_work_.reset();
				delete action;
				// Exit the processing loop
				goto stop;
			}

			// Wait for all VM controllers to stop
		}

		delete action;
	}
	stop:
	process_thread_running_ = false;
}

void CollabVMServer::OnVMControllerVoteEnded(const std::shared_ptr<VMController>& controller)
{
	std::unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new VMAction(controller, ActionType::kVoteEnded));
	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::OnVMControllerTurnChange(const std::shared_ptr<VMController>& controller)
{
	std::unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new VMAction(controller, ActionType::kTurnChange));
	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::OnVMControllerThumbnailUpdate(const std::shared_ptr<VMController>& controller, std::string* str)
{
	std::unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new VMThumbnailUpdate(controller, str));
	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::BroadcastTurnInfo(VMController& controller, UserList& users, const std::deque<std::shared_ptr<CollabVMUser>>& turn_queue, CollabVMUser* current_turn, uint32_t time_remaining)
{
	if (current_turn == nullptr)
	{
		// The instruction is static if there is nobody controlling the VM
		// and nobody is waiting in the queue
		users.ForEachUser([this](CollabVMUser& user)
		{
			SendWSMessage(user, "4.turn,1.0,1.0;");
		});
	}
	else
	{
		std::string users_list;
		users_list.reserve((turn_queue.size() + 1)*(kMaxUsernameLen + 4));

		// Append the number users waiting in the queue plus one
		// for the user who's controlling it
		std::string temp_str = std::to_string(turn_queue.size() + 1);
		users_list += std::to_string(temp_str.length());
		users_list += '.';
		users_list += temp_str;
		users_list += ',';

		// Append name of the user currently in control
		users_list += std::to_string(current_turn->username->length());
		users_list += '.';
		users_list += *current_turn->username;

		// Append the names of the users waiting in the queue
		for (auto it = turn_queue.begin(); it != turn_queue.end(); it++)
		{
			users_list += ',';

			std::shared_ptr<CollabVMUser> user = *it;
			users_list += std::to_string(user->username->length());
			users_list += '.';
			users_list += *user->username;
		}

		users_list += ';';

		std::string turn_instr = "4.turn,";

		// Append time remaining for the current user's turn
		temp_str = std::to_string(time_remaining);
		turn_instr += std::to_string(temp_str.length());
		turn_instr += '.';
		turn_instr += temp_str;
		turn_instr += ',';

		turn_instr += users_list;

		// Send the instruction to the user that has control first
		current_turn->waiting_turn = false;
		SendWSMessage(*current_turn, turn_instr);

		if (!turn_queue.empty())
		{
			// Replace the semicolon at the end of the string with a comma
			turn_instr.replace(turn_instr.length() - 1, 1, 1, ',');

			// The length of the turn instruction before appending the time to it
			const size_t instr_len_before = turn_instr.length();
			// Send the instruction to each user waiting in the queue and include
			// the amount of time until their turn
			const uint32_t turn_time = controller.GetSettings().TurnTime * 1000;
			uint32_t user_wait_time = time_remaining;
			for (auto it = turn_queue.begin(); it != turn_queue.end(); it++, user_wait_time += turn_time)
			{
				std::shared_ptr<CollabVMUser> user = *it;
				temp_str = std::to_string(user_wait_time);
				turn_instr += std::to_string(temp_str.length());
				turn_instr += '.';
				turn_instr += temp_str;
				turn_instr += ';';
				SendWSMessage(*user, turn_instr);

				// Remove the part that we just appended
				turn_instr.resize(instr_len_before);
			}

			// Replace the comma at the end of the string with a semicolon
			turn_instr.replace(turn_instr.length() - 1, 1, 1, ';');
		}

		// Tell all the spectators how many users are in the waiting queue
		users.ForEachUser([&](CollabVMUser& user)
		{
			if (user.waiting_turn || &user == current_turn)
				return;
			SendWSMessage(user, turn_instr);
		});
	}
}

void CollabVMServer::SendTurnInfo(CollabVMUser& user, uint32_t time_remaining, const std::string& current_turn, const std::deque<std::shared_ptr<CollabVMUser>>& turn_queue)
{
	std::string instr = "4.turn,";

	// Remaining time for the current user's turn
	std::string temp_str = std::to_string(time_remaining);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	//instr += user_list;
	// Number of users
	temp_str = std::to_string(turn_queue.size() + 1);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	// Current user controlling the VM
	instr += std::to_string(current_turn.length());
	instr += '.';
	instr += current_turn;
	// Users waiting in the queue
	for (auto it = turn_queue.begin(); it != turn_queue.end(); it++)
	{
		instr += ',';

		std::shared_ptr<CollabVMUser> user = *it;
		instr += std::to_string(user->username->length());
		instr += '.';
		instr += *user->username;
	}

	instr += ';';

	SendWSMessage(user, instr);
}

void CollabVMServer::BroadcastVoteInfo(const VMController& vm, UserList& users, bool vote_started, uint32_t time_remaining, uint32_t yes_votes, uint32_t no_votes)
{
	std::string instr = "4.vote,";
	// Opcode
	instr += vote_started ? "1.0," : "1.1,";
	// Time remaining to vote
	std::string temp_str = std::to_string(time_remaining);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	// Number of yes votes
	temp_str = std::to_string(yes_votes);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	// Number of no votes
	temp_str = std::to_string(no_votes);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;

	instr += ';';
	users.ForEachUser([&](CollabVMUser& user)
	{
		SendWSMessage(user, instr);
	});
}

void CollabVMServer::SendVoteInfo(const VMController& vm, CollabVMUser& user, uint32_t time_remaining, uint32_t yes_votes, uint32_t no_votes)
{
	std::string instr = "4.vote,1.1,";
	// Time remaining to vote
	std::string temp_str = std::to_string(time_remaining);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	// Number of yes votes
	temp_str = std::to_string(yes_votes);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	// Number of no votes
	temp_str = std::to_string(no_votes);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ';';

	SendWSMessage(user, instr);
}

void CollabVMServer::UserStartedVote(const VMController& vm, UserList& users, CollabVMUser& user)
{
	user.ip_data.votes[&vm] = IPData::VoteDecision::kYes;

#define MSG " started a vote to reset the VM."
	std::string instr = "4.chat,0.,";
	instr += std::to_string(user.username->length() + STR_LEN(MSG));
	instr += '.';
	instr += *user.username;
	instr += MSG ";";
	user.voted_amount++;
	users.ForEachUser([&](CollabVMUser& user)
	{
		SendWSMessage(user, instr);
	});
}

void CollabVMServer::UserVoted(const VMController& vm, UserList& users, CollabVMUser& user, bool vote)
{

	if(user.voted_limit) {
		// if you're a votebomber you shouldn't have a decision anyways
		user.ip_data.votes[&vm] = IPData::VoteDecision::kNotVoted;
		return;
	} else {
		user.ip_data.votes[&vm] = vote ? IPData::VoteDecision::kYes : IPData::VoteDecision::kNo;
	}

#define MSG_YES " voted yes."
#define MSG_NO " voted no."
	std::string instr = "4.chat,0.,";
	size_t msg_len = vote ? STR_LEN(MSG_YES) : STR_LEN(MSG_NO);
	instr += std::to_string(user.username->length() + msg_len);
	instr += '.';
	instr += *user.username;
	instr += vote ? MSG_YES ";" : MSG_NO ";";


	users.ForEachUser([&](CollabVMUser& user)
	{
		SendWSMessage(user, instr);
	});
}

void CollabVMServer::BroadcastVoteEnded(const VMController& vm, UserList& users, bool vote_succeeded)
{
	std::string instr = vote_succeeded ? "4.chat,0.,33.The vote to reset the VM has won.;" : "4.chat,0.,34.The vote to reset the VM has lost.;";
		
	users.ForEachUser([&](CollabVMUser& user)
	{
		SendWSMessage(user, "4.vote,1.2;");
		SendWSMessage(user, instr);
		// Reset the vote amount for all users.
		// TODO: Make this only act on users who have voted at least once
		if(user.voted_amount) { user.voted_amount = 0; }
		user.voted_limit = false;
	});

	// Reset for next vote and allow IPData to be deleted
	for (const auto& ip_data : ipv4_data_)
		ip_data.second->votes[&vm] = IPData::VoteDecision::kNotVoted;
	for (const auto& ip_data : ipv6_data_)
		ip_data.second->votes[&vm] = IPData::VoteDecision::kNotVoted;
}

void CollabVMServer::VoteCoolingDown(CollabVMUser& user, uint32_t time_remaining)
{
	std::string temp_str = std::to_string(time_remaining);
	std::string instr = "4.vote,1.3,";
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ';';
	SendWSMessage(user, instr);
}

void CollabVMServer::Stop()
{
	if (stopping_)
		return;
	stopping_ = true;

	// Don't log opertaion aborted error caused by stop_listening
	server_.set_error_channels(websocketpp::log::elevel::none);

	websocketpp::lib::error_code ws_ec;
	server_.stop_listening(ws_ec);
	if (ws_ec)
		std::cout << "stop_listening error: " << ws_ec.message() << std::endl;

	// Stop the timers
	boost::system::error_code asio_ec;
	keep_alive_timer_.cancel(asio_ec);
	ip_data_timer.cancel(asio_ec);
	vm_preview_timer_.cancel(asio_ec);

	if (process_thread_running_)
	{
		unique_lock<std::mutex> lock(process_queue_lock_);
		// Delete all actions currently in the queue and add the
		// shutdown action to signal the processing queue to disconnect
		// all websocket clients and stop all VM controllers
		while (!process_queue_.empty())
		{
			delete process_queue_.front();
			process_queue_.pop();
		}
		// Clear processing queue by swapping it with an empty one
		std::queue<Action*> emptyQueue;
		process_queue_.swap(emptyQueue);
		// Add the stop action to the processing queue
		process_queue_.push(new Action(ActionType::kShutdown));

		lock.unlock();
		process_wait_.notify_one();
	}
}

void CollabVMServer::OnVMControllerStateChange(const std::shared_ptr<VMController>& controller, VMController::ControllerState state)
{
	std::cout << "VM controller ID: " << controller->GetSettings().Name;
	switch (state)
	{
	case VMController::ControllerState::kStopped:
		std::cout << " is stopped";
		break;
	case VMController::ControllerState::kStarting:
		std::cout << " is starting";
		break;
	case VMController::ControllerState::kRunning:
		std::cout << " has been started";
		break;
	case VMController::ControllerState::kStopping:
		std::cout << " is stopping";
		break;
	}
	std::cout << std::endl;

	std::unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new VMStateChange(controller, state));
	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::OnVMControllerCleanUp(const std::shared_ptr<VMController>& controller)
{
	std::unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new VMAction(controller, ActionType::kVMCleanUp));
	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::SendGuacMessage(const std::weak_ptr<void>& user_ptr, const string& str)
{
	std::shared_ptr<void> ptr = user_ptr.lock();
	if (!ptr)
		return;

	CollabVMUser* user = (CollabVMUser*)ptr.get();
	if (!user->connected)
		return;

	websocketpp::lib::error_code ec;
	server_.send(user->handle, str, websocketpp::frame::opcode::text, ec);
}

inline void CollabVMServer::AppendChatMessage(std::ostringstream& ss, ChatMessage* chat_msg)
{
	ss << ',' << chat_msg->username->length() << '.' << *chat_msg->username <<
		',' << chat_msg->message.length() << '.' << chat_msg->message;
}

void CollabVMServer::SendChatHistory(CollabVMUser& user)
{
	// ... How did we ever get here?
	// we need the user to have a valid fuck
	if(user.vm_controller == nullptr)
		return;

	// handy alias
	auto controller = user.vm_controller;

	// controller did not allocate it... ?
	if(controller->chat_history_ == nullptr)
		return;

	if (controller->chat_history_count_)
	{
		std::ostringstream ss("4.chat", std::ios_base::in | std::ios_base::out | std::ios_base::ate);
		unsigned char len = database_.Configuration.ChatMsgHistory;
		
		// Iterate through each of the messages in the circular buffer
		if (controller->chat_history_end_ > controller->chat_history_begin_)
		{
			for (unsigned char i = controller->chat_history_begin_; i < controller->chat_history_end_; i++)
				AppendChatMessage(ss, &controller->chat_history_[i]);
		}
		else
		{
			for (unsigned char i = chat_history_begin_; i < len; i++)
				AppendChatMessage(ss, &controller->chat_history_[i]);

			for (unsigned char i = 0; i < chat_history_end_; i++)
				AppendChatMessage(ss, &controller->chat_history_[i]);
		}
		ss << ';';
		SendWSMessage(user, ss.str());
	}
}

bool CollabVMServer::ValidateUsername(const std::string& username)
{
	if (username.length() < kMinUsernameLen || username.length() > kMaxUsernameLen)
		return false;

	// Cannot start with a space
	char c = username[0];
	if (!((c >= 'A' && c <= 'Z') || // Uppercase letters
		(c >= 'a' && c <= 'z') || // Lowercase letters
		(c >= '0' && c <= '9') || // Numbers
		c == '_' || c == '-' || c == '.' || c == '?' || c == '!')) // Underscores, dashes, dots, question marks, exclamation points
		return false;

	bool prev_space = false;
	for (size_t i = 1; i < username.length() - 1; i++)
	{
		c = username[i];
		// Only allow the following characters
		if ((c >= 'A' && c <= 'Z') || // Uppercase letters
			(c >= 'a' && c <= 'z') || // Lowercase letters
			(c >= '0' && c <= '9') || // Numbers
			c == '_' || c == '-' || c == '.' || c == '?' || c == '!') // Spaces, underscores, dashes, dots, question marks, exclamation points
		{
			prev_space = false;
			continue;
		}
		// Check that the previous character was not a space
		if (!prev_space && c == ' ')
		{
			prev_space = true;
			continue;
		}
		return false;
	}

	// Cannot end with a space
	c = username[username.length()-1];
	if ((c >= 'A' && c <= 'Z') || // Uppercase letters
		(c >= 'a' && c <= 'z') || // Lowercase letters
		(c >= '0' && c <= '9') || // Numbers
		c == '_' || c == '-' || c == '.' || c == '?' || c == '!') // Underscores, dashes, dots, question marks, exclamation points
		return true;
		
	return false;
}

void CollabVMServer::SendOnlineUsersList(CollabVMUser& user)
{
	std::ostringstream ss("7.adduser,", std::ios_base::in | std::ios_base::out | std::ios_base::ate);
	std::string num = std::to_string(UserListSize(user));
	ss << num.size() << '.' << num;

	IterateUserList(user, [&](CollabVMUser& data) {
		num = std::to_string(data.user_rank);
		ss << ',' << data.username->length() << '.' << *data.username << ',' << num.size() << '.' << num;
	});

	ss << ';';
	SendWSMessage(user, ss.str());
}

void CollabVMServer::ChangeUsername(const std::shared_ptr<CollabVMUser>& data, const std::string& new_username, UsernameChangeResult result, bool send_history)
{
	// Send a rename instruction to the client telling them their username
	std::string instr;
	instr = "6.rename,1.0,1.";
	instr += (char)result;
	instr += ',';
	instr += std::to_string(new_username.length());
	instr += '.';
	instr += new_username;
	instr += ',';
	std::string rank = std::to_string(data->user_rank);
	instr += std::to_string(rank.length());
	instr += '.';
	instr += rank;
	instr += ';';
	SendWSMessage(*data, instr);

	// If the result of the username change was not successful
	// then don't broadcast the change to any other clients
	if (result != UsernameChangeResult::kSuccess)
		return;

	{
		// If the client did not previously have a username then
		// it means the client is joining the server
		bool user_joining = !data->username;

		// Create an adduser instruction if the client is joining,
		// otherwise create a rename instruction to update the user's username
		if (user_joining)
		{
			instr = "7.adduser,1.1,";
		}
		else
		{
			instr = "6.rename,1.1,";
			// Append the old username
			instr += std::to_string(data->username->length());
			instr += '.';
			instr += *data->username;
			instr += ',';
		}
		// Append the new username and user type to the instruction
		instr += std::to_string(new_username.length());
		instr += '.';
		instr += new_username;
		instr += ',';
		instr += std::to_string(rank.length());
		instr += '.';
		instr += rank;
		instr += ';';

		IterateUserList(*data, [&](CollabVMUser& u) {
			SendWSMessage(u, instr);	
		});
	}

	if (data->username)
	{
		std::cout << "[Username Changed] IP: " << data->ip_data.GetIP() << " Old: \"" << *data->username << "\" New: \"" << new_username << '"' << std::endl;
		data->username->assign(new_username);
	}
	else
	{
		data->username = std::make_shared<std::string>(new_username);
		std::cout << "[Username Assigned] IP: " << data->ip_data.GetIP() << " New username: \"" << new_username << '"' << std::endl;
	}
}

std::string CollabVMServer::GenerateUsername(CollabVMUser& ob)
{
	// If the username is already taken generate a new one
	uint32_t num = guest_rng_(rng_);
	string username = "guest" + std::to_string(num);

	// Increment the number until a username is found that is not taken
	while (IsUsernameTaken(ob, username) != false)
	{
		username = "guest" + std::to_string(++num);
	}
	return username;
}

string CollabVMServer::EncodeHTMLString(const char* str, size_t strLen)
{
	ostringstream ss;
	for (size_t i = 0; i < strLen; i++)
	{
		char c = str[i];
		switch (c)
		{
		case '&':
			ss << "&amp;";
			break;
		case '<':
			ss << "&lt;";
			break;
		case '>':
			ss << "&gt;";
			break;
		case '"':
			ss << "&quot;";
			break;
		case '\'':
			ss << "&#x27;";
			break;
		case '/':
			ss << "&#x2F;";
			break;
		case '\n':
			ss << "&#13;&#10;";
			break;
		default:
			// Only allow printable ASCII characters
			if (c >= 32 && c <= 126)
				ss << c;
		}
	}
	return ss.str();
}

void CollabVMServer::OnMouseInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	// Only allow a user to send mouse instructions if it is their turn,
	// they are an admin, or they are connected to the admin panel
	if (user->vm_controller != nullptr &&
		(user->vm_controller->CurrentTurn() == user ||
		user->user_rank == UserRank::kAdmin ||
		user->admin_connected) && user->guac_user != nullptr && user->guac_user->client_)
	{
		user->guac_user->client_->HandleMouse(*user->guac_user, args);
	}
}

void CollabVMServer::OnKeyInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	// Only allow a user to send keyboard instructions if it is their turn,
	// they are an admin, or they are connected to the admin panel
	if (user->vm_controller != nullptr &&
		(user->vm_controller->CurrentTurn() == user ||
			user->user_rank == UserRank::kAdmin ||
			user->admin_connected) && user->guac_user != nullptr && user->guac_user->client_)
	{
		user->guac_user->client_->HandleKey(*user->guac_user, args);
	}
}

void CollabVMServer::OnRenameInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	if (args.empty())
	{
		// The users wants the server to generate a username for them
		if (!user->username)
		{
			ChangeUsername(user, GenerateUsername(*user), UsernameChangeResult::kSuccess, false);
		}
		return;
	}
	// The requested username
	std::string username = args[0];

	// Check if the requested username and current usernames are the same
	if (user->username && *user->username.get() == username)
		return;

	// Whether a new username should be generated for the user
	bool gen_username = false;
	UsernameChangeResult result;

	if (username.empty())
	{
		gen_username = true;
		result = UsernameChangeResult::kSuccess;
	}
	else if (!ValidateUsername(username))
	{
		// The requested username is invalid
		if (!user->username)
		{
			// Respond with successful result and generate a new
			// username so the user can join
			gen_username = true;
			result = UsernameChangeResult::kSuccess;
		}
		else
		{
			result = UsernameChangeResult::kInvalid;
		}
	}
	else
	{
		result = UsernameChangeResult::kSuccess;
	
		// Just let them have the username for right now,
		// when the user connects to a VM controller
		// we will enforce username taken/invalid, such and such
		if(!user->vm_controller) {
			ChangeUsername(user, username, result, args.size() > 1);
			return;
		}

		if(IsUsernameTaken(*user, username)) {
			if(!user->username) {
				// effectively an echo of the previous logic
				gen_username = true;
				ChangeUsername(user, GenerateUsername(*user), result, args.size() > 1);
			} else {
				result = UsernameChangeResult::kUsernameTaken;
			}
		}

		ChangeUsername(user, username, result, args.size() > 1);
		return;
	}

	// dead code?
	ChangeUsername(user, gen_username ? GenerateUsername(*user) : *user->username.get(), result, args.size() > 1);
}

/**
 * Appends the VM actions to the instruction.
 */
static void AppendVMActions(const VMController& controller, const VMSettings& settings, std::string& instr)
{
	instr += settings.TurnsEnabled ? "1,1." : "0,1.";
	instr += settings.VotesEnabled ? "1,1." : "0,1.";
	if (settings.UploadsEnabled && controller.agent_connected_)
	{
		instr += "1,";
		std::string temp = std::to_string(settings.MaxUploadSize);
		instr += std::to_string(temp.length());
		instr += '.';
		instr += temp;
		instr += ',';
		temp = std::to_string(controller.agent_max_filename_);
		instr += std::to_string(temp.length());
		instr += '.';
		instr += temp;
		instr += ';';
	}
	else
	{
		instr += "0;";
	}
}

void CollabVMServer::SendActionInstructions(VMController& controller, const VMSettings& settings)
{
	std::string instr = "6.action,1.";
	AppendVMActions(controller, settings, instr);
	controller.GetUsersList().ForEachUser([this, &instr](CollabVMUser& user)
	{
		SendWSMessage(user, instr);
	});
}

void CollabVMServer::OnConnectInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	if (args.empty())
	{
		// Disconnect
		if (user->vm_controller != nullptr)
		{
			if (user->upload_info)
			{
				CancelFileUpload(*user);
				SendWSMessage(*user, "4.file,1.3;");
			}
			user->vm_controller->RemoveUser(user);

			delete user->guac_user;
			user->guac_user = nullptr;
			SendWSMessage(*user, "7.connect,1.2;");
		}
		return;
	}
	else if (args.size() != 1 || user->guac_user != nullptr)
	{
		return;
	}

	// Connect
	std::string vm_name = args[0];
	if (vm_name.empty())
	{
		// Invalid VM ID
		SendWSMessage(*user, "7.connect,1.0;");
		return;
	}
	auto it = vm_controllers_.find(vm_name);
	if (it == vm_controllers_.end())
	{
		// VM not found
		SendWSMessage(*user, "7.connect,1.0;");
		return;
	}

	VMController& controller = *it->second;

	// Send cooldown time before action instruction
	if (user->ip_data.upload_in_progress)
		SendWSMessage(*user, "4.file,1.6;");
	else
		SendUploadCooldownTime(*user, controller);

	std::string instr = "7.connect,1.1,1.";
	AppendVMActions(controller, controller.GetSettings(), instr);
	SendWSMessage(*user, instr);

	// TODO: This may be causing a memory leak
	user->guac_user = new GuacUser(this, user);
	controller.AddUser(user);

	// Generate a username if the user joins with
	// one that collides with this controller's user list.
	if(IsUsernameTaken(*user, *user->username)) {
		ChangeUsername(user, GenerateUsername(*user), UsernameChangeResult::kSuccess, false);
	}
		
	// NOTE: This is a sequence change
	// to allow these to work
	SendOnlineUsersList(*user);
	SendChatHistory(*user);
}

void CollabVMServer::OnAdminInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	// This instruction should have at least one argument
	if (args.empty())
		return;

	std::string str = args[0];
	if (str.length() > 2)
		return;

	int opcode;
	try
	{
		opcode = std::stoi(str);
	}
	catch (...)
	{
		return;
	}

	if (!user->admin_connected && opcode != kSeshID && opcode != kMasterPwd)
		return;

	switch (opcode)
	{
	case kStop:
		user->admin_connected = false;
		user->user_rank = UserRank::kUnregistered;
		admin_connections_.erase(user);
		break;
	case kSeshID:
		if (args.size() == 1)
		{
			// The user did not provide an admin session ID
			// so check if their rank is admin
			//if (user->user_rank == UserRank::kAdmin)
		}
		else if (args.size() == 2)
		{
			if (!admin_session_id_.empty() && args[1] == admin_session_id_)
			{
				user->admin_connected = true;
				user->user_rank = UserRank::kAdmin;
				admin_connections_.insert(user);

				// Send login success response
				SendWSMessage(*user, "5.admin,1.0,1.1;");

				if(user->vm_controller != nullptr)
					SendOnlineUsersList(*user); // send userlist if connected to a VM
			}
			else
			{
				// Send login failed response
				SendWSMessage(*user, "5.admin,1.0,1.0;");
			}
		}
		break;
	case kMasterPwd:
		if (args.size() == 2 && args[1] == database_.Configuration.MasterPassword)
		{
			user->admin_connected = true;
			user->user_rank = UserRank::kAdmin;
			admin_connections_.insert(user);

			// Send login success response
			SendWSMessage(*user, "5.admin,1.0,1.1;");

			if(user->vm_controller != nullptr)
				SendOnlineUsersList(*user); // send userlist if connected to a VM
		}
		else
		{
			// Send login failed response
			SendWSMessage(*user, "5.admin,1.0,1.0;");
		}
		break;
	case kGetSettings: {
		ostringstream ss("5.admin,1.1,", ostringstream::in | ostringstream::out | ostringstream::ate);
		string resp = GetServerSettings();
		ss << resp.length() << '.' << resp << ';';
		SendWSMessage(*user, ss.str());
		break;
	}
	case kSetSettings:
		if (args.size() == 2)
		{
			ostringstream ss("5.admin,1.1,", ostringstream::in | ostringstream::out | ostringstream::ate);
			string resp = PerformConfigFunction(args[1]);
			ss << resp.length() << '.' << resp << ';';
			SendWSMessage(*user, ss.str());
		}
		break;
	case kQEMU:
	{
		if (args.size() != 3)
			break;

		auto it = vm_controllers_.find(args[1]);
		if (it == vm_controllers_.end())
		{
			// VM not found
			SendWSMessage(*user, "5.admin,1.0,1.2;");
			break;
		}

		size_t strLen = strlen(args[2]);
		if (!strLen)
			break;

		// TODO: Verify that the VMController is a QEMUController
		std::static_pointer_cast<QEMUController>(it->second)->SendMonitorCommand(string(args[2], strLen),
			std::bind(&CollabVMServer::OnQEMUResponse, shared_from_this(), user, std::placeholders::_1));
		break;
	}
	case kStartController:
	{
		if (args.size() != 2)
			return;

		std::string vm_name = args[1];
		auto vm_it = vm_controllers_.find(vm_name);
		if (vm_it == vm_controllers_.end())
		{
			auto db_it = database_.VirtualMachines.find(vm_name);
			if (db_it != database_.VirtualMachines.end())
			{
				CreateVMController(db_it->second)->Start();
				SendWSMessage(*user, "5.admin,1.1,15.{\"result\":true};");
			}
			else
			{
				// VM not found
				SendWSMessage(*user, "5.admin,1.0,1.2;");
			}
		}
		else
		{
			// VM already running, report success
			SendWSMessage(*user, "5.admin,1.1,15.{\"result\":true};");
		}
		break;
	}
	case kStopController:
	case kRestoreVM:
	case kRebootVM:
	case kResetVM:
	case kRestartVM:
		for (size_t i = 1; i < args.size(); i++)
		{
			auto it = vm_controllers_.find(args[i]);
			if (it == vm_controllers_.end())
			{
				// VM not found
				SendWSMessage(*user, "5.admin,1.0,1.2;");
				continue;
			}

			switch (opcode)
			{
			//case kStartController:
			//	it->second->Start();
			//	break;
			case kStopController:
				it->second->Stop(VMController::StopReason::kRemove);
				break;
			case kRestoreVM:
				it->second->RestoreVMSnapshot();
				break;
			case kRebootVM:
				it->second->PowerOffVM();
				break;
			case kResetVM:
				it->second->ResetVM();
				break;
			case kRestartVM:
				it->second->Stop(VMController::StopReason::kRestart);
				break;
			}
			// Send success response
			SendWSMessage(*user, "5.admin,1.1,15.{\"result\":true};");
		}
		break;
	}
}

void CollabVMServer::OnListInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	std::string instr("4.list");
	for (auto it = vm_controllers_.begin(); it != vm_controllers_.end(); it++)
	{
		instr += ',';
		const VMSettings& vm_settings = it->second->GetSettings();
		instr += std::to_string(vm_settings.Name.length());
		instr += '.';
		instr += vm_settings.Name;
		instr += ',';
		instr += std::to_string(vm_settings.DisplayName.length());
		instr += '.';
		instr += vm_settings.DisplayName;

		instr += ',';
		std::string* png = it->second->GetThumbnail();
		if (png && png->length())
		{
			instr += std::to_string(png->length());
			instr += '.';
			instr += *png;
		}
		else
		{
			instr += "0.";
		}
	}
	instr += ';';
	SendWSMessage(*user, instr);
}

void CollabVMServer::OnNopInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	user->last_nop_instr = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());
}

void CollabVMServer::OnQEMUResponse(std::weak_ptr<CollabVMUser> data, rapidjson::Document& d)
{
	auto ptr = data.lock();
	if (!ptr)
		return;

	rapidjson::Value::MemberIterator r = d.FindMember("return");
	if (r != d.MemberEnd() && r->value.IsString() && r->value.GetStringLength() > 0)
	{
		rapidjson::Value& v = r->value;
		string msg = EncodeHTMLString(v.GetString(), v.GetStringLength());
		if (msg.length() < 1)
			return;

		ostringstream ss("5.admin,1.2,", ostringstream::in | ostringstream::out | ostringstream::ate);
		ss << msg.length() << '.' << msg << ';';

		websocketpp::lib::error_code ec;
		server_.send(ptr->handle, ss.str(), websocketpp::frame::opcode::text, ec);
	}
}

void CollabVMServer::OnChatInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	if (args.size() != 1 || !user->username)
		return;

	// The user must be on a VM controller to chat
	if(!user->vm_controller)
		return;

	// Limit message send rate
	auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());
	if (user->ip_data.chat_muted)
	{
		if ((now - user->ip_data.last_chat_msg).count() >= database_.Configuration.ChatMuteTime)
			user->ip_data.chat_muted = false;
		else
			return;
	}

	if (database_.Configuration.ChatRateCount && database_.Configuration.ChatRateTime)
	{
		// Calculate the time since the user's last message
		if ((now - user->ip_data.last_chat_msg).count() < database_.Configuration.ChatRateTime)
		{
			if (++user->ip_data.chat_msg_count >= database_.Configuration.ChatRateCount)
			{
				std::string mute_time = std::to_string(database_.Configuration.ChatMuteTime);
				std::cout << "[Chat] User was muted for " << mute_time <<
					" seconds. IP: " << user->ip_data.GetIP() << " Username: \"" <<
					*user->username << '"' << std::endl;
				// Mute the user for attempting to go over the
				// chat message limit
				user->ip_data.last_chat_msg = now;
				user->ip_data.chat_muted = true;

#define part1 "You have been muted for "
#define part2 " seconds."
				std::string instr = "4.chat,0.,";
				instr += std::to_string(mute_time.length() + sizeof(part1) + sizeof(part2) - 2);
				instr += "." part1;
				instr += mute_time;
				instr += part2 ";";
				SendWSMessage(*user, instr);
				return;
			}
		}
		else
		{
			user->ip_data.chat_msg_count = 0;
			user->ip_data.last_chat_msg = now;
		}
	}

	size_t str_len = strlen(args[0]);
	if (!str_len || str_len > kMaxChatMsgLen)
		return;
		
	string msg = EncodeHTMLString(args[0], str_len);
	if (msg.empty())
		return;

	if (database_.Configuration.ChatMsgHistory)
	{
		// Add the message to the VM chat history
		ChatMessage* chat_message = &user->vm_controller->chat_history_[user->vm_controller->chat_history_end_];
		chat_message->timestamp = now;
		chat_message->username = user->username;
		chat_message->message = msg;

		uint8_t last_index = database_.Configuration.ChatMsgHistory - 1;

		if (user->vm_controller->chat_history_end_ == user->vm_controller->chat_history_begin_ && user->vm_controller->chat_history_count_)
		{
			// Increment the begin index
			if (user->vm_controller->chat_history_begin_ == last_index)
				user->vm_controller->chat_history_begin_ = 0;
			else
				user->vm_controller->chat_history_begin_++;
		}
		else
		{
			user->vm_controller->chat_history_count_++;
		}

		// Increment the end index
		if (user->vm_controller->chat_history_end_ == last_index)
			user->vm_controller->chat_history_end_ = 0;
		else
			user->vm_controller->chat_history_end_++;
	}


	std::string instr = "4.chat,";
	instr += std::to_string(user->username->length());
	instr += '.';
	instr += *user->username;
	instr += ',';
	instr += std::to_string(msg.length());
	instr += '.';
	instr += msg;
	instr += ';';

	for (auto it = connections_.begin(); it != connections_.end(); it++)
		// TODO: This really should be fixed in a better way
		// but cosmic code is annyoing
		if(user->vm_controller == (**it).vm_controller)
			SendWSMessage(**it, instr);
}

void CollabVMServer::OnTurnInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	if (user->vm_controller != nullptr && user->username)
		user->vm_controller->TurnRequest(user);
}

void CollabVMServer::OnVoteInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	if (args.size() == 1 && user->vm_controller != nullptr && user->username)
		user->vm_controller->Vote(*user, args[0][0] == '1');
}

void CollabVMServer::OnFileInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args)
{
	if (!user->vm_controller || args.empty() || args[0][0] == '\0' ||
		!user->vm_controller->GetSettings().UploadsEnabled)
		return;

	switch (static_cast<ClientFileOp>(args[0][0]))
	{
	case ClientFileOp::kBegin:
		if (args.size() == 4 && !user->upload_info && !user->ip_data.upload_in_progress)
		{
			if (SendUploadCooldownTime(*user, *user->vm_controller))
				break;

			std::string filename = args[1];
			if (!IsFilenameValid(filename))
				break;

			uint32_t file_size = std::strtoul(args[2], nullptr, 10);
			bool run_file = args[3][0] == '1';
			if (user->vm_controller->IsFileUploadValid(user, filename, file_size, run_file))
			{
				bool found = false;
				std::shared_ptr<VMController> vm;
				for (auto vm_controller : vm_controllers_)
				{
					if (vm_controller.second.get() == user->vm_controller)
					{
						vm = vm_controller.second;
						found = true;
						break;
					}
				}
				assert(found);

				user->upload_info = std::make_shared<UploadInfo>(user, vm, filename, file_size, run_file,
													user->vm_controller->GetSettings().UploadCooldownTime);
				std::unique_lock<std::mutex> lock(upload_lock_);
				user->upload_info->upload_it = upload_ids_.end();
				lock.unlock();
				user->ip_data.upload_in_progress = true;
				if (upload_count_ != kMaxFileUploads)
				{
					StartFileUpload(*user);
				}
				else
				{
					user->waiting_for_upload = true;
					upload_queue_.push_back(user);
				}
				break;
			}
		}
		SendWSMessage(*user, "4.file,1.5;");
		break;
	}
}

void CollabVMServer::OnAgentConnect(const std::shared_ptr<VMController>& controller,
									const std::string& os_name, const std::string& service_pack,
									const std::string& pc_name, const std::string& username, uint32_t max_filename)
{
	std::unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new AgentConnectAction(controller, os_name, service_pack, pc_name, username, max_filename));
	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::OnAgentDisconnect(const std::shared_ptr<VMController>& controller)
{
	std::unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new VMAction(controller, ActionType::kAgentDisconnect));
	lock.unlock();
	process_wait_.notify_one();
}

//void CollabVMServer::OnAgentHeartbeatTimeout(const std::shared_ptr<VMController>& controller)
//{
//	std::unique_lock<std::mutex> lock(process_queue_lock_);
//	process_queue_.push(new VMAction(controller, ActionType::kHeartbeatTimeout));
//	lock.unlock();
//	process_wait_.notify_one();
//}

bool CollabVMServer::IsFilenameValid(const std::string& filename)
{
	for (char c : filename)
	{
		// Only allow printable ASCII characters
		if (c < ' ' || c > '~')
			return false;
		// Characters disallowed by Windows
		switch (c)
		{
		case '<':
		case '>':
		case ':':
		case '"':
		case '/':
		case '\\':
		case '|':
		case '?':
		case '*':
			return false;
		}
	}
	return true;
}

void CollabVMServer::OnUploadTimeout(const boost::system::error_code ec, std::shared_ptr<UploadInfo> upload_info)
{
	if (ec)
		return;

	UploadInfo::HttpUploadState prev_state =
		upload_info->http_state.exchange(UploadInfo::HttpUploadState::kCancel);
	if (prev_state == UploadInfo::HttpUploadState::kNotStarted)
	{
		std::unique_lock<std::mutex> lock(process_queue_lock_);
		process_queue_.push(new HttpAction(ActionType::kHttpUploadTimedout, upload_info));
		lock.unlock();
		process_wait_.notify_one();
	}
	else if (prev_state == UploadInfo::HttpUploadState::kNotWriting)
	{
		std::unique_lock<std::mutex> lock(process_queue_lock_);
		process_queue_.push(new HttpAction(ActionType::kHttpUploadFailed, upload_info));
		lock.unlock();
		process_wait_.notify_one();
	}
}

void CollabVMServer::StartFileUpload(CollabVMUser& user)
{
	assert(user.upload_info);
	const std::shared_ptr<UploadInfo>& upload_info = user.upload_info;
	std::string file_path = kFileUploadPath;
	file_path += std::to_string(upload_count_ + 1);

	upload_info->file_stream.open(file_path, std::fstream::in | std::fstream::out | std::fstream::trunc | std::fstream::binary);
	if (upload_info->file_stream.is_open() && upload_info->file_stream.good())
	{
		upload_count_++;
		upload_info->file_path = file_path;

		boost::asio::steady_timer* timer = new boost::asio::steady_timer(service_);
		std::string upload_id = GenerateUuid();
		unique_lock<std::mutex> lock(upload_lock_);
		auto result = upload_ids_.insert({ upload_id, upload_info });
		while (!result.second)
		{
			upload_id = GenerateUuid();
			result = upload_ids_.insert({ upload_id, upload_info });
		}
		upload_info->upload_it = result.first;
		upload_info->timeout_timer = timer;
		lock.unlock();

		std::string instr = "4.file,1.0,";
		instr += std::to_string(upload_id.length());
		instr += '.';
		instr += upload_id;
		instr += ';';
		SendWSMessage(user, instr);

		boost::system::error_code ec;
		timer->expires_from_now(std::chrono::seconds(kUploadStartTimeout), ec);
		timer->async_wait(std::bind(&CollabVMServer::OnUploadTimeout, shared_from_this(),
									std::placeholders::_1, upload_info));
	}
	else
	{
		CancelFileUpload(user);
		SendWSMessage(user, "4.file,1.5;");

		std::cout << "Error: Failed to create file \"" << file_path << "\" for upload." << std::endl;
	}
}

void CollabVMServer::SendUploadResultToIP(IPData& ip_data, const CollabVMUser& user, const std::string& instr)
{
	for (const shared_ptr<CollabVMUser>& connection : connections_)
		if (&connection->ip_data == &ip_data && connection->vm_controller && connection.get() != &user)
			SendWSMessage(*connection, instr);
}

static void AppendCooldownTime(std::string& instr, uint32_t cooldown_time)
{
	std::string temp = std::to_string(cooldown_time * 1000);
	instr += std::to_string(temp.length());
	instr += '.';
	instr += temp;
	instr += ';';
}

void CollabVMServer::FileUploadEnded(const std::shared_ptr<UploadInfo>& upload_info,
										const std::shared_ptr<CollabVMUser>* user, FileUploadResult result)
{
	if (boost::asio::steady_timer* timer = upload_info->timeout_timer)
	{
		upload_info->timeout_timer = nullptr;
		boost::system::error_code ec;
		timer->cancel(ec);
		delete timer;
	}

	VMController& vm_controller = *upload_info->vm_controller;
	if (user)
	{
		upload_info->file_stream.close();
		std::remove(upload_info->file_path.c_str());

		uint32_t cooldown_time = vm_controller.GetSettings().UploadCooldownTime;
		// The IPData has at least one client associated with it so
		// locking isn't required
		SetUploadCooldownTime(upload_info->ip_data, cooldown_time);

		std::string uploader_instr;
		std::string other_instr;
		switch (result)
		{
		case FileUploadResult::kHttpUploadTimedOut:
			uploader_instr = "4.file,1.7";
			goto send_message;
		case FileUploadResult::kHttpUploadFailed:
			uploader_instr = "4.file,1.5";
			goto send_message;
		case FileUploadResult::kAgentUploadSucceeded:
			uploader_instr = "4.file,1.2";
		send_message:
			if (cooldown_time)
			{
				std::string cooldown_str;
				AppendCooldownTime(cooldown_str, cooldown_time);

				uploader_instr += ',';
				uploader_instr += cooldown_str;

				other_instr = "4.file,1.4,";
				other_instr += cooldown_str;
			}
			else
			{
				uploader_instr += ';';
				other_instr = "4.file,1.4,1.0;";
			}
			SendWSMessage(**user, uploader_instr);
			break;
		}

		if (upload_info->ip_data.connections > 1)
			SendUploadResultToIP(upload_info->ip_data, *user->get(), other_instr);
	}
	else
	{
		// The IPData might not have any clients associated with it so
		// locking is required
		unique_lock<std::mutex> lock(ip_lock_);
		uint32_t cooldown_time = vm_controller.GetSettings().UploadCooldownTime;
		SetUploadCooldownTime(upload_info->ip_data, cooldown_time);
		if (upload_info->ip_data.connections > 1)
		{
			std::string instr = cooldown_time ? "4.file,1.4," : "4.file,1.4,1.0;";
			if (cooldown_time)
				AppendCooldownTime(instr, cooldown_time);
			SendUploadResultToIP(upload_info->ip_data, *user->get(), instr);
		}
		lock.unlock();
	}

	switch (result)
	{
	case FileUploadResult::kHttpUploadTimedOut:
	case FileUploadResult::kHttpUploadFailed:
		StartNextUpload();
		break;
	case FileUploadResult::kAgentUploadSucceeded:
		BroadcastUploadedFileInfo(*upload_info, vm_controller);
		// Fall through
	case FileUploadResult::kAgentUploadFailed:
		if (vm_controller.agent_upload_queue_.empty())
		{
			vm_controller.agent_upload_in_progress_ = false;
			StartNextUpload();
		}
		else
		{
			vm_controller.UploadFile(vm_controller.agent_upload_queue_.front());
			vm_controller.agent_upload_queue_.pop_front();
		}
		break;
	}
}

void CollabVMServer::StartNextUpload()
{
	upload_count_--;
	if (!upload_queue_.empty())
	{
		CollabVMUser& user = *upload_queue_.front();
		upload_queue_.pop_front();
		user.waiting_for_upload = false;
		StartFileUpload(user);
	}
}

void CollabVMServer::CancelFileUpload(CollabVMUser& user)
{
	if (!user.upload_info)
		return;

	UploadInfo::HttpUploadState prev_state =
		user.upload_info->http_state.exchange(UploadInfo::HttpUploadState::kCancel);
	if (prev_state != UploadInfo::HttpUploadState::kNotStarted &&
		prev_state != UploadInfo::HttpUploadState::kNotWriting)
		return;

	if (prev_state == UploadInfo::HttpUploadState::kNotStarted)
	{
		std::lock_guard<std::mutex> lock(upload_lock_);
		if (user.upload_info->upload_it != upload_ids_.end())
			upload_ids_.erase(user.upload_info->upload_it);
	}

	if (boost::asio::steady_timer* timer = user.upload_info->timeout_timer)
	{
		user.upload_info->timeout_timer = nullptr;
		boost::system::error_code ec;
		timer->cancel(ec);
		delete timer;
	}

	uint32_t cooldown_time = user.vm_controller->GetSettings().UploadCooldownTime;
	SetUploadCooldownTime(user.ip_data, cooldown_time);

	if (user.upload_info->ip_data.connections > 1)
	{
		std::string instr = cooldown_time ? "4.file,1.4," : "4.file,1.4,1.0;";
		if (cooldown_time)
			AppendCooldownTime(instr, cooldown_time);
		SendUploadResultToIP(user.upload_info->ip_data, user, instr);
	}

	// Close the fstream first to allow a new one to be opened if there is a pending upload
	user.upload_info->file_stream.close();
	user.upload_info.reset();

	if (user.waiting_for_upload)
	{
		user.waiting_for_upload = false;
		bool user_found = false;
		for (auto it = upload_queue_.begin(); it != upload_queue_.end(); it++)
		{
			if ((*it).get() == &user)
			{
				upload_queue_.erase(it);
				user_found = true;
				break;
			}
		}
		assert(user_found);
	}
	else
	{
		StartNextUpload();
	}
}

void CollabVMServer::SetUploadCooldownTime(IPData& ip_data, uint32_t time)
{
	ip_data.upload_in_progress = false;
	if (time)
		ip_data.next_upload_time = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now()) + std::chrono::seconds(time);
}

bool CollabVMServer::SendUploadCooldownTime(CollabVMUser& user, const VMController& vm_controller)
{
	if (vm_controller.GetSettings().UploadCooldownTime)
	{
		using namespace std::chrono;
		int64_t remaining = (user.ip_data.next_upload_time -
			time_point_cast<milliseconds>(steady_clock::now())).count();

		if (remaining > 0)
		{
			std::string instr = "4.file,1.4,";
			std::string temp = std::to_string(remaining);
			instr += std::to_string(temp.length());
			instr += '.';
			instr += temp;
			instr += ';';
			SendWSMessage(user, instr);
			return true;
		}
	}

	return false;
}

void CollabVMServer::BroadcastUploadedFileInfo(UploadInfo& upload_info, VMController& vm_controller)
{
#define UPLOAD_STR " uploaded "
#define FILE_SIZE_PREFIX " ("
#define FILE_SIZE_SUFFIX " bytes)"
	std::string instr = "4.chat,0.,";
	std::string file_size = std::to_string(upload_info.file_size);
	std::string escaped_filename = EncodeHTMLString(upload_info.filename.c_str(), upload_info.filename.length());
	instr += std::to_string(upload_info.username->length() + STR_LEN(UPLOAD_STR) + escaped_filename.length() +
		STR_LEN(FILE_SIZE_PREFIX) + file_size.length() + STR_LEN(FILE_SIZE_SUFFIX));
	instr += '.';
	instr += *upload_info.username;
	instr += UPLOAD_STR;
	instr += escaped_filename;
	instr += FILE_SIZE_PREFIX;
	instr += file_size;
	instr += FILE_SIZE_SUFFIX;
	instr += ';';

	vm_controller.GetUsersList().ForEachUser([&](CollabVMUser& user)
	{
		SendWSMessage(user, instr);
	});
}

void CollabVMServer::OnFileUploadFailed(const std::shared_ptr<VMController>& controller, const std::shared_ptr<UploadInfo>& info)
{
	assert(controller == info->vm_controller);
	std::unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new FileUploadAction(info, FileUploadAction::UploadResult::kFailed));
	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::OnFileUploadFinished(const std::shared_ptr<VMController>& controller, const std::shared_ptr<UploadInfo>& info)
{
	assert(controller == info->vm_controller);
	std::unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new FileUploadAction(info, FileUploadAction::UploadResult::kSuccess));
	lock.unlock();
	process_wait_.notify_one();
}

void CollabVMServer::OnFileUploadExecFinished(const std::shared_ptr<VMController>& controller, const std::shared_ptr<UploadInfo>& info, bool exec_success)
{
	assert(controller == info->vm_controller);
	// TODO: Send exec_success
	std::unique_lock<std::mutex> lock(process_queue_lock_);
	process_queue_.push(new FileUploadAction(info, FileUploadAction::UploadResult::kSuccess));
	lock.unlock();
	process_wait_.notify_one();
}

/**
 * The error message that the server will respond with when
 * it receives a JSON string with an value of the wrong type.
 */
static const std::string invalid_object_ = "Invalid object type";

/**
 * Simple convenience function for writing an object
 * to a JSON string.
 */
static void WriteJSONObject(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::string key, const std::string val)
{
	writer.StartObject();
	writer.String(key.c_str());
	writer.String(val.c_str());
	writer.EndObject();
}

/**
 * Parses the JSON object and changes the setings of the VMSettings.
 * @returns Whether the settings were valid
 */
bool CollabVMServer::ParseVMSettings(VMSettings& vm, rapidjson::Value& settings, rapidjson::Writer<rapidjson::StringBuffer>& writer)
{
	bool valid = true;
	for (auto it = settings.MemberBegin(); it != settings.MemberEnd(); it++)
	{
		for (size_t i = 0; i < sizeof(vm_settings_)/sizeof(std::string); i++)
		{
			std::string key(it->name.GetString(), it->name.GetStringLength());
			if (key == vm_settings_[i])
			{
				rapidjson::Value& value = it->value;
				switch (i)
				{
				case kName:
					if (value.IsString() && value.GetStringLength() > 0)
					{
						vm.Name = std::string(value.GetString(), value.GetStringLength());
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kName], invalid_object_);
						valid = false;
					}
					break;
				case kAutoStart:
					if (value.IsBool())
					{
						vm.AutoStart = value.GetBool();
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kAutoStart], invalid_object_);
						valid = false;
					}
					break;
				case kDisplayName:
					if (value.IsString())
					{
						vm.DisplayName = string(value.GetString(), value.GetStringLength());
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kDisplayName], invalid_object_);
						valid = false;
					}
					break;
				case kHypervisor:
				{
					if (!value.IsString())
					{
						WriteJSONObject(writer, vm_settings_[kHypervisor], invalid_object_);
						valid = false;
					}
					std::string hypervisor = std::string(value.GetString(), value.GetStringLength());
					for (size_t x = 0; x < sizeof(hypervisor_names_)/sizeof(std::string); x++)
					{
						if (hypervisor == hypervisor_names_[x])
						{
							vm.Hypervisor = static_cast<VMSettings::HypervisorEnum>(x);
							goto found_hypervisor;
						}
					}
					// Hypervisor name not recognized
					WriteJSONObject(writer, vm_settings_[kHypervisor], "Unknown hypervisor name");
					valid = false;
				found_hypervisor:
					break;
				}
				case kRestoreShutdown:
					if (value.IsBool())
					{
						vm.RestoreOnShutdown = value.GetBool();
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kRestoreShutdown], invalid_object_);
						valid = false;
					}
					break;
				case kRestoreTimeout:
					if (value.IsBool())
					{
						vm.RestoreOnTimeout = value.GetBool();
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kRestoreTimeout], invalid_object_);
						valid = false;
					}
					break;
				case kVNCAddress:
					if (value.IsString())
					{
						vm.VNCAddress = string(value.GetString(), value.GetStringLength());
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kVNCAddress], invalid_object_);
						valid = false;
					}
					break;
				case kVNCPort:
					if (value.IsUint())
					{
						if (value.GetUint() < std::numeric_limits<uint16_t>::max())
						{
							vm.VNCPort = value.GetUint();
						}
						else
						{
							WriteJSONObject(writer, vm_settings_[kVNCPort], "Port too large");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kVNCPort], invalid_object_);
						valid = false;
					}
					break;
				case kQMPSocketType:
					if (value.IsString() && value.GetStringLength())
					{
						std::string str(value.GetString(), value.GetStringLength());
						if (str == "tcp")
							vm.QMPSocketType = VMSettings::SocketType::kTCP;
						else if (str == "local")
							vm.QMPSocketType = VMSettings::SocketType::kLocal;
						else
						{
							WriteJSONObject(writer, vm_settings_[kQMPSocketType], "Invalid socket type");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kQMPSocketType], invalid_object_);
						valid = false;
					}
					break;
				case kQMPAddress:
					if (value.IsString())
						vm.QMPAddress = string(value.GetString(), value.GetStringLength());
					else
					{
						WriteJSONObject(writer, vm_settings_[kQMPAddress], invalid_object_);
						valid = false;
					}
					break;
				case kQMPPort:
					if (value.IsUint())
					{
						if (value.GetUint() < std::numeric_limits<uint16_t>::max())
						{
							vm.QMPPort = value.GetUint();
						}
						else
						{
							WriteJSONObject(writer, vm_settings_[kQMPPort], "Port too large");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kQMPPort], invalid_object_);
						valid = false;
					}
					break;
				case kQEMUCmd:
					if (value.IsString())
					{
						vm.QEMUCmd = string(value.GetString(), value.GetStringLength());
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kQEMUCmd], invalid_object_);
						valid = false;
					}
					break;
				case kQEMUSnapshotMode:
					if (value.IsString())
					{
						std::string mode = std::string(value.GetString(), value.GetStringLength());
						for (size_t x = 0; x < sizeof(snapshot_modes_) / sizeof(std::string); x++)
						{
							if (mode == snapshot_modes_[x])
							{
								vm.QEMUSnapshotMode = (VMSettings::SnapshotMode)x;
								goto found_mode;
							}
						}
						WriteJSONObject(writer, vm_settings_[kQEMUSnapshotMode], "Unknown snapshot mode");
						valid = false;
					found_mode:
						break;
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kQEMUSnapshotMode], invalid_object_);
						valid = false;
					}
					break;
				case kTurnsEnabled:
					if (value.IsBool())
					{
						vm.TurnsEnabled = value.GetBool();
						// TODO
						//if (!config.TurnsEnabled)
						//{
						//	current_turn_ = nullptr;
						//	turn_queue_.clear();
						//	boost::system::error_code ec;
						//	turn_timer_.cancel(ec);
						//	BroadcastTurnInfo();
						//}
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kTurnsEnabled], invalid_object_);
						valid = false;
					}
					break;
				case kTurnTime:
					if (value.IsUint())
					{
						vm.TurnTime = value.GetUint();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kTurnTime], invalid_object_);
						valid = false;
					}
					break;
				case kVotesEnabled:
					if (value.IsBool())
					{
						vm.VotesEnabled = value.GetBool();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kVotesEnabled], invalid_object_);
						valid = false;
					}
					break;
				case kVoteTime:
					if (value.IsUint())
					{
						vm.VoteTime = value.GetUint();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kVoteTime], invalid_object_);
						valid = false;
					}
					break;
				case kVoteCooldownTime:
					if (value.IsUint())
					{
						vm.VoteCooldownTime = value.GetUint();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kVoteCooldownTime], invalid_object_);
						valid = false;
					}
					break;
				case kAgentEnabled:
					if (value.IsBool())
					{
						vm.AgentEnabled = value.GetBool();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kAgentEnabled], invalid_object_);
						valid = false;
					}
					break;
				case kAgentSocketType:
					if (value.IsString() && value.GetStringLength())
					{
						std::string str(value.GetString(), value.GetStringLength());
						if (str == "tcp")
							vm.AgentSocketType = VMSettings::SocketType::kTCP;
						else if (str == "local")
							vm.AgentSocketType = VMSettings::SocketType::kLocal;
						else
						{
							WriteJSONObject(writer, vm_settings_[kAgentSocketType], "Invalid socket type");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kAgentSocketType], invalid_object_);
						valid = false;
					}
					break;
				case kAgentUseVirtio:
					if (value.IsBool())
					{
						vm.AgentUseVirtio = value.GetBool();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kAgentUseVirtio], invalid_object_);
						valid = false;
					}
					break;
				case kAgentAddress:
					if (value.IsString())
					{
						vm.AgentAddress = string(value.GetString(), value.GetStringLength());
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kAgentAddress], invalid_object_);
						valid = false;
					}
					break;
				case kAgentPort:
					if (value.IsUint())
					{
						if (value.GetUint() < std::numeric_limits<uint16_t>::max())
						{
							vm.AgentPort = value.GetUint();
						}
						else
						{
							WriteJSONObject(writer, vm_settings_[kAgentPort], "Port too large");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, vm_settings_[kAgentPort], invalid_object_);
						valid = false;
					}
					break;
				case kRestoreHeartbeat:
					if (value.IsBool())
					{
						vm.RestoreHeartbeat = value.GetBool();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kRestoreHeartbeat], invalid_object_);
						valid = false;
					}
					break;
				case kHeartbeatTimeout:
					if (value.IsUint())
					{
						vm.HeartbeatTimeout = value.GetUint();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kHeartbeatTimeout], invalid_object_);
						valid = false;
					}
					break;
				case kUploadsEnabled:
					if (value.IsBool())
					{
						vm.UploadsEnabled = value.GetBool();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kUploadsEnabled], invalid_object_);
						valid = false;
					}
					break;
				case kUploadCooldownTime:
					if (value.IsUint())
					{
						vm.UploadCooldownTime = value.GetUint();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kUploadCooldownTime], invalid_object_);
						valid = false;
					}
					break;
				case kUploadMaxSize:
					if (value.IsUint())
					{
						vm.MaxUploadSize = value.GetUint();
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kUploadMaxSize], invalid_object_);
						valid = false;
					}
					break;
				case kUploadMaxFilename:
					if (value.IsUint())
					{
						if (value.GetUint() < std::numeric_limits<uint8_t>::max())
						{
							vm.UploadMaxFilename = value.GetUint();
						}
						else
						{
							WriteJSONObject(writer, vm_settings_[kUploadMaxFilename], "Max filename is too long");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kUploadMaxFilename], invalid_object_);
						valid = false;
					}
					break;
				}
				break;
			}
		}
	}
	if (valid)
	{
		// Set the value of the "result" property to true to indicate success
		writer.Bool(true);
	}
	return valid;
}

std::string CollabVMServer::PerformConfigFunction(const std::string& json)
{
	// Start the JSON response
	rapidjson::StringBuffer str_buf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(str_buf);
	// Start the root object
	writer.StartObject();
	writer.String("result");
	if (json.empty())
	{
		writer.String("Request was empty");
		writer.EndObject();
		return std::string(str_buf.GetString(), str_buf.GetSize());
	}
	// Copy the data to a writable buffer
	size_t len = json.length() + 1;
	char* buf = new char[len];
	memcpy(buf, json.c_str(), len);
	// Parse the JSON
	Document d;
	d.ParseInsitu(buf);
	if (!d.IsObject())
	{
		writer.String("Root value was not an object");
		writer.EndObject();
		return string(str_buf.GetString(), str_buf.GetSize());
	}

	for (auto it = d.MemberBegin(); it != d.MemberEnd(); it++)
	{
		for (size_t i = 0; i < sizeof(config_functions_)/sizeof(std::string); i++)
		{
			std::string key = std::string(it->name.GetString(), it->name.GetStringLength());
			if (key == config_functions_[i])
			{
				rapidjson::Value& value = it->value;
				switch (i)
				{
				case kSettings:
					ParseServerSettings(value, writer);
					break;
				case kPassword:
					if (value.IsString())
					{
						database_.Configuration.MasterPassword = std::string(value.GetString(), value.GetStringLength());
						database_.Save(database_.Configuration);
						writer.Bool(true);
					}
					else
					{
						WriteJSONObject(writer, config_functions_[kPassword], invalid_object_);
					}
					break;
				case kAddVM:
				{
					if (!value.IsObject())
					{
						WriteJSONObject(writer, config_functions_[kAddVM], invalid_object_);
						break;
					}
					// Get the name of the VM to update
					auto name_it = value.FindMember("name");
					if (name_it == value.MemberEnd())
					{
						WriteJSONObject(writer, config_functions_[kUpdateVM], "No VM name");
						break;
					}

					if (!name_it->value.IsString())
					{
						WriteJSONObject(writer, config_functions_[kUpdateVM], invalid_object_);
						break;
					}

					if (!name_it->value.GetStringLength())
					{
						WriteJSONObject(writer, config_functions_[kUpdateVM], "VM name cannot be empty");
						break;
					}

					// Check if the VM name is already taken
					auto vm_it = database_.VirtualMachines.find(std::string(name_it->value.GetString(), name_it->value.GetStringLength()));
					if (vm_it == database_.VirtualMachines.end())
					{
						VMSettings* vm = new VMSettings();
						if (ParseVMSettings(*vm, it->value, writer))
						{
							std::shared_ptr<VMSettings> vm_ptr(vm);
							database_.AddVM(vm_ptr);
							if (vm->AutoStart)
							{
								CreateVMController(vm_ptr)->Start();
							}
							WriteServerSettings(writer);
							break;
						}
						delete vm;
					}
					else
					{
						writer.String("A VM with that name already exists");
					}
					break;
				}
				case kUpdateVM:
				{
					if (!value.IsObject())
					{
						WriteJSONObject(writer, config_functions_[kUpdateVM], invalid_object_);
						break;
					}
					// Get the name of the VM to update
					auto name_it = value.FindMember("name");
					if (name_it == value.MemberEnd())
					{
						WriteJSONObject(writer, config_functions_[kUpdateVM], "No VM name");
						break;
					}

					if (!name_it->value.IsString())
					{
						WriteJSONObject(writer, config_functions_[kUpdateVM], invalid_object_);
						break;
					}
					// Find the VM with the corresponding name
					auto vm_it = database_.VirtualMachines.find(std::string(name_it->value.GetString(), name_it->value.GetStringLength()));
					if (vm_it == database_.VirtualMachines.end())
					{
						WriteJSONObject(writer, config_functions_[kUpdateVM], "Unknown VM name");
						break;
					}

					std::shared_ptr<VMSettings> vm = std::make_shared<VMSettings>(*vm_it->second);
					if (ParseVMSettings(*vm, it->value, writer))
					{
						database_.UpdateVM(vm);
						vm_it->second = vm;

						auto vm_it = vm_controllers_.find(vm->Name);
						if (vm_it != vm_controllers_.end())
							vm_it->second->ChangeSettings(vm);

						WriteServerSettings(writer);
					}
					break;
				}
				case kDeleteVM:
				{
					if (!value.IsString())
					{
						WriteJSONObject(writer, config_functions_[kDeleteVM], invalid_object_);
						break;
					}
					std::string vm_name(value.GetString(), value.GetStringLength());

					// Find the VM with the corresponding name
					auto vm_it = database_.VirtualMachines.find(vm_name);
					if (vm_it == database_.VirtualMachines.end())
					{
						// VM with ID not found
						writer.String("VM with ID not found");
						break;
					}
					std::map<std::string, std::shared_ptr<VMController>>::iterator vm_ctrl_it;
					if ((vm_ctrl_it = vm_controllers_.find(vm_name)) != vm_controllers_.end())
					{
						vm_ctrl_it->second->Stop(VMController::StopReason::kRemove);
					}

					database_.RemoveVM(vm_it->first);

					writer.Bool(true);

					WriteServerSettings(writer);
					break;
				}
				default:
					writer.String("Unsupported operation");
				}
				break;
			}
		}
	}

	// End the root object
	writer.EndObject();

	return std::string(str_buf.GetString(), str_buf.GetSize());
}

std::string CollabVMServer::GetServerSettings()
{
	rapidjson::StringBuffer str_buf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(str_buf);
	writer.StartObject();
	WriteServerSettings(writer);
	writer.EndObject();
	return std::string(str_buf.GetString(), str_buf.GetSize());
}

void CollabVMServer::WriteServerSettings(rapidjson::Writer<rapidjson::StringBuffer>& writer)
{
	writer.String("settings");
	writer.StartObject();

	writer.String(server_settings_[kChatRateCount].c_str());
	writer.Uint(database_.Configuration.ChatRateCount);

	writer.String(server_settings_[kChatRateTime].c_str());
	writer.Uint(database_.Configuration.ChatRateTime);

	writer.String(server_settings_[kChatMuteTime].c_str());
	writer.Uint(database_.Configuration.ChatMuteTime);

	writer.String(server_settings_[kMaxConnections].c_str());
	writer.Uint(database_.Configuration.MaxConnections);

	writer.String(server_settings_[kMaxUploadTime].c_str());
	writer.Uint(database_.Configuration.MaxUploadTime);

	// "vm" is an array of objects containing the settings for each VM
	writer.String("vm");
	writer.StartArray();

	for (auto it = database_.VirtualMachines.begin(); it != database_.VirtualMachines.end(); it++)
	{
		std::shared_ptr<VMSettings>& vm = it->second;
		writer.StartObject();
		writer.String("name");
		writer.String(vm->Name.c_str());
		for (size_t i = 0; i < sizeof(vm_settings_)/sizeof(std::string); i++)
		{
			switch (i)
			{
			case kAutoStart:
				writer.String(vm_settings_[kAutoStart].c_str());
				writer.Bool(vm->AutoStart);
				break;
			case kDisplayName:
				writer.String(vm_settings_[kDisplayName].c_str());
				writer.String(vm->DisplayName.c_str());
				break;
			case kHypervisor:
				writer.String(vm_settings_[kHypervisor].c_str());
				writer.String(hypervisor_names_[static_cast<size_t>(vm->Hypervisor)].c_str());
				break;
			case kStatus:
			{
				writer.String(vm_settings_[kStatus].c_str());
				auto vm_it = vm_controllers_.find(vm->Name);
				if (vm_it != vm_controllers_.end())
				{
					writer.Uint(static_cast<uint32_t>(vm_it->second->GetState()));
					break;
				}
				writer.Uint(static_cast<uint32_t>(VMController::ControllerState::kStopped));
				break;
			}
			case kRestoreShutdown:
				writer.String(vm_settings_[kRestoreShutdown].c_str());
				writer.Bool(vm->RestoreOnShutdown);
				break;
			case kRestoreTimeout:
				writer.String(vm_settings_[kRestoreTimeout].c_str());
				writer.Bool(vm->RestoreOnTimeout);
				break;
			case kVNCAddress:
				writer.String(vm_settings_[kVNCAddress].c_str());
				writer.String(vm->VNCAddress.c_str());
				break;
			case kVNCPort:
				writer.String(vm_settings_[kVNCPort].c_str());
				writer.Uint(vm->VNCPort);
				break;
			case kQMPSocketType:
				writer.String(vm_settings_[kQMPSocketType].c_str());
				writer.String(vm->QMPSocketType == VMSettings::SocketType::kLocal ? "local" : "tcp");
				break;
			case kQMPAddress:
				writer.String(vm_settings_[kQMPAddress].c_str());
				writer.String(vm->QMPAddress.c_str());
				break;
			case kQMPPort:
				writer.String(vm_settings_[kQMPPort].c_str());
				writer.Uint(vm->QMPPort);
				break;
			case kQEMUCmd:
				writer.String(vm_settings_[kQEMUCmd].c_str());
				writer.String(vm->QEMUCmd.c_str());
				break;
			case kQEMUSnapshotMode:
				writer.String(vm_settings_[kQEMUSnapshotMode].c_str());
				writer.String(snapshot_modes_[static_cast<size_t>(vm->QEMUSnapshotMode)].c_str());
				break;
			case kTurnsEnabled:
				writer.String(vm_settings_[kTurnsEnabled].c_str());
				writer.Bool(vm->TurnsEnabled);
				break;
			case kTurnTime:
				writer.String(vm_settings_[kTurnTime].c_str());
				writer.Uint(vm->TurnTime);
				break;
			case kVotesEnabled:
				writer.String(vm_settings_[kVotesEnabled].c_str());
				writer.Bool(vm->VotesEnabled);
				break;
			case kVoteTime:
				writer.String(vm_settings_[kVoteTime].c_str());
				writer.Uint(vm->VoteTime);
				break;
			case kVoteCooldownTime:
				writer.String(vm_settings_[kVoteCooldownTime].c_str());
				writer.Uint(vm->VoteCooldownTime);
				break;
			case kAgentEnabled:
				writer.String(vm_settings_[kAgentEnabled].c_str());
				writer.Bool(vm->AgentEnabled);
				break;
			case kAgentSocketType:
				writer.String(vm_settings_[kAgentSocketType].c_str());
				writer.String(vm->AgentSocketType == VMSettings::SocketType::kLocal ? "local" : "tcp");
				break;
			case kAgentUseVirtio:
				writer.String(vm_settings_[kAgentUseVirtio].c_str());
				writer.Bool(vm->AgentUseVirtio);
				break;
			case kAgentAddress:
				writer.String(vm_settings_[kAgentAddress].c_str());
				writer.String(vm->AgentAddress.c_str());
				break;
			case kAgentPort:
				writer.String(vm_settings_[kAgentPort].c_str());
				writer.Uint(vm->AgentPort);
				break;
			case kRestoreHeartbeat:
				writer.String(vm_settings_[kRestoreHeartbeat].c_str());
				writer.Bool(vm->RestoreHeartbeat);
				break;
			case kHeartbeatTimeout:
				writer.String(vm_settings_[kHeartbeatTimeout].c_str());
				writer.Uint(vm->HeartbeatTimeout);
				break;
			case kUploadsEnabled:
				writer.String(vm_settings_[kUploadsEnabled].c_str());
				writer.Bool(vm->UploadsEnabled);
				break;
			case kUploadCooldownTime:
				writer.String(vm_settings_[kUploadCooldownTime].c_str());
				writer.Uint(vm->UploadCooldownTime);
				break;
			case kUploadMaxSize:
				writer.String(vm_settings_[kUploadMaxSize].c_str());
				writer.Uint(vm->MaxUploadSize);
				break;
			case kUploadMaxFilename:
				writer.String(vm_settings_[kUploadMaxFilename].c_str());
				writer.Uint(vm->UploadMaxFilename);
				break;
			}
		}
		writer.EndObject();
	}
	// End array of VM settings
	writer.EndArray();
	// End "settings" object
	writer.EndObject();
}

void CollabVMServer::ParseServerSettings(rapidjson::Value& settings, rapidjson::Writer<rapidjson::StringBuffer>& writer)
{
	// Copy the current config to a temporary one so that
	// changes are not made if a setting is invalid
	Config config = database_.Configuration;
	bool valid = true;
	for (auto it = settings.MemberBegin(); it != settings.MemberEnd(); it++)
	{
		for (size_t i = 0; i < sizeof(server_settings_)/sizeof(std::string); i++)
		{
			std::string key = std::string(it->name.GetString(), it->name.GetStringLength());
			if (key == server_settings_[i])
			{
				rapidjson::Value& value = it->value;
				switch (i)
				{
				case kChatRateCount:
					if (value.IsUint())
					{
						if (value.GetUint() <= std::numeric_limits<uint8_t>::max())
							config.ChatRateCount = value.GetUint();
						else
						{
							WriteJSONObject(writer, server_settings_[kChatRateCount], "Value too big");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kChatRateCount], invalid_object_);
						valid = false;
					}
					break;
				case kChatRateTime:
					if (value.IsUint())
					{
						if (value.GetUint() <= std::numeric_limits<uint8_t>::max())
							config.ChatRateTime = value.GetUint();
						else
						{
							WriteJSONObject(writer, server_settings_[kChatRateTime], "Value too big");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kChatRateTime], invalid_object_);
						valid = false;
					}
					break;
				case kChatMuteTime:
					if (value.IsUint())
					{
						if (value.GetUint() <= std::numeric_limits<uint8_t>::max())
							config.ChatMuteTime = value.GetUint();
						else
						{
							WriteJSONObject(writer, server_settings_[kChatMuteTime], "Value too big");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kChatMuteTime], invalid_object_);
						valid = false;
					}
					break;
				case kMaxConnections:
					if (value.IsUint())
					{
						if (value.GetUint() <= std::numeric_limits<uint8_t>::max())
						{
							config.MaxConnections = value.GetUint();
						}
						else
						{
							WriteJSONObject(writer, server_settings_[kMaxConnections], "Value too big");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kMaxConnections], invalid_object_);
						valid = false;
					}
					break;
				case kMaxUploadTime:
					if (value.IsUint())
					{
						if (value.GetUint() <= std::numeric_limits<uint16_t>::max())
						{
							config.MaxUploadTime = value.GetUint();
						}
						else
						{
							WriteJSONObject(writer, server_settings_[kMaxUploadTime], "Value too big");
							valid = false;
						}
					}
					else
					{
						WriteJSONObject(writer, server_settings_[kMaxUploadTime], invalid_object_);
						valid = false;
					}
					break;
				}
				break;
			}
		}
	}

	// Only save the configuration if all settings valid
	if (valid)
	{
		database_.Save(config);

		// Set the value of the "result" property to true to indicate success
		writer.Bool(true);
		// Append the updated settings to the JSON object
		WriteServerSettings(writer);

		std::cout << "Settings were updated" << std::endl;
	}
	else
	{
		std::cout << "Failed to update settings" << std::endl;
	}
}
