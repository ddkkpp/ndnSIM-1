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

#ifndef NDN_PRODUCER_REP_H
#define NDN_PRODUCER_REP_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"
#include "ndn-producer.hpp"//别忘了
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/nstime.h"
#include "ns3/ptr.h"

#include "ns3/watchdog.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A simple Interest-sink applia simple Interest-sink application
 *
 * A simple Interest-sink applia simple Interest-sink application,
 * which replying every incoming Interest with Data packet with a specified
 * size and name same as in Interest.cation, which replying every incoming Interest
 * with Data packet with a specified size and name same as in Interest.
 */
class ProducerRep : public Producer {
public:
  static TypeId
  GetTypeId(void);

  ProducerRep();

  friend void pvChangeWDCallback(ProducerRep *ptr);//友元函数可以访问类的非公开成员
  //延迟onInterest1功能
  virtual void
  OnInterest(shared_ptr<const Interest> interest);
  //与producer::onInterest一样，在此重新构造是为了producerRep::onInterest函数能够延迟producerRep::onInterest1功能,producerRep::onInterest无法延迟producer::onInterest
  virtual void
  OnInterest1(shared_ptr<const Interest> interest);
  
  void SetWatchDog(double t);

  Watchdog pvChangeWD;
  int rounds=2;//批量验证轮数
  bool batchTrue=true;
  bool honest=false;
  double batch100=738.367;//us
  double batch10 =1055.162;//us
  double secRtrT=670.0;//概率平均值//us
  int cur_num=0;
  int trueN=0;
  int falseN=0;
  double rep=1;
  double pv=1;
  
  double m_honesty;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_PRODUCER_H
