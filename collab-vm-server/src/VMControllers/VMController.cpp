#include "VMController.h"
#include "CollabVM.h"
#include "Database/VMSettings.h"

VMController::VMController(CollabVMServer& server, asio::io_service& service, const std::shared_ptr<VMSettings>& settings) :
	server_(server),
	io_service_(service),
	settings_(settings),
	turn_timer_(service),
	vote_state_(VoteState::kIdle),
	vote_count_yes_(0),
	vote_count_no_(0),
	vote_timer_(service),
	current_turn_(nullptr),
	connected_users_(0),
	stop_reason_(StopReason::kNormal),
	thumbnail_str_(nullptr),
	agent_timer_(service),
	agent_connected_(false)
{
}

void VMController::InitAgent(const VMSettings& settings, asio::io_service& service)
{
	if (settings.AgentEnabled)
	{
		if (settings_->AgentSocketType == VMSettings::SocketType::kTCP)
		{
			agent_address_ = settings_->AgentAddress;
			// Port number doesn't need to be appended because
			// agent_address_ isn't used by QEMUController for TCP
			//std::string port = std::to_string(settings_->AgentPort);
			//agent_address_.reserve(agent_address_.length() + 1 + port.length());
			//agent_address_ += ':';
			//agent_address_ += port;
			agent_ = std::make_shared<AgentTCPClient>(service, settings_->AgentAddress, settings_->AgentPort);
		}
		else if (settings_->AgentSocketType == VMSettings::SocketType::kLocal)
		{
#ifdef _WIN32
			agent_address_ = R"(\\.\pipe\collab-vm-agent-)";
#else
			// Unix domain sockets need to have a valid file path
			agent_address_ = P_tmpdir "/collab-vm-agent-";
#endif
			agent_address_ += settings.Name;
			agent_ = std::make_shared<AgentLocalClient>(service, agent_address_);
		}

#ifndef _WIN32
		agent_->Init("collab-vm-agent.dll", /*settings.HeartbeatTimeout*/ 5);
#else
#if _DEBUG
		agent_->Init(R"(..\..\collab-vm-agent\Debug\collab-vm-agent.dll)", /*settings.HeartbeatTimeout*/ 5);
#else

#endif
#endif
	}
	else if (agent_)
	{
		agent_.reset();
	}
}

void VMController::ChangeSettings(const std::shared_ptr<VMSettings>& settings)
{
	if (settings->TurnsEnabled != settings_->TurnsEnabled ||
		settings->VotesEnabled != settings_->VotesEnabled ||
		settings->UploadsEnabled != settings_->UploadsEnabled)
	{
		server_.ActionsChanged(*this, *settings);
	}
}

void VMController::Stop(StopReason reason)
{
	asio::error_code ec;
	turn_timer_.cancel(ec);
	vote_timer_.cancel(ec);
	agent_timer_.cancel(ec);

	if (thumbnail_str_)
	{
		delete thumbnail_str_;
		thumbnail_str_ = nullptr;
	}

	server_.OnVMControllerStateChange(shared_from_this(), VMController::ControllerState::kStopping);
}

void VMController::Vote(CollabVMUser& user, bool vote)
{
	if (settings_->VotesEnabled)
	{
		switch (vote_state_)
		{
		case VoteState::kCoolingdown:
		{
			int32_t time_remaining = std::chrono::duration_cast<std::chrono::duration<int32_t>>(vote_timer_.expires_from_now()).count();
			if (time_remaining > 0)
			{
				server_.VoteCoolingDown(user, time_remaining);
				//break;
			}
			// Could fall through when the timer has expired,
			// although this may create a race condition with the
			// timer's callback
			break;
		}
		case VoteState::kIdle:
			if (vote)
			{
				// Start a new vote
				vote_state_ = VoteState::kVoting;
				vote_count_yes_ = 1;
				vote_count_no_ = 0;
				asio::error_code ec;
				vote_timer_.expires_from_now(std::chrono::seconds(settings_->VoteTime), ec);
				vote_timer_.async_wait(std::bind(&VMController::VoteEndedCallback, shared_from_this(), std::placeholders::_1));
				server_.UserStartedVote(*this, users_, user);
				server_.BroadcastVoteInfo(*this, users_, true, std::chrono::duration_cast<millisecs_t>(vote_timer_.expires_from_now()).count(), vote_count_yes_, vote_count_no_);
			}
			break;
		case VoteState::kVoting:
		{
			int32_t time_remaining = std::chrono::duration_cast<millisecs_t>(vote_timer_.expires_from_now()).count();
			if (time_remaining > 0)
			{
				IPData::VoteDecision prev_vote = user.ip_data.votes[this];
				bool changed = false;
				// A vote is already in progress so count the user's vote
				if (vote && prev_vote != IPData::VoteDecision::kYes)
				{
					if (prev_vote == IPData::VoteDecision::kYes)
						vote_count_no_--;

					vote_count_yes_++;
					changed = true;
				}
				else if (!vote && prev_vote != IPData::VoteDecision::kNo)
				{
					if (prev_vote == IPData::VoteDecision::kYes)
						vote_count_yes_--;

					vote_count_no_++;
					changed = true;
				}

				if (changed)
				{
					server_.UserVoted(*this, users_, user, vote);
					server_.BroadcastVoteInfo(*this, users_, false, time_remaining, vote_count_yes_, vote_count_no_);
				}
			}
			break;
		}
		}
	}
}

void VMController::VoteEndedCallback(const asio::error_code& ec)
{
	if (ec)
		return;

	server_.OnVMControllerVoteEnded(shared_from_this());
}

void VMController::EndVote()
{
	bool vote_succeeded = vote_count_yes_ >= vote_count_no_;
	server_.BroadcastVoteEnded(*this, users_, vote_succeeded);
	if (settings_->VoteCooldownTime)
	{
		vote_state_ = VoteState::kCoolingdown;
		asio::error_code ec;
		vote_timer_.expires_from_now(std::chrono::seconds(settings_->VoteCooldownTime), ec);
		std::shared_ptr<VMController> self = shared_from_this();
		vote_timer_.async_wait([this, self](const asio::error_code& ec)
		{
			if (!ec)
				vote_state_ = VoteState::kIdle;
		});
	}
	else
	{
		vote_state_ = VoteState::kIdle;
	}

	if (vote_succeeded)
		RestoreVMSnapshot();
}

void VMController::TurnRequest(const std::shared_ptr<CollabVMUser>& user)
{
	// If the user is already in the queue or they are already
	// in control don't allow them to make another turn request
	if (!settings_->TurnsEnabled || GetState() != ControllerState::kRunning ||
		current_turn_ == user || user->waiting_turn)
		return;

	if (!current_turn_)
	{
		// If no one currently has a turn then give the requesting user control
		current_turn_ = user;
		// Start the turn timer
		asio::error_code ec;
		turn_timer_.expires_from_now(std::chrono::seconds(settings_->TurnTime), ec);
		turn_timer_.async_wait(std::bind(&VMController::TurnTimerCallback, shared_from_this(), std::placeholders::_1));
	}
	else
	{
		// Otherwise add them to the queue
		turn_queue_.push_back(user);
		user->waiting_turn = true;
	}

	server_.BroadcastTurnInfo(*this, users_, turn_queue_, current_turn_.get(),
		std::chrono::duration_cast<millisecs_t>(turn_timer_.expires_from_now()).count());
}

void VMController::NextTurn()
{
	int32_t time_remaining;
	if (!turn_queue_.empty())
	{
		current_turn_ = turn_queue_.front();
		current_turn_->waiting_turn = false;
		turn_queue_.pop_front();

		// Set up the turn timer
		asio::error_code ec;
		turn_timer_.expires_from_now(std::chrono::seconds(settings_->TurnTime), ec);
		turn_timer_.async_wait(std::bind(&VMController::TurnTimerCallback, shared_from_this(), std::placeholders::_1));
		time_remaining = std::chrono::duration_cast<millisecs_t>(turn_timer_.expires_from_now()).count();
	}
	else
	{
		current_turn_ = nullptr;
		time_remaining = 0;
	}

	server_.BroadcastTurnInfo(*this, users_, turn_queue_, current_turn_.get(), time_remaining);
}

void VMController::ClearTurnQueue()
{
	auto it = turn_queue_.begin();
	while (it != turn_queue_.end())
	{
		(*it)->waiting_turn = false;
		it = turn_queue_.erase(it);
	}
	current_turn_ = nullptr;
	server_.BroadcastTurnInfo(*this, users_, turn_queue_, current_turn_.get(), 0);
}

void VMController::TurnTimerCallback(const asio::error_code& ec)
{
	if (ec)
		return;

	server_.OnVMControllerTurnChange(shared_from_this());
}


void VMController::OnAgentConnect(const std::string& os_name, const std::string& service_pack,
								  const std::string& pc_name, const std::string& username, uint32_t max_filename)
{
	server_.OnAgentConnect(shared_from_this(), os_name, service_pack, pc_name, username, max_filename);
}

void VMController::OnAgentDisconnect(bool protocol_error)
{
	server_.OnAgentDisconnect(shared_from_this());
}

void VMController::OnAgentHeartbeatTimeout()
{
	if (settings_->RestoreHeartbeat)
		RestoreVMSnapshot();
	else
		agent_->Disconnect();
	//server_.OnAgentHeartbeatTimeout(shared_from_this());
}

void VMController::OnFileUploadStarted(const std::shared_ptr<UploadInfo>& info, std::string* filename)
{
	// TODO: Report new filename
}

void VMController::OnFileUploadFailed(const std::shared_ptr<UploadInfo>& info)
{
	server_.OnFileUploadFailed(shared_from_this(), info);
}

void VMController::OnFileUploadFinished(const std::shared_ptr<UploadInfo>& info)
{
	server_.OnFileUploadFinished(shared_from_this(), info);
}

void VMController::OnFileUploadExecFinished(const std::shared_ptr<UploadInfo>& info, bool exec_success)
{
	server_.OnFileUploadExecFinished(shared_from_this(), info, exec_success);
}

void VMController::AddUser(const std::shared_ptr<CollabVMUser>& user)
{
	users_.AddUser(*user, [this](CollabVMUser& user) { OnAddUser(user); });

	int32_t time_remaining;
	if (current_turn_)
	{
		time_remaining = std::chrono::duration_cast<millisecs_t>(turn_timer_.expires_from_now()).count();
		if (time_remaining > 0)
			server_.SendTurnInfo(*user, time_remaining, *current_turn_->username, turn_queue_);
	}

	if (vote_state_ == VoteState::kVoting)
	{
		time_remaining = std::chrono::duration_cast<millisecs_t>(vote_timer_.expires_from_now()).count();
		if (time_remaining > 0)
			server_.SendVoteInfo(*this, *user, time_remaining, vote_count_yes_, vote_count_no_);
	}
}

void VMController::RemoveUser(const std::shared_ptr<CollabVMUser>& user)
{
	// Remove the user's vote
	IPData::VoteDecision prev_vote = user->ip_data.votes[this];
	int32_t time_remaining = std::chrono::duration_cast<millisecs_t>(vote_timer_.expires_from_now()).count();
	if (vote_state_ == VoteState::kVoting && time_remaining > 0 && prev_vote != IPData::VoteDecision::kNotVoted)
	{
		if (prev_vote == IPData::VoteDecision::kYes)
			vote_count_yes_--;
		else if (prev_vote == IPData::VoteDecision::kNo)
			vote_count_no_--;

		user->ip_data.votes[this] = IPData::VoteDecision::kNotVoted;
		server_.BroadcastVoteInfo(*this, users_, false, time_remaining, vote_count_yes_, vote_count_no_);
	}

	bool turn_change = false;
	// Check if they are in the turn queue and remove them if they are
	if (user->waiting_turn)
	{
		for (auto it = turn_queue_.begin(); it != turn_queue_.end(); it++)
		{
			if (*it == user)
			{
				turn_change = true;
				turn_queue_.erase(it);
				// The client should not be in the queue more than once
				break;
			}
		}
	}

	// Check if it is currently their turn
	if (current_turn_ == user)
	{
		// Cancel the pending timer callback
		asio::error_code ec;
		turn_timer_.cancel(ec);

		if (!turn_queue_.empty())
		{
			current_turn_ = turn_queue_.front();
			current_turn_->waiting_turn = false;
			turn_queue_.pop_front();

			// Set up the turn timer
			turn_timer_.expires_from_now(std::chrono::seconds(settings_->TurnTime), ec);
			turn_timer_.async_wait(std::bind(&VMController::TurnTimerCallback, shared_from_this(), std::placeholders::_1));
		}
		else
			current_turn_ = nullptr;

		turn_change = true;
	}

	if (turn_change)
		server_.BroadcastTurnInfo(*this, users_, turn_queue_, current_turn_.get(),
			current_turn_ ? std::chrono::duration_cast<millisecs_t>(turn_timer_.expires_from_now()).count() : 0);

	users_.RemoveUser(*user, [this](CollabVMUser& user) { OnRemoveUser(user); });
}

void VMController::NewThumbnail(std::string* str)
{
	server_.OnVMControllerThumbnailUpdate(shared_from_this(), str);
}

bool VMController::IsFileUploadValid(const std::shared_ptr<CollabVMUser>& user, const std::string& filename, size_t file_size, bool run_file)
{
	return file_size >= 1 && file_size <= settings_->MaxUploadSize &&
		AgentClient::IsFilenameValid(agent_max_filename_, filename);
}

VMController::~VMController()
{
}
