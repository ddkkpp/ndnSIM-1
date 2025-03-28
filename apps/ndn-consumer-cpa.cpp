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

#include "ndn-consumer-cpa.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

#include "utils/batches.hpp"

#include <ndn-cxx/lp/tags.hpp>

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerCPA");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ConsumerCPA);

TypeId
ConsumerCPA::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ConsumerCPA")
      .SetGroupName("Ndn")
      .SetParent<ConsumerCbr>()
      .AddConstructor<ConsumerCPA>()
      .AddAttribute("vMax", "max send rate", DoubleValue(200), MakeDoubleAccessor(&ConsumerCPA::m_vMax),
              MakeDoubleChecker<double>())
      .AddAttribute("isDynamic", "is rate Dynamic?", BooleanValue(false), 
              MakeBooleanAccessor(&ConsumerCPA::m_isDynamic),
              MakeBooleanChecker())
        .AddAttribute("vStep", "value of send rate for each increment", DoubleValue(10), MakeDoubleAccessor(&ConsumerCPA::m_vStep),
              MakeDoubleChecker<double>())
        .AddAttribute("tStep", "time between each send rate increase", TimeValue(Seconds(0.05)),
            MakeTimeAccessor(&ConsumerCPA::m_tStep),
            MakeTimeChecker()) 
        .AddAttribute("MaxSeqA", "Maximum sequence number to request",
                    UintegerValue(10000),
                    MakeUintegerAccessor(&ConsumerCPA::m_seqMaxA), MakeUintegerChecker<uint32_t>())
        .AddAttribute("range", "range of seq", UintegerValue(10), MakeUintegerAccessor(&ConsumerCPA::m_range),
                    MakeUintegerChecker<uint32_t>());

  return tid;
}

ConsumerCPA::ConsumerCPA()
  : ConsumerCbr()
  , m_initial(true)
  , m_vNow(0)
{
}

void
ConsumerCPA::StartApplication()
{
  Consumer::StartApplication();

  InitializeCPA();
  
}

void
ConsumerCPA::InitializeCPA()
{
  if(m_isDynamic){
    NS_LOG_LOGIC("is dynamic");
    uint32_t numOfSteps = m_vMax / m_vStep;
    NS_LOG_LOGIC("numOfSteps=" << numOfSteps);
    CPA();
    for(uint32_t i=1;i<numOfSteps;i++){
        Simulator::ScheduleWithContext(GetNode()->GetId(), m_tStep*i, &ConsumerCPA::CPA,
                                    this);
    }
  }
  else{
    NS_LOG_LOGIC("is static");
    m_vNow = m_vMax;
    NS_LOG_LOGIC("m_vNow=" << m_vNow);
  }
  ScheduleNextPacket();
}

void
ConsumerCPA::CPA()
{
  m_vNow += m_vStep;
  NS_LOG_LOGIC("m_vNow=" << m_vNow);
}

void
ConsumerCPA::SendPacket()
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
    //之前写成了m_seqMaxA，区别是:m_seqMax是发送的兴趣包总数上限（原本ndn就有的），m_seqMaxA是发送的兴趣包序号上限（我加的）
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        return; // we are totally done
      }
    }

    seq = ConsumerCPA::GetNextSeq();
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

  //加上ConsumerIdTag
  auto nodeid = GetNode()->GetId();
  //高16位为用户类型（0为正常，1为恶意），接下来16位为是否直接来自消费者（消费者产生的为1），低32位为节点id。设置Tag并读取三个部分
  uint64_t tagValue = (uint64_t)1 << 48 | (uint64_t)1 << 32 |nodeid;
  interest->setTag(make_shared<ndn::lp::ConsumerIdTag>(tagValue));
  auto tagRead = *(interest->getTag<ndn::lp::ConsumerIdTag>());
  // 提取高16位
  uint16_t highBits = (tagRead >> 48) & 0xFFFF;
  // 提取中16位
  uint16_t middleBits = (tagRead >> 32) & 0xFFFF;
  // 提取低32位
  uint32_t lowBits = tagRead & 0xFFFFFFFF;
  NS_LOG_INFO("Tag value: high16=" << highBits << ", mid16=" << middleBits<< ", low32=" << lowBits);


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

  ConsumerCPA::ScheduleNextPacket();
}

uint32_t
ConsumerCPA::GetNextSeq()
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
ConsumerCPA::ScheduleNextPacket()
{
  if (m_firstTime) {
    NFD_LOG_DEBUG("first time");
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &ConsumerCPA::SendPacket, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning())
  {
    Time delay = Seconds(1.0 / m_vNow);
    NS_LOG_LOGIC("delay=" << delay);
    m_sendEvent = Simulator::Schedule(delay, &ConsumerCPA::SendPacket, this);
  }
}

} // namespace ndn
} // namespace ns3