#include "QMP.h"
//#include "QEMUController.h"
#include "rapidjson/writer.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include <boost/asio.hpp>
#include <functional>
#include <iostream>

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace rapidjson;

QMP::QMP(io_service& service, const std::string& host, uint16_t port) :
	host_(host),
	port_(std::to_string(port)),
	_resolver(service),
	socket_(service),
	stopped_(true),
	state_(ConnectionState::kConnecting),
	_resultId(0),
	timer_(service)
{
	socket_.open(tcp::v4());
	socket_.set_option(tcp::no_delay(true));
}

void QMP::SetEndpoint(std::string& host, uint16_t port)
{
	host_ = host;
	port_ = std::to_string(port);
}

void QMP::Connect(std::weak_ptr<QMPCallback> controller)
{
	std::lock_guard<std::mutex> lock(lock_);
	//if (!stopped_)
	//	return;
	stopped_ = false;
	controller_ = controller;

	tcp::resolver::query query(tcp::v4(), host_, port_);
	_resolver.async_resolve(query, bind(&QMP::OnResolve,
		shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void QMP::StartConnection(tcp::resolver::iterator endpoint_iter)
{
	if (stopped_)
		return;

	if (endpoint_iter != tcp::resolver::iterator())
	{
		std::cout << "Trying " << endpoint_iter->endpoint() << "...\n";

		boost::system::error_code ec;
		timer_.expires_from_now(std::chrono::seconds(10), ec);
		timer_.async_wait(bind(&QMP::TimerCallback, shared_from_this(), std::placeholders::_1));

		socket_.async_connect(endpoint_iter->endpoint(), std::bind(&QMP::OnConnect,
			shared_from_this(), std::placeholders::_1, endpoint_iter));
	}
	else
	{
		DisconnectReason(States::kConnectFailure);
	}
}

void QMP::Disconnect()
{
	DisconnectReason(States::kDisconnected);
}

void QMP::DisconnectReason(States state)
{
	std::lock_guard<std::mutex> lock(lock_);
	if (!stopped_)
	{
		stopped_ = true;
		state_ = ConnectionState::kConnecting;

		boost::system::error_code ec;
		socket_.shutdown(tcp::socket::shutdown_both, ec);
		socket_.close(ec);
		timer_.cancel(ec);

		if (auto ptr = controller_.lock())
			ptr->OnQMPStateChange(state);
	}
}

void QMP::TimerCallback(const boost::system::error_code& ec)
{
	if (ec || stopped_)
		return;

	boost::system::error_code err;
	socket_.close(err);
}

void QMP::RegisterEventCallback(Events event, EventCallback callback)
{
	_eventCallbacks[event] = callback;
}

void QMP::SystemPowerDown()
{
	if (stopped_ || state_ != ConnectionState::kFinished)
		return;

	const char cmd[] = "{\"execute\":\"system_powerdown\"}\r\n";
	WriteData(cmd, sizeof(cmd) - 1);
}

void QMP::SystemReset()
{
	if (stopped_ || state_ != ConnectionState::kFinished)
		return;

	const char cmd[] = "{\"execute\":\"system_reset\"}\r\n";
	WriteData(cmd, sizeof(cmd) - 1);
}

void QMP::SystemStop()
{
	if (stopped_ || state_ != ConnectionState::kFinished)
		return;

	const char cmd[] = "{\"execute\":\"stop\"}\r\n";
	WriteData(cmd, sizeof(cmd) - 1);
}

void QMP::SystemResume()
{
	if (stopped_ || state_ != ConnectionState::kFinished)
		return;

	const char cmd[] = "{\"execute\":\"cont\"}\r\n";
	WriteData(cmd, sizeof(cmd) - 1);
}

void QMP::QEMUQuit()
{
	if (stopped_ || state_ != ConnectionState::kFinished)
		return;

	const char cmd[] = "{\"execute\":\"quit\"}\r\n";
	WriteData(cmd, sizeof(cmd) - 1);
}

void QMP::SendMonitorCommand(string cmd, ResultCallback resultCb)
{
	if (stopped_ || state_ != ConnectionState::kFinished)
		return;
	// Use macro to provide the string length to
	// the function so it doesn't need to use strlen
#define STRING(str) writer.String(str, sizeof(str)-1)

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
	if (resultCb)
	{
		STRING("id");
		// TODO: Lock access to the map and ID
		writer.Uint(_resultId);
		writer.EndObject();

		_resultCallbacks[_resultId] = resultCb;
		// Increment the ID and allow it to overflow
		// Overflowing should be fine as long as there are
		// no more than 2^16 callbacks already in the map
		_resultId++;
	}
	else
	{
		writer.EndObject();
	}

	WriteData(s.GetString(), s.GetSize());
}

void QMP::LoadSnapshot(string snapshot, ResultCallback resultCb)
{
	SendMonitorCommand("loadvm " + snapshot, resultCb);
}

void QMP::OnResolve(const boost::system::error_code& ec, tcp::resolver::iterator iterator)
{
	if (stopped_)
		return;

	if (!ec)
	{
		StartConnection(iterator);
	}
	else
	{
		cout << "QMP OnResolve error: " << ec.message() << endl;
		Disconnect();
	}
}

void QMP::OnConnect(const boost::system::error_code& ec, tcp::resolver::iterator iterator)
{
	if (stopped_)
		return;

	// If the socket is closed it means that the connection timed-out
	// and the callback for the timer closed it
	if (!socket_.is_open())
	{
		std::cout << "QMP connection timed out" << std::endl;
		StartConnection(++iterator);
	}
	else if (ec)
	{
		std::cout << "QMP connection failed: " << ec.message() << endl;
		boost::system::error_code ec;
		socket_.close(ec);
		StartConnection(++iterator);
	}
	else
	{
		// Cancel connection timeout timer
		boost::system::error_code ec;
		timer_.cancel(ec);

		std::unique_lock<std::mutex> lock(lock_);
		if (stopped_)
			return;

		state_ = ConnectionState::kCapabilities;

		lock.unlock();

		// Wait to receive capabilities negotiation command
		ReadLine();
	}
}

void QMP::OnReadLine(const boost::system::error_code& ec, size_t size)
{
	if (stopped_)
		return;

	if (!ec)
	{
		// Copy the data from the streambuf into a string
		if (size > 2)
		{
			// Copy the data to a writable buffer
			char* buf = new char[size - 1];
			memcpy(buf, boost::asio::buffer_cast<const char*>(buf_.data()), size - 2);
			// Null terminate the buffer
			buf[size - 2] = '\0';
			//cout << buf << "\n" << endl;
			// Parse it as JSON
			Document d;
			d.ParseInsitu(buf);
			Value::MemberIterator e;
			switch (state_)
			{
			case kCapabilities:
				// Read command:
				// {"QMP":
				//		{"version":
				//			{"qemu": {"micro": 0, "minor": 2, "major": 2},
				//			 "package": " (Debian 1:2.2+dfsg-5expubuntu9.2)"},
				//			  "capabilities": []
				//		}
				// }
				e = d.FindMember("QMP");
				if (e != d.MemberEnd() && e->value.IsObject())
				{
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
					WriteData(cmd, sizeof(cmd) - 1);
					state_ = ConnectionState::kResponse;
				}
				break;
			case kResponse:
				// Read command:
				// { "return": {} }
				e = d.FindMember("return");
				if (e != d.MemberEnd() && e->value.IsObject())
				{
					state_ = ConnectionState::kFinished;
					cout << "Connected to QEMU" << endl;
					std::lock_guard<std::mutex> lock(lock_);
					if (stopped_)
						return;
					if (auto ptr = controller_.lock())
						ptr->OnQMPStateChange(States::kConnected);
				}
				else
				{
					// Invalid response
					cout << "QMP invalid handshake response: " <<
						std::string(boost::asio::buffer_cast<const char*>(buf_.data()), size - 2) << endl;
					Disconnect();
				}
				break;
			case kFinished:
				e = d.FindMember("event");
				if (e != d.MemberEnd() && e->value.IsString())
				{
					Value& v = e->value;
					for (int i = 0; i < sizeof(events_)/sizeof(std::string); i++)
					{
						if (!events_[i].compare(0, std::string::npos, v.GetString(), v.GetStringLength()))
						{
							if (_eventCallbacks[i])
								_eventCallbacks[i](d);
							break;
						}
					}
				}
				else if ((e = d.FindMember("return")) != d.MemberEnd())
				{
					Value::MemberIterator v = d.FindMember("id");
					if (v != d.MemberEnd() && v->value.IsUint())
					{
						map<uint16_t, ResultCallback>::iterator it = _resultCallbacks.find(v->value.GetUint());
						if (it != _resultCallbacks.end())
						{
							it->second(d);
							_resultCallbacks.erase(it);
						}
					}
				}
				break;
			}
		}
		buf_.consume(size);
		ReadLine();
	}
	else
	{
		cout << "QMP read error: " << ec.message() << endl;
		Disconnect();
	}
}

void QMP::OnWrite(const boost::system::error_code& ec, size_t size)
{
	if (stopped_)
		return;

	if (ec)
	{
		cout << "QMP write error: " << ec.message() << endl;
		Disconnect();
	}
}

void QMP::ReadLine()
{
	async_read_until(socket_, buf_, "\r\n",
		std::bind(&QMP::OnReadLine, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void QMP::WriteData(const char data[], size_t len)
{
	async_write(socket_, boost::asio::buffer(data, len),
		std::bind(&QMP::OnWrite, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}
