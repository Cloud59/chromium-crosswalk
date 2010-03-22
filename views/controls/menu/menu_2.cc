// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/controls/menu/menu_2.h"

#include "app/l10n_util.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "views/controls/menu/menu_wrapper.h"

namespace views {

Menu2::Menu2(menus::MenuModel* model)
    : model_(model),
      ALLOW_THIS_IN_INITIALIZER_LIST(
          wrapper_(MenuWrapper::CreateWrapper(this))) {
  Rebuild();
}

gfx::NativeMenu Menu2::GetNativeMenu() const {
  return wrapper_->GetNativeMenu();
}

void Menu2::RunMenuAt(const gfx::Point& point, Alignment alignment) {
  wrapper_->RunMenuAt(point, alignment);
}

void Menu2::RunContextMenuAt(const gfx::Point& point) {
  RunMenuAt(point, ALIGN_TOPLEFT);
}

void Menu2::CancelMenu() {
  wrapper_->CancelMenu();
}

void Menu2::Rebuild() {
  wrapper_->Rebuild();
}

void Menu2::UpdateStates() {
  wrapper_->UpdateStates();
}

MenuWrapper::MenuAction Menu2::GetMenuAction() const {
  return wrapper_->GetMenuAction();
}

}  // namespace
