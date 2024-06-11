/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ndn-consumer-pulse.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

#include "utils/batches.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerPulse");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ConsumerPulse);

TypeId
ConsumerPulse::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ConsumerPulse")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<ConsumerPulse>()

      .AddAttribute("period", "pulse period", 
                    TimeValue(Seconds(0)), MakeTimeAccessor(&ConsumerPulse::m_period),
                    MakeTimeChecker())
        .AddAttribute("sendTime", "send time in a period", 
                TimeValue(Seconds(0)), MakeTimeAccessor(&ConsumerPulse::m_sendTime),
                MakeTimeChecker())
        .AddAttribute("sendRate", "send rate in send time", UintegerValue(0),
            MakeUintegerAccessor(&ConsumerPulse::m_sendRate),
            MakeUintegerChecker<uint32_t>())
        .AddAttribute("periodNum", "period number", UintegerValue(0),
            MakeUintegerAccessor(&ConsumerPulse::m_periodNum),
            MakeUintegerChecker<uint32_t>());

  return tid;
}

ConsumerPulse::ConsumerPulse()
  : m_initial(true)
{
}

void
ConsumerPulse::StartApplication()
{
  Consumer::StartApplication();

  InitializePulse();
  
}

void
ConsumerPulse::InitializePulse()
{
  for(int i=0;i<m_periodNum;i++){
      Simulator::ScheduleWithContext(GetNode()->GetId(), m_period*i, &ConsumerPulse::Pulse,
                                   this);
  }
  //ScheduleNextPacket();
}

void
ConsumerPulse::Pulse()
{
  
  m_seqMax += m_sendTime.GetSeconds() * m_sendRate;
  NS_LOG_DEBUG("m_seqMax = "<<m_seqMax);
  ScheduleNextPacket();
}

void
ConsumerPulse::ScheduleNextPacket()
{
  if (!m_sendEvent.IsRunning()) {
    Time delay = Seconds(1.0 / m_sendRate);
    m_sendEvent = Simulator::Schedule(delay, &Consumer::SendPacket, this);
  }
}

} // namespace ndn
} // namespace ns3
