/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Tsinghua University, P.R.China.
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
 *
 * @author Xiaoke Jiang <shock.jiang@gmail.com>
 **/

#include "ndn-consumer-zipf-fdbk.hpp"
#include "ndn-cxx/data.hpp"

#include <math.h>

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerZipfFdbk");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ConsumerZipfFdbk);

TypeId
ConsumerZipfFdbk::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ConsumerZipfFdbk")
      .SetGroupName("Ndn")
      .SetParent<ConsumerCbr>()
      .AddConstructor<ConsumerZipfFdbk>()

      .AddAttribute("NumberOfContents", "Number of the Contents in total", StringValue("100"),
                    MakeUintegerAccessor(&ConsumerZipfFdbk::SetNumberOfContents,
                                         &ConsumerZipfFdbk::GetNumberOfContents),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("q", "parameter of improve rank", StringValue("0.7"),
                    MakeDoubleAccessor(&ConsumerZipfFdbk::SetQ,
                                       &ConsumerZipfFdbk::GetQ),
                    MakeDoubleChecker<double>())

      .AddAttribute("s", "parameter of power", StringValue("0.7"),
                    MakeDoubleAccessor(&ConsumerZipfFdbk::SetS,
                                       &ConsumerZipfFdbk::GetS),
                    MakeDoubleChecker<double>());

  return tid;
}

ConsumerZipfFdbk::ConsumerZipfFdbk()
  : m_N(100) // needed here to make sure when SetQ/SetS are called, there is a valid value of N
  , m_q(0.7)
  , m_s(0.7)
  , m_seqRng(CreateObject<UniformRandomVariable>())
{
  // SetNumberOfContents is called by NS-3 object system during the initialization
}

ConsumerZipfFdbk::~ConsumerZipfFdbk()
{
}


void
ConsumerZipfFdbk::OnData(shared_ptr<const Data> data)
{
  Consumer::OnData(data);
  //if(::ndn::readNonNegativeInteger(data->getSignature().getValue())==std::numeric_limits<uint32_t>::max())//data为假
  if(!(data->getIsFake()))
  {
    feedback_name=data->getName();
    feedback_content=data->getContent();
    NS_LOG_DEBUG("Receive invalid data");
    SendFeedback();
  }
}

void
ConsumerZipfFdbk::SetNumberOfContents(uint32_t numOfContents)
{
  m_N = numOfContents;

  NS_LOG_DEBUG(m_q << " and " << m_s << " and " << m_N);

  m_Pcum = std::vector<double>(m_N + 1);

  m_Pcum[0] = 0.0;
  for (uint32_t i = 1; i <= m_N; i++) {
    m_Pcum[i] = m_Pcum[i - 1] + 1.0 / std::pow(i + m_q, m_s);
  }

  for (uint32_t i = 1; i <= m_N; i++) {
    m_Pcum[i] = m_Pcum[i] / m_Pcum[m_N];
    NS_LOG_LOGIC("Cumulative probability [" << i << "]=" << m_Pcum[i]);
  }
}

uint32_t
ConsumerZipfFdbk::GetNumberOfContents() const
{
  return m_N;
}

void
ConsumerZipfFdbk::SetQ(double q)
{
  m_q = q;
  SetNumberOfContents(m_N);
}

double
ConsumerZipfFdbk::GetQ() const
{
  return m_q;
}

void
ConsumerZipfFdbk::SetS(double s)
{
  m_s = s;
  SetNumberOfContents(m_N);
}

double
ConsumerZipfFdbk::GetS() const
{
  return m_s;
}

void
ConsumerZipfFdbk::SendFeedback()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  uint64_t content_64=0;//加入过滤器的元素
  int j=0;
  //content的buffer是vector<uint8_t>,遍历该容器，转化为过滤器能使用的uint64_t
  for(auto i=feedback_content.value_begin();i<feedback_content.value_end();i++){
    if(j==7){
      j=0;
    }
    //std::cout<<"j="<<j<<" i="<<(int)i<<std::endl;
    content_64+=((*i)<<(8*j));
    j++;
    //std::cout<<"content= "<<content<<std::endl;
  }

  shared_ptr<Interest> feedback = make_shared<Interest>();
  feedback->setName(feedback_name);
  //feedback->setApplicationParameters(feedback_content);
  feedback->setNonce(std::numeric_limits<uint32_t>::max());//feedback的nouce设置为max
  feedback->setContentinFeedback(content_64);

  // NS_LOG_INFO ("Requesting Feedback: \n" << *interest);
  NS_LOG_INFO("Send feedback " << feedback_name << ", to face: " << m_face->getId()<<" , aims at content_64 = "<<content_64);

  NS_LOG_INFO("Feedback's getContentinFeedback is "<<feedback->getContentinFeedback());

  m_transmittedInterests(feedback, this, m_face);
  m_appLink->onReceiveInterest(*feedback);

  // ConsumerZipfFdbk::ScheduleNextPacket();
}

void
ConsumerZipfFdbk::SendPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

   //std::cout << Simulator::Now ().ToDouble (Time::S) << "s max -> " << m_seqMax << "\n";

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
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        return; // we are totally done
      }
    }

    seq = ConsumerZipfFdbk::GetNextSeq();
    m_seq++;
  }

  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s -> " << seq << "\n";

  //
  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);
  //

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()-1));
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

  ConsumerZipfFdbk::ScheduleNextPacket();
}

uint32_t
ConsumerZipfFdbk::GetNextSeq()
{
  uint32_t content_index = 1; //[1, m_N]
  double p_sum = 0;

  double p_random = m_seqRng->GetValue();
  while (p_random == 0) {
    p_random = m_seqRng->GetValue();
  }
  // if (p_random == 0)
  NS_LOG_LOGIC("p_random=" << p_random);
  for (uint32_t i = 1; i <= m_N; i++) {
    p_sum = m_Pcum[i]; // m_Pcum[i] = m_Pcum[i-1] + p[i], p[0] = 0;   e.g.: p_cum[1] = p[1],
                       // p_cum[2] = p[1] + p[2]
    if (p_random <= p_sum) {
      content_index = i;
      break;
    } // if
  }   // for
  // content_index = 1;
  NS_LOG_DEBUG("RandomNumber=" << content_index);
  return content_index;
}

void
ConsumerZipfFdbk::ScheduleNextPacket()
{

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &ConsumerZipfFdbk::SendPacket, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning())
    m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                      : Seconds(m_random->GetValue()),
                                      &ConsumerZipfFdbk::SendPacket, this);
}

} /* namespace ndn */
} /* namespace ns3 */
