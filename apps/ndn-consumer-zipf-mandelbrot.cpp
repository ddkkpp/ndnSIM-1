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

 #include "ndn-consumer-zipf-mandelbrot.hpp"
 #include <ndn-cxx/lp/tags.hpp>
 
 #include "ns3/log.h"
 
 #include <math.h>
 
 NS_LOG_COMPONENT_DEFINE("ndn.ConsumerZipfMandelbrot");
 
 namespace ns3 {
 namespace ndn {
 
 NS_OBJECT_ENSURE_REGISTERED(ConsumerZipfMandelbrot);
 
 
 void 
 computeMetricsWDCallback(ConsumerZipfMandelbrot *ptr)
 {
   NS_LOG_INFO("m_numOfReceivedData in this period = "<<ptr->m_numOfReceivedData);
   if(ptr->m_numOfReceivedData==0){
     NS_LOG_INFO("retrievalTime in this period = (没有data返回)");
   }
   else{
     NS_LOG_INFO("m_numOfSentInterest in this period = "<<ptr->m_numOfSentInterest);
     NS_LOG_INFO("ISR in this period = "<<double(ptr->m_numOfReceivedData) / double(ptr->m_numOfSentInterest));
     NS_LOG_INFO("retrievalTime in this period = "<<double(ptr->m_sumRetrievalTime.GetMilliSeconds()) / double(ptr->m_numOfReceivedData));
     NS_LOG_INFO("hopCount in this period = "<<double(ptr->m_sumOfHopCount) / double(ptr->m_numOfReceivedData));
   }
   ptr->m_numOfReceivedData=0;
   ptr->m_numOfSentInterest=0;
   ptr->m_sumRetrievalTime=Simulator::Now() -Simulator::Now();
   ptr->m_sumOfHopCount=0;
   ptr->computeMetricsWD.Ping(MilliSeconds(500));
 }
 
 TypeId
 ConsumerZipfMandelbrot::GetTypeId(void)
 {
   static TypeId tid =
     TypeId("ns3::ndn::ConsumerZipfMandelbrot")
       .SetGroupName("Ndn")
       .SetParent<ConsumerCbr>()
       .AddConstructor<ConsumerZipfMandelbrot>()
       .AddAttribute("WatchDog", "",
                                   DoubleValue(500),
                                   MakeDoubleAccessor(&ConsumerZipfMandelbrot::SetWatchDog),
                                   MakeDoubleChecker<double>())
       .AddAttribute("NumberOfContents", "Number of the Contents in total", StringValue("100"),
                     MakeUintegerAccessor(&ConsumerZipfMandelbrot::SetNumberOfContents,
                                          &ConsumerZipfMandelbrot::GetNumberOfContents),
                     MakeUintegerChecker<uint32_t>())
 
       .AddAttribute("q", "parameter of improve rank", StringValue("0.7"),
                     MakeDoubleAccessor(&ConsumerZipfMandelbrot::SetQ,
                                        &ConsumerZipfMandelbrot::GetQ),
                     MakeDoubleChecker<double>())
 
       .AddAttribute("s", "parameter of power", StringValue("0.7"),
                     MakeDoubleAccessor(&ConsumerZipfMandelbrot::SetS,
                                        &ConsumerZipfMandelbrot::GetS),
                     MakeDoubleChecker<double>());
 
   return tid;
 }
 
 ConsumerZipfMandelbrot::ConsumerZipfMandelbrot()
   : m_N(100) // needed here to make sure when SetQ/SetS are called, there is a valid value of N
   , m_q(0.7)
   , m_s(0.7)
 {
   //设置随机数生成器的种子
   //ns3::RngSeedManager::SetSeed(static_cast<unsigned int>(std::time(0)));
   m_seqRng = CreateObject<UniformRandomVariable>();
   // SetNumberOfContents is called by NS-3 object system during the initialization
 }
 
 ConsumerZipfMandelbrot::~ConsumerZipfMandelbrot()
 {
 }
 
 void 
 ConsumerZipfMandelbrot::SetWatchDog(double t)
 {
     if (t > 0)
     {
         computeMetricsWD.Ping(MilliSeconds(t));
         computeMetricsWD.SetFunction(computeMetricsWDCallback);
         computeMetricsWD.SetArguments<ConsumerZipfMandelbrot *>(this);
     }
 }
 
 void
 ConsumerZipfMandelbrot::SetNumberOfContents(uint32_t numOfContents)
 {
   NS_LOG_LOGIC("m_frequency =" << m_frequency);
   NS_LOG_LOGIC("m_randomType =" << m_randomType);
 
   m_N = numOfContents;
 
   NS_LOG_DEBUG(m_q << " and " << m_s << " and " << m_N);
 
   m_Pcum = std::vector<double>(m_N + 1);
 
   m_Pcum[0] = 0.0;
   for (uint32_t i = 1; i <= m_N; i++) {
     m_Pcum[i] = m_Pcum[i - 1] + 1.0 / std::pow(i + m_q, m_s);
   }
 
   for (uint32_t i = 1; i <= m_N; i++) {
     m_Pcum[i] = m_Pcum[i] / m_Pcum[m_N];
     //NS_LOG_LOGIC("Cumulative probability [" << i << "]=" << m_Pcum[i]);
   }
 }
 
 uint32_t
 ConsumerZipfMandelbrot::GetNumberOfContents() const
 {
   return m_N;
 }
 
 void
 ConsumerZipfMandelbrot::SetQ(double q)
 {
   m_q = q;
   SetNumberOfContents(m_N);
 }
 
 double
 ConsumerZipfMandelbrot::GetQ() const
 {
   return m_q;
 }
 
 void
 ConsumerZipfMandelbrot::SetS(double s)
 {
   m_s = s;
   SetNumberOfContents(m_N);
 }
 
 double
 ConsumerZipfMandelbrot::GetS() const
 {
   return m_s;
 }
 
 void
 ConsumerZipfMandelbrot::SendPacket()
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
     if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
       if (m_seq >= m_seqMax) {
         return; // we are totally done
       }
     }
 
     seq = ConsumerZipfMandelbrot::GetNextSeq();
     m_seq++;
   }
 
 
   m_numOfSentInterest ++;
 
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
   uint64_t tagValue = (uint64_t)0 << 48 | (uint64_t)1 << 32 |nodeid;
   interest->setTag(make_shared<ndn::lp::ConsumerIdTag>(tagValue));
   auto tagRead = *(interest->getTag<ndn::lp::ConsumerIdTag>());
   // 提取高16位
   uint16_t highBits = (tagRead >> 48) & 0xFFFF;
   // 提取中16位
   uint16_t middleBits = (tagRead >> 32) & 0xFFFF;
   // 提取低32位
   uint32_t lowBits = tagRead & 0xFFFFFFFF;
   NFD_LOG_DEBUG("Tag value: high16=" << highBits << ", mid16=" << middleBits<< ", low32=" << lowBits);
 
   // NS_LOG_DEBUG ("Requesting Interest: \n" << *interest);
   NS_LOG_DEBUG("> Interest for " << seq << ", Total: " << m_seq << ", face: " << m_face->getId());
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
 
   ConsumerZipfMandelbrot::ScheduleNextPacket();
 }
 
 uint32_t
 ConsumerZipfMandelbrot::GetNextSeq()
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
 ConsumerZipfMandelbrot::ScheduleNextPacket()
 {
 
   if (m_firstTime) {
     m_sendEvent = Simulator::Schedule(Seconds(0.0), &ConsumerZipfMandelbrot::SendPacket, this);
     m_firstTime = false;
   }
   else if (!m_sendEvent.IsRunning())
   { 
     auto delay = m_random->GetValue();
     NS_LOG_DEBUG("m_frequency=" << m_frequency);
     NS_LOG_DEBUG("delay=" << delay);
     m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                       : Seconds(delay),
                                       &ConsumerZipfMandelbrot::SendPacket, this);
   }
 }
 
 void
 ConsumerZipfMandelbrot::OnData(shared_ptr<const Data> data)
 {
 
   App::OnData(data); // tracing inside
 
   ++m_numOfReceivedData;
   
   int hopCount = 0;
   auto hopCountTag = data->getTag<lp::HopCountTag>();
   if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
     hopCount = *hopCountTag;
   }
   NS_LOG_DEBUG("Hop count: " << hopCount);
   m_sumOfHopCount += hopCount;
   NS_LOG_DEBUG("m_sumOfHopCount= " << m_sumOfHopCount);
 }
 
 } /* namespace ndn */
 } /* namespace ns3 */
 