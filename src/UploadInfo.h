#pragma once
#include "CollabVMUser.h"
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <atomic>
#include <memory>
#include <string>

class VMController;

struct UploadInfo {
	UploadInfo(const std::shared_ptr<CollabVMUser>& user, const std::shared_ptr<VMController>& vm_controller,
			   const std::string& filename, size_t file_size, bool run_file, uint32_t cooldown_time)
		: user(user),
		  vm_controller(vm_controller),
		  username(user->username),
		  filename(filename),
		  file_size(file_size),
		  run_file(run_file),
		  ip_data(user->ip_data),
		  http_state(HttpUploadState::kNotStarted),
		  canceled(false),
		  timeout_timer(nullptr) {
	}

#ifdef _DEBUG
	UploadInfo(/*const std::shared_ptr<CollabVMUser>& user, const std::shared_ptr<std::string>& username, */
			   IPData& ip_data,
			   const std::string& filename, size_t file_size, bool run_file, uint32_t cooldown_time)
		: //user(user),
		  //vm_controller(user->vm_controller),
		  //username(username),
		  filename(filename),
		  file_size(file_size),
		  run_file(run_file),
		  ip_data(ip_data),
		  http_state(HttpUploadState::kNotStarted),
		  canceled(false),
		  timeout_timer(nullptr) {
	}
#endif

	std::string filename;
	size_t file_size;
	bool run_file;
	/**
	* The path on the host to the temporary file where the upload is
	* saved to. It is used to open the file_stream.
	*/
	std::string file_path;
	/**
	* A file stream used to write the file chunks received from the client
	* to a temporary file and then read the file to send it to the agent.
	*/
	std::fstream file_stream;
	/**
	* The username of the user who uploaded the file. This is stored
	* here in case the user disconects before the file is executed by the agent.
	*/
	std::shared_ptr<std::string> username;
	/**
	* Store the VMController that the file will be uploaded to in case the
	* user switches VMs during the upload
	*/
	std::shared_ptr<VMController> vm_controller;
	std::weak_ptr<CollabVMUser> user;
	IPData& ip_data;

	std::map<std::string, std::shared_ptr<UploadInfo> /*, CollabVMServer::case_insensitive_cmp*/>::iterator upload_it;

	enum class HttpUploadState : uint8_t {
		kNotStarted,
		kNotWriting,
		kWriting,
		kCancel
	};

	/**
	 * Set to true when the HTTP upload has been stopped.
	 */
	std::atomic<HttpUploadState> http_state;

	/**
	 * Set to true when the processing thread has handled this upload.
	 */
	bool canceled;

	boost::asio::steady_timer* timeout_timer;
};
