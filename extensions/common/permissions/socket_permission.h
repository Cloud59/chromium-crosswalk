// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_PERMISSIONS_SOCKET_PERMISSION_H_
#define EXTENSIONS_COMMON_PERMISSIONS_SOCKET_PERMISSION_H_

#include <string>

#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/set_disjunction_permission.h"
#include "extensions/common/permissions/socket_permission_data.h"

namespace extensions {

class SocketPermission
    : public SetDisjunctionPermission<SocketPermissionData, SocketPermission> {
 public:
  struct CheckParam : APIPermission::CheckParam {
    CheckParam(content::SocketPermissionRequest::OperationType type,
               const std::string& host,
               int port)
        : request(type, host, port) {}
    content::SocketPermissionRequest request;
  };

  explicit SocketPermission(const APIPermissionInfo* info);

  ~SocketPermission() override;

  // Returns the localized permission messages of this permission.
  PermissionMessages GetMessages() const override;

 private:
  bool AddAnyHostMessage(PermissionMessages& messages) const;
  void AddSubdomainHostMessage(PermissionMessages& messages) const;
  void AddSpecificHostMessage(PermissionMessages& messages) const;
  void AddNetworkListMessage(PermissionMessages& messages) const;
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_PERMISSIONS_SOCKET_PERMISSION_H_
