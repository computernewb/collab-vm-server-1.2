//
// CollabVM Server
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#ifndef COLLAB_VM_SERVER_VNCHACK_H
#define COLLAB_VM_SERVER_VNCHACK_H

namespace collab3::core {

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

} // namespace collab3::core

#endif //COLLAB_VM_SERVER_VNCHACK_H
