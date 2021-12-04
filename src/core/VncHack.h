//
// Created by lily on 12/4/21.
//

#ifndef COLLAB_VM_SERVER_VNCHACK_H
#define COLLAB_VM_SERVER_VNCHACK_H

namespace collabvm::core {

	/**
	 * Hack-fix for LibVNCServer.
	 *
	 * Since it doesn't use asynchronous signals,
	 * a SIGPIPE in the VNC thread would result in a crash.
	 *
	 * We can ignore it using this function,
	 * and that'll fix the problem.
	 */
	void IgnoreSigPipe();

} // namespace collabvm::core

#endif //COLLAB_VM_SERVER_VNCHACK_H
