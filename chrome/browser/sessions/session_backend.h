// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SESSIONS_SESSION_BACKEND_H_
#define CHROME_BROWSER_SESSIONS_SESSION_BACKEND_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/sessions/base_session_service.h"
#include "chrome/browser/sessions/session_command.h"

namespace base {
class File;
}

// SessionBackend -------------------------------------------------------------

// SessionBackend is the backend used by BaseSessionService. It is responsible
// for maintaining two files:
// . The current file, which is the file commands passed to AppendCommands
//   get written to.
// . The last file. When created the current file is moved to the last
//   file.
//
// Each file contains an arbitrary set of commands supplied from
// BaseSessionService. A command consists of a unique id and a stream of bytes.
// SessionBackend does not use the id in anyway, that is used by
// BaseSessionService.
class SessionBackend : public base::RefCountedThreadSafe<SessionBackend> {
 public:
  typedef SessionCommand::id_type id_type;
  typedef SessionCommand::size_type size_type;

  // Initial size of the buffer used in reading the file. This is exposed
  // for testing.
  static const int kFileReadBufferSize;

  // Creates a SessionBackend. This method is invoked on the MAIN thread,
  // and does no IO. The real work is done from Init, which is invoked on
  // the file thread.
  //
  // |path_to_dir| gives the path the files are written two, and |type|
  // indicates which service is using this backend. |type| is used to determine
  // the name of the files to use as well as for logging.
  SessionBackend(BaseSessionService::SessionType type,
                 const base::FilePath& path_to_dir);

  // Moves the current file to the last file, and recreates the current file.
  //
  // NOTE: this is invoked before every command, and does nothing if we've
  // already Init'ed.
  void Init();
  bool inited() const { return inited_; }

  // Appends the specified commands to the current file. If reset_first is
  // true the the current file is recreated.
  void AppendCommands(ScopedVector<SessionCommand> commands, bool reset_first);

  // Invoked from the service to read the commands that make up the last
  // session, invokes ReadLastSessionCommandsImpl to do the work.
  void ReadLastSessionCommands(
      const base::CancelableTaskTracker::IsCanceledCallback& is_canceled,
      const BaseSessionService::GetCommandsCallback& callback);

  // Reads the commands from the last file.
  //
  // On success, the read commands are added to commands.
  bool ReadLastSessionCommandsImpl(ScopedVector<SessionCommand>* commands);

  // Deletes the file containing the commands for the last session.
  void DeleteLastSession();

  // Moves the current session to the last and resets the current. This is
  // called during startup and if the user launchs the app and no tabbed
  // browsers are running.
  void MoveCurrentSessionToLastSession();

  // Reads the commands from the current file.
  //
  // On success, the read commands are added to commands. It is up to the
  // caller to delete the commands.
  bool ReadCurrentSessionCommandsImpl(ScopedVector<SessionCommand>* commands);

 private:
  friend class base::RefCountedThreadSafe<SessionBackend>;

  ~SessionBackend();

  // If current_session_file_ is open, it is truncated so that it is essentially
  // empty (only contains the header). If current_session_file_ isn't open, it
  // is is opened and the header is written to it. After this
  // current_session_file_ contains no commands.
  // NOTE: current_session_file_ may be NULL if the file couldn't be opened or
  // the header couldn't be written.
  void ResetFile();

  // Opens the current file and writes the header. On success a handle to
  // the file is returned.
  base::File* OpenAndWriteHeader(const base::FilePath& path);

  // Appends the specified commands to the specified file.
  bool AppendCommandsToFile(base::File* file,
                            const ScopedVector<SessionCommand>& commands);

  const BaseSessionService::SessionType type_;

  // Returns the path to the last file.
  base::FilePath GetLastSessionPath();

  // Returns the path to the current file.
  base::FilePath GetCurrentSessionPath();

  // Directory files are relative to.
  const base::FilePath path_to_dir_;

  // Whether the previous target file is valid.
  bool last_session_valid_;

  // Handle to the target file.
  scoped_ptr<base::File> current_session_file_;

  // Whether we've inited. Remember, the constructor is run on the
  // Main thread, all others on the IO thread, hence lazy initialization.
  bool inited_;

  // If true, the file is empty (no commands have been added to it).
  bool empty_file_;

  DISALLOW_COPY_AND_ASSIGN(SessionBackend);
};

#endif  // CHROME_BROWSER_SESSIONS_SESSION_BACKEND_H_
