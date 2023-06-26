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
#include "ndn-producer-rep.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>
#include<iostream>

NS_LOG_COMPONENT_DEFINE("ndn.ProducerRep");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ProducerRep);

void pvChangeWDCallback(ProducerRep *ptr)
{   ptr->rounds=1;
    double rep_temp=ptr->rep;
    ptr->rep=pow((double(ptr->trueN+1))/double((ptr->trueN+ptr->falseN+2)),0.1);
    if(ptr->rep>(double(17)/double(18))) ptr->pv=0.1;
    else ptr->pv=1-1/(20*(1-ptr->rep));
    if(((abs(rep_temp-ptr->rep)/ptr->rep)<0.05)&&(ptr->rep>(double(17)/double(18)))) ptr->honest=true;
    else ptr->honest=false;
    ptr->pvChangeWD.Ping(MilliSeconds(10));
    std::cout<<Simulator::Now ().ToDouble (Time::S)<<"\ttrueN"<<ptr->trueN<<"\tfalseN"<<ptr->falseN<<"\t"<<ptr->rep<<"\t"<<ptr->pv<<std::endl;
}

TypeId
ProducerRep::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ProducerRep")
      .SetGroupName("Ndn")
      .SetParent<Producer>()
      .AddConstructor<ProducerRep>()
      .AddAttribute("honesty", "honesty degree for the specific producer", DoubleValue(1),
                    MakeDoubleAccessor(&ProducerRep::m_honesty), MakeDoubleChecker<double>())           
      .AddAttribute("WatchDog", "",
                                  DoubleValue(1),
                                  MakeDoubleAccessor(&ProducerRep::SetWatchDog),
                                  MakeDoubleChecker<double>());
                                  
  return tid;
}

ProducerRep::ProducerRep()
  : m_honesty(1)
{
}

void ProducerRep::SetWatchDog(double t)
{
    if (t > 0)
    {
        pvChangeWD.Ping(Seconds(t));
        pvChangeWD.SetFunction(pvChangeWDCallback);
        pvChangeWD.SetArguments<ProducerRep *>(this);
    }
}

void
ProducerRep::OnInterest1(shared_ptr<const Interest> interest)
{
  Producer::OnInterest(interest);
}

void
ProducerRep::OnInterest(shared_ptr<const Interest> interest)
{ 
  //以m_honesty概率产生真包
  int r=rand()%1000;
 // std::cout<<"r= "<<r<<"\t";
  if(r>=1000*m_honesty){
      batchTrue=false;
      //std::cout<<"假";
  }
  else{
   // std::cout<<"真";
  }
  cur_num++;
  if(cur_num==10){//每十个包为一批，判断该批真假，计入真假数量
      if(batchTrue)
      {
        trueN++;
        //std::cout<<"\t"<<"batchtrue"<<std::endl;
      }
      else 
      {
        falseN++;
        //std::cout<<"\t"<<"batchfalse"<<std::endl;
      }
      cur_num=0;
      batchTrue=true;
  }
  if(rounds==2){//初始先100再10
      Simulator::Schedule(MicroSeconds(batch10+batch100),&ProducerRep::OnInterest1,this,interest);
  }
  else if(honest){//若诚实则100每批
      int rr=rand()%1000;
      if(rr>1000*pv) Simulator::ScheduleNow(&ProducerRep::OnInterest1,this,interest);//1-pv概率直接通过不验证
      else Simulator::Schedule(MicroSeconds(batch100+secRtrT),&ProducerRep::OnInterest1,this,interest);
  }
  else{
      int rr=rand()%1000;
      if(rr>1000*pv) Simulator::ScheduleNow(&ProducerRep::OnInterest1,this,interest);//1-pv概率直接通过不验证
      else Simulator::Schedule(MicroSeconds(batch10+secRtrT),&ProducerRep::OnInterest1,this,interest);
  }
}

} // namespace ndn
} // namespace ns3
