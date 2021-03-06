// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin/auth_sync_observer.h"

#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/chromeos/login/users/supervised_user_manager.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/user_manager/user_manager.h"
#include "components/user_manager/user_type.h"
#include "content/public/browser/user_metrics.h"
#include "google_apis/gaia/gaia_auth_util.h"

class Profile;
class ProfileSyncService;

namespace chromeos {

AuthSyncObserver::AuthSyncObserver(Profile* profile)
    : profile_(profile) {
}

AuthSyncObserver::~AuthSyncObserver() {
}

void AuthSyncObserver::StartObserving() {
  ProfileSyncService* sync_service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  if (sync_service)
    sync_service->AddObserver(this);
}

void AuthSyncObserver::Shutdown() {
  ProfileSyncService* sync_service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  if (sync_service)
    sync_service->RemoveObserver(this);
}

void AuthSyncObserver::OnStateChanged() {
  DCHECK(user_manager::UserManager::Get()->IsLoggedInAsRegularUser() ||
         user_manager::UserManager::Get()->IsLoggedInAsSupervisedUser());
  ProfileSyncService* sync_service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  user_manager::User* user = ProfileHelper::Get()->GetUserByProfile(profile_);
  GoogleServiceAuthError::State state =
      sync_service->GetAuthError().state();
  if (state != GoogleServiceAuthError::NONE &&
      state != GoogleServiceAuthError::CONNECTION_FAILED &&
      state != GoogleServiceAuthError::SERVICE_UNAVAILABLE &&
      state != GoogleServiceAuthError::REQUEST_CANCELED) {
    // Invalidate OAuth2 refresh token to force Gaia sign-in flow. This is
    // needed because sign-out/sign-in solution is suggested to the user.
    // TODO(nkostylev): Remove after crosbug.com/25978 is implemented.
    LOG(WARNING) << "Invalidate OAuth token because of a sync error: "
                 << sync_service->GetAuthError().ToString();
    std::string email = user->email();
    DCHECK(!email.empty());
    // TODO(nkostyelv): Change observer after active user has changed.
    user_manager::User::OAuthTokenStatus old_status =
        user->oauth_token_status();
    user_manager::UserManager::Get()->SaveUserOAuthStatus(
        email, user_manager::User::OAUTH2_TOKEN_STATUS_INVALID);
    if (user->GetType() == user_manager::USER_TYPE_SUPERVISED &&
        old_status != user_manager::User::OAUTH2_TOKEN_STATUS_INVALID) {
       // Attempt to restore token from file.
      ChromeUserManager::Get()
          ->GetSupervisedUserManager()
          ->LoadSupervisedUserToken(
              profile_,
              base::Bind(&AuthSyncObserver::OnSupervisedTokenLoaded,
                         base::Unretained(this)));
       content::RecordAction(
           base::UserMetricsAction("ManagedUsers_Chromeos_Sync_Invalidated"));
    }
  } else if (state == GoogleServiceAuthError::NONE) {
    if (user->GetType() == user_manager::USER_TYPE_SUPERVISED &&
        user->oauth_token_status() ==
            user_manager::User::OAUTH2_TOKEN_STATUS_INVALID) {
      LOG(ERROR) <<
          "Got an incorrectly invalidated token case, restoring token status.";
      user_manager::UserManager::Get()->SaveUserOAuthStatus(
          user->email(), user_manager::User::OAUTH2_TOKEN_STATUS_VALID);
       content::RecordAction(
           base::UserMetricsAction("ManagedUsers_Chromeos_Sync_Recovered"));
    }
  }
}

void AuthSyncObserver::OnSupervisedTokenLoaded(const std::string& token) {
  ChromeUserManager::Get()->GetSupervisedUserManager()->ConfigureSyncWithToken(
      profile_, token);
}

}  // namespace chromeos
