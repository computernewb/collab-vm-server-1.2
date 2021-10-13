#include "QMPClient.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "rapidjson/writer.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"

using rapidjson::StringBuffer;
using rapidjson::Writer;
using rapidjson::Document;
using rapidjson::Value;

void QMPClient::Connect(std::weak_ptr<QMPCallback> controller) {
	auto self = shared_from_this();
	GetService().dispatch([this, self, controller]() {
		if(state_ == ConnectionState::kDisconnected) {
			state_ = ConnectionState::kConnecting;
			controller_ = controller;
			ConnectSocket();
		}
	});
}

void QMPClient::OnConnect(std::shared_ptr<SocketCtx>& ctx) {
	if(state_ == ConnectionState::kConnecting) {
		state_ = ConnectionState::kCapabilities;
		StartTimeoutTimer();
		DoReadLine(ctx);
	}
}

void QMPClient::OnDisconnect() {
	if(state_ != ConnectionState::kDisconnected) {
		state_ = ConnectionState::kDisconnected;

		boost::system::error_code error;
		timer_.cancel(error);

		if(auto ptr = controller_.lock())
			ptr->OnQMPStateChange(QMPState::kDisconnected);
	}
}

void QMPClient::Disconnect() {
	auto self = shared_from_this();
	GetService().dispatch([this, self]() {
		if(state_ != ConnectionState::kDisconnected)
			DisconnectSocket();
	});
}

void QMPClient::StartTimeoutTimer() {
	boost::system::error_code error;
	timer_.expires_from_now(kReadTimeout, error);
	auto self = shared_from_this();
	timer_.async_wait([this, self](const boost::system::error_code& ec) {
		if(!ec)
			Disconnect();
	});
}

void QMPClient::RegisterEventCallback(Events event, EventCallback callback) {
	auto self = shared_from_this();
	GetService().dispatch([this, self, event, callback]() {
		event_callbacks_[static_cast<size_t>(event)] = callback;
	});
}

void QMPClient::SendString(const std::string& str) {
	auto self = shared_from_this();
	GetService().dispatch([this, self, str]() {
		if(state_ != ConnectionState::kConnected)
			return;

		DoWriteData(str.c_str(), str.length(), GetSocketContext());
	});
}

void QMPClient::SystemPowerDown() {
	SendString("{\"execute\":\"system_powerdown\"}\r\n");
}

void QMPClient::SystemReset() {
	SendString("{\"execute\":\"system_reset\"}\r\n");
}

void QMPClient::SystemStop() {
	SendString("{\"execute\":\"stop\"}\r\n");
}

void QMPClient::SystemResume() {
	SendString("{\"execute\":\"cont\"}\r\n");
}

void QMPClient::QEMUQuit() {
	SendString("{\"execute\":\"quit\"}\r\n");
}

void QMPClient::SendMonitorCommand(const std::string& cmd, ResultCallback result_cb) {
	auto self = shared_from_this();
	GetService().dispatch([this, self, cmd, result_cb]() {
		if(state_ != ConnectionState::kConnected)
			return;
			// Use macro to provide the string length to
			// the function so it doesn't need to use strlen
#define STRING(str) writer.String(str, sizeof(str) - 1)

		StringBuffer s;
		Writer<StringBuffer> writer(s);
		writer.StartObject();
		STRING("execute");
		STRING("human-monitor-command");
		STRING("arguments");
		writer.StartObject();
		STRING("command-line");
		writer.String_bs(cmd);
		writer.EndObject();
		// If a callback was provided, assign an ID to the command
		if(result_cb) {
			STRING("id");
			writer.Uint(result_id_);
			writer.EndObject();

			result_callbacks_[result_id_] = result_cb;
			// Increment the ID and allow it to overflow
			// Overflowing should be fine as long as there are
			// no more than 2^16 callbacks already in the map
			result_id_++;
		} else {
			writer.EndObject();
		}

		DoWriteData(s.GetString(), s.GetSize(), GetSocketContext());
	});
}

void QMPClient::LoadSnapshot(const std::string& snapshot, ResultCallback result_cb) {
	SendMonitorCommand("loadvm " + snapshot, result_cb);
}

void QMPClient::OnReadLine(const boost::system::error_code& ec, size_t size, std::shared_ptr<SocketCtx>& ctx) {
	if(ctx->IsStopped())
		return;

	if(ec) {
		std::cout << "QMP read error: " << ec.message() << std::endl;
		DisconnectSocket();
		return;
	}

	// Copy the data from the streambuf into a string
	if(size > 2) {
		// Copy the data to a writable buffer
		char* buf = new char[size - 1];
		// Subtract two from the length to exclude the "\r\n"
		std::memcpy(buf, boost::asio::buffer_cast<const char*>(buf_.data()), size - 2);
		// Null terminate the buffer
		buf[size - 2] = '\0';
		// Parse it as JSON
		Document d;
		d.ParseInsitu(buf);
		Value::MemberIterator e;
		switch(state_) {
			case ConnectionState::kCapabilities: {
				boost::system::error_code error;
				timer_.cancel(error);

				// Read command:
				// {"QMP":
				//		{"version":
				//			{"qemu": {"micro": 0, "minor": 2, "major": 2},
				//			 "package": " (Debian 1:2.2+dfsg-5expubuntu9.2)"},
				//			  "capabilities": []
				//		}
				// }
				e = d.FindMember("QMP");
				if(e != d.MemberEnd() && e->value.IsObject()) {
					state_ = ConnectionState::kResponse;
					buf_.consume(size);

					//Value::MemberIterator ver = e->value.FindMember("version");
					//if (ver != e->value.MemberEnd() && ver->value.IsObject())
					//{
					//	//cout << "QEMU version: " << ver->value.GetString() << endl;
					//}
					// Respond with qmp_capabilities command
					// An ID could be included with this command but it's unnecessary
					// because no other commands will be received until after the
					// handshake so the next one must be the result
					const char cmd[] = "{\"execute\":\"qmp_capabilities\"}\r\n";
					DoWriteData(cmd, sizeof(cmd) - 1, ctx);
				} else {
					// Unexpected first command
					std::cout << "QMP invalid capabilities command: " << std::string(boost::asio::buffer_cast<const char*>(buf_.data()), size - 2) << std::endl;
					DisconnectSocket();
				}
				break;
			}
			case ConnectionState::kResponse: {
				boost::system::error_code error;
				timer_.cancel(error);

				// Read command:
				// { "return": {} }
				e = d.FindMember("return");
				if(e != d.MemberEnd() && e->value.IsObject()) {
					state_ = ConnectionState::kConnected;
					std::cout << "Connected to QEMU" << std::endl;

					if(auto ptr = controller_.lock())
						ptr->OnQMPStateChange(QMPState::kConnected);

					buf_.consume(size);
					DoReadLine(ctx);
				} else {
					// Unexpected response
					std::cout << "QMP invalid handshake response: " << std::string(boost::asio::buffer_cast<const char*>(buf_.data()), size - 2) << std::endl;
					DisconnectSocket();
				}
				break;
			}
			case ConnectionState::kConnected: {
				e = d.FindMember("event");
				if(e != d.MemberEnd() && e->value.IsString()) {
					Value& v = e->value;
					for(int i = 0; i < sizeof(events_) / sizeof(std::string); i++) {
						if(!events_[i].compare(0, std::string::npos, v.GetString(), v.GetStringLength())) {
							if(event_callbacks_[i])
								event_callbacks_[i](d);
							break;
						}
					}
				} else if((e = d.FindMember("return")) != d.MemberEnd()) {
					Value::MemberIterator v = d.FindMember("id");
					if(v != d.MemberEnd() && v->value.IsUint()) {
						auto it = result_callbacks_.find(v->value.GetUint());
						if(it != result_callbacks_.end()) {
							it->second(d);
							result_callbacks_.erase(it);
						}
					}
				}
				buf_.consume(size);
				DoReadLine(ctx);
				break;
				}
				case ConnectionState::kConnecting: {
					break; // What should this do?
				}
				case ConnectionState::kDisconnected: {
					break; // implode()? murder() death() kill()? i feel like this should restart the VM
				}
		}
		delete[] buf;
	}
}

void QMPClient::OnWrite(const boost::system::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx) {
	if(ctx->IsStopped())
		return;

	if(ec) {
		std::cout << "QMP write error: " << ec.message() << std::endl;
		DisconnectSocket();
	} else if(state_ == ConnectionState::kResponse) {
		DoReadLine(ctx);
	}
}
