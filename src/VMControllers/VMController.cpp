#include "VMController.h"
#include "CollabVM.h"
#include "Database/VMSettings.h"

VMController::VMController(CollabVMServer& server, const std::shared_ptr<VMSettings>& settings)
	: server_(server),
	  io_context_(server.GetIOContext()),
	  settings_(settings),
	  turn_timer_(server.GetIOContext()),
	  vote_state_(VoteState::kIdle),
	  vote_count_yes_(0),
	  vote_count_no_(0),
	  vote_timer_(server.GetIOContext()),
	  current_turn_(nullptr),
	  connected_users_(0),
	  stop_reason_(StopReason::kNormal),
	  thumbnail_str_(nullptr) {
}

void VMController::ChangeSettings(const std::shared_ptr<VMSettings>& settings) {
	if(settings->TurnsEnabled != settings_->TurnsEnabled || settings->VotesEnabled != settings_->VotesEnabled) {
		server_.ActionsChanged(*this, *settings);
	} else if(settings->MOTD != settings_->MOTD && !settings->MOTD.empty()) {
		server_.BroadcastMOTD(*this, *settings);
	}
}

void VMController::Stop(StopReason reason) {
	boost::system::error_code ec;
	turn_timer_.cancel(ec);
	vote_timer_.cancel(ec);

	if(thumbnail_str_) {
		delete thumbnail_str_;
		thumbnail_str_ = nullptr;
	}

	server_.OnVMControllerStateChange(shared_from_this(), VMController::ControllerState::kStopping);
}

void VMController::Vote(CollabVMUser& user, bool vote) {
	if(settings_->VotesEnabled) {
		switch(vote_state_) {
			case VoteState::kCoolingdown: {
				int32_t time_remaining = std::chrono::duration_cast<std::chrono::duration<int32_t>>(vote_timer_.expires_from_now()).count();
				// Earlier I was attempting to get staff to bypass this but this will need more work, come back to it later
				if(time_remaining > 0) {
					server_.VoteCoolingDown(user, time_remaining);
					//break;
				}
				// Could fall through when the timer has expired,
				// although this may create a race condition with the
				// timer's callback
				break;
			}
			case VoteState::kIdle:
				if(vote) {
					// Start a new vote
					vote_state_ = VoteState::kVoting;
					vote_count_yes_ = 1;
					vote_count_no_ = 0;
					boost::system::error_code ec;
					vote_timer_.expires_from_now(std::chrono::seconds(settings_->VoteTime), ec);
					vote_timer_.async_wait(std::bind(&VMController::VoteEndedCallback, shared_from_this(), std::placeholders::_1));
					server_.UserStartedVote(*this, users_, user);
					server_.BroadcastVoteInfo(*this, users_, true, std::chrono::duration_cast<millisecs_t>(vote_timer_.expires_from_now()).count(), vote_count_yes_, vote_count_no_);
				}
				break;
			case VoteState::kVoting: {
				int32_t time_remaining = std::chrono::duration_cast<millisecs_t>(vote_timer_.expires_from_now()).count();
				if(time_remaining > 0) {
					IPData::VoteDecision prev_vote = user.ip_data.votes[this];
					bool changed = false;
					if(user.voted_limit == false) {
						if(user.voted_amount >= 5) {
							user.voted_limit = true;
							goto _vote_limit_die;
						}

						// A vote is already in progress so count the user's vote unless they've hit the limit
						if(vote && prev_vote != IPData::VoteDecision::kYes) {
							if(prev_vote == IPData::VoteDecision::kNo)
								vote_count_no_--, user.voted_amount++;

							vote_count_yes_++;
							user.voted_amount++;
							changed = true;
						} else if(!vote && prev_vote != IPData::VoteDecision::kNo) {
							if(prev_vote == IPData::VoteDecision::kYes)
								vote_count_yes_--, user.voted_amount++;

							vote_count_no_++;
							user.voted_amount++;
							changed = true;
						}

					_vote_limit_die : {}
					}

					if(changed) {
						server_.UserVoted(*this, users_, user, vote);
						server_.BroadcastVoteInfo(*this, users_, false, time_remaining, vote_count_yes_, vote_count_no_);
					}
				}
				break;
			}
		}
	}
}

void VMController::VoteEndedCallback(const boost::system::error_code& ec) {
	if(ec)
		return;

	server_.OnVMControllerVoteEnded(shared_from_this());
}

void VMController::EndVote() {
	if(vote_state_ != VoteState::kVoting)
		return;

	EndVoteCommonLogic((vote_count_yes_ >= vote_count_no_));
}

void VMController::SkipVote(bool pass) {
	if(vote_state_ != VoteState::kVoting)
		return;

	EndVoteCommonLogic(pass);
}


void VMController::EndVoteCommonLogic(bool vote_passed) {
	server_.BroadcastVoteEnded(*this, users_, vote_passed);

	if(settings_->VoteCooldownTime) {
		vote_state_ = VoteState::kCoolingdown;
		boost::system::error_code ec;
		vote_timer_.expires_from_now(std::chrono::seconds(settings_->VoteCooldownTime), ec);

		vote_timer_.async_wait([self = shared_from_this()](const boost::system::error_code& ec) {
			if(!ec)
				self->vote_state_ = VoteState::kIdle;
		});
	} else {
		vote_state_ = VoteState::kIdle;
	}

	if(vote_passed)
		RestoreVMSnapshot();
}

void VMController::TurnRequest(const std::shared_ptr<CollabVMUser>& user, bool turnJack, bool isStaff) {
	// If the user is already in the queue or they are already
	// in control don't allow them to make another turn request
	if(GetState() != ControllerState::kRunning || (!settings_->TurnsEnabled && !isStaff) ||
	   (user->waiting_turn && !turnJack) || current_turn_ == user)
		return;

	if(user->waiting_turn) {
		for(auto it = turn_queue_.begin(); it != turn_queue_.end(); it++) {
			if(*it == user) {
				turn_queue_.erase(it);
				user->waiting_turn = false;
				break;
			}
		}
	};

	if(!current_turn_) {
		// If no one currently has a turn then give the requesting user control
		current_turn_ = user;
		// Start the turn timer
		boost::system::error_code ec;
		turn_timer_.expires_from_now(std::chrono::seconds(settings_->TurnTime), ec);
		turn_timer_.async_wait(std::bind(&VMController::TurnTimerCallback, shared_from_this(), std::placeholders::_1));
	} else {
		if(!turnJack) {
			// Otherwise add them to the queue
			turn_queue_.push_back(user);
			user->waiting_turn = true;
		} else {
			// Turn-jack
			turn_queue_.push_front(current_turn_);
			current_turn_->waiting_turn = true;
			current_turn_ = user;

			boost::system::error_code ec;
			turn_timer_.cancel(ec);
			turn_timer_.expires_from_now(std::chrono::seconds(settings_->TurnTime), ec);
			turn_timer_.async_wait(std::bind(&VMController::TurnTimerCallback, shared_from_this(), std::placeholders::_1));
		}
	};

	server_.BroadcastTurnInfo(*this, users_, turn_queue_, current_turn_.get(),
							  std::chrono::duration_cast<millisecs_t>(turn_timer_.expires_from_now()).count());
}

void VMController::NextTurn() {
	int32_t time_remaining;
	if(!turn_queue_.empty()) {
		current_turn_ = turn_queue_.front();
		current_turn_->waiting_turn = false;
		turn_queue_.pop_front();

		// Set up the turn timer
		boost::system::error_code ec;
		turn_timer_.expires_from_now(std::chrono::seconds(settings_->TurnTime), ec);
		turn_timer_.async_wait(std::bind(&VMController::TurnTimerCallback, shared_from_this(), std::placeholders::_1));
		time_remaining = std::chrono::duration_cast<millisecs_t>(turn_timer_.expires_from_now()).count();
	} else {
		current_turn_ = nullptr;
		time_remaining = 0;
	}

	server_.BroadcastTurnInfo(*this, users_, turn_queue_, current_turn_.get(), time_remaining);
}

void VMController::ClearTurnQueue() {
	auto it = turn_queue_.begin();
	while(it != turn_queue_.end()) {
		(*it)->waiting_turn = false;
		it = turn_queue_.erase(it);
	}
	current_turn_ = nullptr;
	server_.BroadcastTurnInfo(*this, users_, turn_queue_, current_turn_.get(), 0);
}

void VMController::TurnTimerCallback(const boost::system::error_code& ec) {
	if(ec)
		return;

	server_.OnVMControllerTurnChange(shared_from_this());
}


void VMController::EndTurn(const std::shared_ptr<CollabVMUser>& user) {
	bool turn_change = false;
	// Check if they are in the turn queue and remove them if they are
	if(user->waiting_turn) {
		for(auto it = turn_queue_.begin(); it != turn_queue_.end(); it++) {
			if(*it == user) {
				turn_change = true;
				turn_queue_.erase(it);
				user->waiting_turn = false;
				// The client should not be in the queue more than once
				break;
			}
		}
	}

	// Check if it is currently their turn
	if(current_turn_ == user) {
		// Cancel the pending timer callback
		boost::system::error_code ec;
		turn_timer_.cancel(ec);

		if(!turn_queue_.empty()) {
			current_turn_ = turn_queue_.front();
			current_turn_->waiting_turn = false;
			turn_queue_.pop_front();

			// Set up the turn timer
			turn_timer_.expires_from_now(std::chrono::seconds(settings_->TurnTime), ec);
			turn_timer_.async_wait(std::bind(&VMController::TurnTimerCallback, shared_from_this(), std::placeholders::_1));
		} else
			current_turn_ = nullptr;

		turn_change = true;
	}

	if(turn_change)
		server_.BroadcastTurnInfo(*this, users_, turn_queue_, current_turn_.get(),
								  current_turn_ ? std::chrono::duration_cast<millisecs_t>(turn_timer_.expires_from_now()).count() : 0);
}

void VMController::AddUser(const std::shared_ptr<CollabVMUser>& user) {
	users_.AddUser(*user, [this](CollabVMUser& user) { OnAddUser(user); });

	int32_t time_remaining;
	if(current_turn_) {
		time_remaining = std::chrono::duration_cast<millisecs_t>(turn_timer_.expires_from_now()).count();
		if(time_remaining > 0)
			server_.SendTurnInfo(*user, time_remaining, *current_turn_->username, turn_queue_);
	}

	if(vote_state_ == VoteState::kVoting) {
		time_remaining = std::chrono::duration_cast<millisecs_t>(vote_timer_.expires_from_now()).count();
		if(time_remaining > 0)
			server_.SendVoteInfo(*this, *user, time_remaining, vote_count_yes_, vote_count_no_);
	}
}

void VMController::RemoveUser(const std::shared_ptr<CollabVMUser>& user) {
	// Remove the user's vote
	IPData::VoteDecision prev_vote = user->ip_data.votes[this];
	int32_t time_remaining = std::chrono::duration_cast<millisecs_t>(vote_timer_.expires_from_now()).count();
	if(vote_state_ == VoteState::kVoting && time_remaining > 0 && prev_vote != IPData::VoteDecision::kNotVoted) {
		if(prev_vote == IPData::VoteDecision::kYes)
			vote_count_yes_--;
		else if(prev_vote == IPData::VoteDecision::kNo)
			vote_count_no_--;

		user->ip_data.votes[this] = IPData::VoteDecision::kNotVoted;
		server_.BroadcastVoteInfo(*this, users_, false, time_remaining, vote_count_yes_, vote_count_no_);
	}

	EndTurn(user);

	users_.RemoveUser(*user, [this](CollabVMUser& user) { OnRemoveUser(user); });
}

void VMController::NewThumbnail(std::string* str) {
	server_.OnVMControllerThumbnailUpdate(shared_from_this(), str);
}
