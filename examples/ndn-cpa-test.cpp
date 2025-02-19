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

// ndn-grid.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/strategy.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/multicast-strategy.hpp"
#include "common/global.hpp"
#include "ns3/ndnSIM/apps/ndn-consumer-pcon.hpp"
namespace ns3 {   

int
main(int argc, char* argv[])
{

  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-cpa.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(100);
  ndnHelper.setPolicy("nfd::cs::lru");
  for(int i = 0; i <= 1; i++){
    ndnHelper.Install(Names::Find<Node>(std::to_string(i)));
  }
  //消费者节点和（或）边缘节点要禁用缓存，不然兴趣包在消费者节点的缓存就满足了，不会转发到边缘节点
  ndnHelper.setCsSize(0);
  for(int i = 2; i <= 12; i++){
    ndnHelper.Install(Names::Find<Node>(std::to_string(i)));
  }
  

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  Ptr<Node> producerNode = Names::Find<Node>("0");


 Ptr<Node> consumers[10] = {Names::Find<Node>("3"), Names::Find<Node>("4"),
                            Names::Find<Node>("5"), Names::Find<Node>("6"), Names::Find<Node>("7"), 
                            Names::Find<Node>("8"), Names::Find<Node>("9"), Names::Find<Node>("10"), 
                            Names::Find<Node>("11"), Names::Find<Node>("12")};
  

  //正常用户
  for (int i = 0; i < 5; i++) {
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
    consumerHelper.SetPrefix("/prefix");
    std::ostringstream oss;
    oss << 500 + 100 * i;
    std::string frequencyValue = oss.str();
    consumerHelper.SetAttribute("Frequency", StringValue(frequencyValue)); 
    consumerHelper.SetAttribute("Randomize", StringValue("exponential"));
    consumerHelper.SetAttribute("s", StringValue("1.0"));//每设置一次s或q或NumberOfContents，都会调用SetNumberOfContents进行流行度计算
    consumerHelper.SetAttribute("NumberOfContents", StringValue("10000"));
    consumerHelper.Install(consumers[i]);
  }

  //攻击者
  for (int i = 5; i < 10; i++) {
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerD2CPA");
    consumerHelper.SetPrefix("/prefix");
    //这个是对所有内容的请求速率之和，后续可以乘以内容数量，使得每个内容的请求速率较大。
    //现在一个攻击者对单个内容的速率是200/50(vMax/range),而活动水平最大的正常用户对最流行内容的速率是200*0.06（q=0.7,s=1.0时）。
    consumerHelper.SetAttribute("vMax", UintegerValue(1000)); 
    consumerHelper.SetAttribute("vStep", UintegerValue(100));
    consumerHelper.SetAttribute("tStep", TimeValue(Seconds(0.05)));
    consumerHelper.SetAttribute("MaxSeqA", UintegerValue(10000));
    consumerHelper.SetAttribute("range", UintegerValue(50));
    consumerHelper.SetAttribute("StartTime", TimeValue(Seconds(5)));
    consumerHelper.Install(consumers[i]);
  }


    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetPrefix("/prefix");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.Install(producerNode);
    ndnGlobalRoutingHelper.AddOrigins("/prefix", producerNode);
  



  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();



  Simulator::Stop(Seconds(20));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}