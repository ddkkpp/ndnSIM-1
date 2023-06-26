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
#include <math.h>
#include "ndn-producer-single.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>
#include "ns3/nstime.h"

NS_LOG_COMPONENT_DEFINE("ndn.ProducerSingle");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ProducerSingle);

TypeId
ProducerSingle::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ProducerSingle")
      .SetGroupName("Ndn")
      .SetParent<Producer>()
      .AddConstructor<ProducerSingle>();
  return tid;
}


  
void
ProducerSingle::OnInterest1(shared_ptr<const Interest> interest)
{  
    Producer::OnInterest(interest);
}

void
ProducerSingle::OnInterest(shared_ptr<const Interest> interest)
{  
  Simulator::Schedule(MicroSeconds(1130), &ProducerSingle::OnInterest1, this, interest);//single4100 batch1130
}

} // namespace ndn
} // namespace ns3
