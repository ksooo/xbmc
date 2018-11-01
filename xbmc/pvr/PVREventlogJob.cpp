/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVREventlogJob.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"

namespace PVR
{
CPVREventlogJob::CPVREventlogJob(bool bNotifyUser, bool bError, const std::string& label, const std::string& msg, const std::string& icon)
{
  AddEvent(bNotifyUser, bError, label, msg, icon);
}

void CPVREventlogJob::AddEvent(bool bNotifyUser, bool bError, const std::string& label, const std::string& msg, const std::string& icon)
{
  m_events.emplace_back(Event(bNotifyUser, bError, label, msg, icon));
}

bool CPVREventlogJob::DoWork()
{
  for (const auto& event : m_events)
  {
    if (event.m_bNotifyUser)
      CGUIDialogKaiToast::QueueNotification(
        event.m_bError ? CGUIDialogKaiToast::Error : CGUIDialogKaiToast::Info, event.m_label.c_str(), event.m_msg, 5000, true);

    // Write event log entry.
    CServiceBroker::GetEventLog().Add(
      EventPtr(new CNotificationEvent(event.m_label, event.m_msg, event.m_icon, event.m_bError ? EventLevel::Error : EventLevel::Information)));
  }
  return true;
}

} // namespace PVR
