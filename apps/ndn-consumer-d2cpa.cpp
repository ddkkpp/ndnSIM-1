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

#include "ndn-consumer-d2cpa.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

#include "utils/batches.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerD2CPA");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ConsumerD2CPA);

TypeId
ConsumerD2CPA::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ConsumerD2CPA")
      .SetGroupName("Ndn")
      .SetParent<ConsumerCbr>()
      .AddConstructor<ConsumerD2CPA>()

      .AddAttribute("vMax", "max send rate", UintegerValue(200), MakeUintegerAccessor(&ConsumerD2CPA::m_vMax),
                    MakeUintegerChecker<uint32_t>())
        .AddAttribute("vStep", "value of send rate for each increment", UintegerValue(10), MakeUintegerAccessor(&ConsumerD2CPA::m_vStep),
                    MakeUintegerChecker<uint32_t>())
        .AddAttribute("tStep", "time between each send rate increase", TimeValue(Seconds(0.05)),
            MakeTimeAccessor(&ConsumerD2CPA::m_tStep),
            MakeTimeChecker()) 
        .AddAttribute("MaxSeqA", "Maximum sequence number to request",
                    UintegerValue(10000),
                    MakeUintegerAccessor(&ConsumerD2CPA::m_seqMaxA), MakeUintegerChecker<uint32_t>())
        .AddAttribute("range", "range of seq", UintegerValue(10), MakeUintegerAccessor(&ConsumerD2CPA::m_range),
                    MakeUintegerChecker<uint32_t>());

  return tid;
}

ConsumerD2CPA::ConsumerD2CPA()
  : ConsumerCbr()
  , m_initial(true)
  , m_vNow(0)
{
}

void
ConsumerD2CPA::StartApplication()
{
  Consumer::StartApplication();

  InitializeD2CPA();
  
}

void
ConsumerD2CPA::InitializeD2CPA()
{
  uint32_t numOfSteps = m_vMax / m_vStep;
  NS_LOG_LOGIC("numOfSteps=" << numOfSteps);
  D2CPA();
  for(uint32_t i=1;i<numOfSteps;i++){
      Simulator::ScheduleWithContext(GetNode()->GetId(), m_tStep*i, &ConsumerD2CPA::D2CPA,
                                   this);
  }
  //ScheduleNextPacket();
}

void
ConsumerD2CPA::D2CPA()
{
  m_vNow += m_vStep;
  NS_LOG_LOGIC("m_vNow=" << m_vNow);
  ScheduleNextPacket();
}

void
ConsumerD2CPA::SendPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s max -> " << m_seqMax << "\n";

  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());

    // NS_ASSERT (m_seqLifetimes.find (seq) != m_seqLifetimes.end ());
    // if (m_seqLifetimes.find (seq)->time <= Simulator::Now ())
    //   {

    //     NS_LOG_DEBUG ("Expire " << seq);
    //     m_seqLifetimes.erase (seq); // lifetime expired. Trying to find another unexpired
    //     sequence number
    //     continue;
    //   }
    NS_LOG_DEBUG("=interest seq " << seq << " from m_retxSeqs");
    break;
  }

  if (seq == std::numeric_limits<uint32_t>::max()) // no retransmission
  {
    if (m_seqMaxA != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMaxA) {
        return; // we are totally done
      }
    }

    seq = ConsumerD2CPA::GetNextSeq();
    m_seq++;
  }

  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s -> " << seq << "\n";

  //
  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);
  //

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO("> Interest for " << seq << ", Total: " << m_seq << ", face: " << m_face->getId());
  NS_LOG_DEBUG("Trying to add " << seq << " with " << Simulator::Now() << ". already "
                                << m_seqTimeouts.size() << " items");

  m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));

  m_seqLastDelay.erase(seq);
  m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));

  m_seqRetxCounts[seq]++;

  m_rtt->SentSeq(SequenceNumber32(seq), 1);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  ConsumerD2CPA::ScheduleNextPacket();
}

uint32_t
ConsumerD2CPA::GetNextSeq()
{
  NS_LOG_LOGIC("m_vNow=" << m_vNow);
  auto r = rand() % m_range;
  NS_LOG_LOGIC("m_seqMaxA=" << m_seqMaxA);
  NS_LOG_LOGIC("rand =" << r);
  uint32_t content_index = m_seqMaxA - r;//从m_seqMaxA到m_seqMaxA-m_range+1之间均匀选择
  NS_LOG_LOGIC("content_index=" << content_index);
  return content_index;
}

void
ConsumerD2CPA::ScheduleNextPacket()
{
  if (m_firstTime) {
    NFD_LOG_DEBUG("first time");
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &ConsumerD2CPA::SendPacket, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning())
  {
    Time delay = Seconds(1.0 / m_vNow);
    NS_LOG_LOGIC("delay=" << delay);
    m_sendEvent = Simulator::Schedule(delay, &ConsumerD2CPA::SendPacket, this);
  }
}

} // namespace ndn
} // namespace ns3