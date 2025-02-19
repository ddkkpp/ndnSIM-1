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

#ifndef NDN_CONSUMER_D2CPA_H
#define NDN_CONSUMER_D2CPA_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-consumer.hpp"
#include "ndn-consumer-cbr.hpp"
#include "ns3/traced-value.h"
#include "ns3/ndnSIM/utils/batches.hpp"
#include "ns3/nstime.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * \brief Ndn application for sending out Interest packets in batches
 */
class ConsumerD2CPA : public ConsumerCbr {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   */
  ConsumerD2CPA();

private:
  virtual void
  StartApplication(); ///< @brief Called at time specified by Start

  void
  AddBatch(uint32_t amount);

  void
  InitializeD2CPA();

  void
  D2CPA();

  virtual void
  SendPacket();

  uint32_t
  GetNextSeq();

protected:
  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */
  virtual void
  ScheduleNextPacket();

private:
  bool m_initial;

  uint32_t m_vMax;//最大速率
  uint32_t m_vStep;//每次速率变化的值
  uint32_t m_vNow;//当前速率
  Time m_tStep;//每次速率变化的时间间隔
  Time m_tNow;//当前时刻距离上一次速率变化后的时间
  uint32_t m_seqMaxA;//最大seq(虽然)
  uint32_t m_range;//seq范围


  Batches m_batches;
  Time m_period;
  Time m_sendTime;
  uint32_t m_sendRate;
  uint32_t m_periodNum;

};

} // namespace ndn
} // namespace ns3

#endif