/*
 *      Copyright (C) 2012-2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/include/xbmc_pvr_types.h"
#include "pvr/timers/PVRTimerType.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/PVRManager.h"
#include "utils/log.h"

using namespace PVR;

// static
const std::vector<CPVRTimerTypePtr> CPVRTimerType::GetAllTypes()
{
  std::vector<CPVRTimerTypePtr> allTypes;

  PVR_CLIENTMAP clients;
  g_PVRClients->GetConnectedClients(clients);
  for (auto cit = clients.begin(); cit != clients.end(); ++cit)
  {
    const PVR_TIMER_TYPES *types = g_PVRClients->GetTimerTypes((*cit).first);
    for (auto tit = types->begin(); tit != types->end(); ++tit)
    {
      if ((*tit).iId != PVR_TIMER_TYPE_NONE)
        allTypes.push_back(CPVRTimerTypePtr(new CPVRTimerType(*tit, (*cit).first)));
    }
  }

  return allTypes;
}

// static
const CPVRTimerTypePtr CPVRTimerType::GetFirstAvailableType()
{
  PVR_CLIENTMAP clients;
  g_PVRClients->GetConnectedClients(clients);
  for (auto cit = clients.begin(); cit != clients.end(); ++cit)
  {
    const PVR_TIMER_TYPES *types = g_PVRClients->GetTimerTypes((*cit).first);
    for (auto tit = types->begin(); tit != types->end(); ++tit)
    {
      if ((*tit).iId != PVR_TIMER_TYPE_NONE)
        return CPVRTimerTypePtr(new CPVRTimerType(*tit, (*cit).first));
    }
  }
  return CPVRTimerTypePtr();
}

// static
CPVRTimerTypePtr CPVRTimerType::CreateFromIds(unsigned int iTypeId, int iClientId)
{
  const PVR_TIMER_TYPES *types = g_PVRClients->GetTimerTypes(iClientId);
  if (types)
  {
    for (auto it = types->begin(); it != types->end(); ++it)
    {
      if (it->iId == iTypeId)
      {
        CPVRTimerTypePtr newType(new CPVRTimerType());

        newType->m_iClientId      = iClientId;
        newType->m_iTypeId        = iTypeId;
        newType->m_iAttributes    = it->iAttributes;
        newType->m_strDescription = it->strDescription;

        return newType;
      }
    }
  }

  CLog::Log(LOGERROR, "CPVRTimerType::CreateFromIds unable to resolve numeric timer type (%d, %d)", iTypeId, iClientId);
  return CPVRTimerTypePtr();
}

// static
CPVRTimerTypePtr CPVRTimerType::CreateFromAttributes(
  unsigned int iMustHaveAttr, unsigned int iMustNotHaveAttr, int iClientId)
{
  const PVR_TIMER_TYPES *types = g_PVRClients->GetTimerTypes(iClientId);
  if (types)
  {
    for (auto it = types->begin(); it != types->end(); ++it)
    {
      if (((it->iAttributes & iMustHaveAttr)    == iMustHaveAttr) &&
          ((it->iAttributes & iMustNotHaveAttr) == 0))
      {
        CPVRTimerTypePtr newType(new CPVRTimerType());

        newType->m_iClientId      = iClientId;
        newType->m_iTypeId        = it->iId;
        newType->m_iAttributes    = it->iAttributes;
        newType->m_strDescription = it->strDescription;

        return newType;
      }
    }
  }

  CLog::Log(LOGERROR, "CPVRTimerType::CreateFromAttributes unable to resolve timer type (0x%x, 0x%x, %d)", iMustHaveAttr, iMustNotHaveAttr, iClientId);
  return CPVRTimerTypePtr();
}

CPVRTimerType::CPVRTimerType() :
  m_iClientId(0),
  m_iTypeId(PVR_TIMER_TYPE_NONE),
  m_iAttributes(PVR_TIMER_TYPE_ATTRIBUTE_NONE)
{
}

CPVRTimerType::CPVRTimerType(const PVR_TIMER_TYPE &type, int iClientId) :
  m_iClientId(iClientId),
  m_iTypeId(type.iId),
  m_iAttributes(type.iAttributes),
  m_strDescription(type.strDescription)
{
}

// virtual
CPVRTimerType::~CPVRTimerType()
{
}

bool CPVRTimerType::operator ==(const CPVRTimerType& right) const
{
  return (m_iClientId      == right.m_iClientId   &&
          m_iTypeId        == right.m_iTypeId  &&
          m_iAttributes    == right.m_iAttributes &&
          m_strDescription == right.m_strDescription);
}

bool CPVRTimerType::operator !=(const CPVRTimerType& right) const
{
  return !(*this == right);
}
