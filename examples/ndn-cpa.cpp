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
#include <iostream>
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

  // std::cout << "命令行参数数量: " << argc << std::endl;
  // std::cout << "所有参数: ";
  // for (int i = 0; i < argc; i++) {
  //   std::cout << argv[i] << " ";
  // }
  // std::cout << std::endl;

  // if (argc < 4) {
  //   std::cerr << "Usage: ./waf --run=ndn-cpa para1 para2 para3\n";
  //   std::cerr << "para1 is topo-cpa-basic-A-, topo-cpa-basic-A+, topo-cpa-DFN-A-, topo-cpa-DFN-A+\n";
  //   std::cerr << "para2 is rate_high, rate_medium, rate_dynamic\n";
  //   std::cerr << "para3 is LDA, FLA\n";
  //   return 1;
  // }

  // std::string para1 = argv[1];
  // std::string para2 = argv[2];
  // std::string para3 = argv[3];

  // std::vector<std::string> validPara1 = {"topo-cpa-basic-A-", "topo-cpa-basic-A+", "topo-cpa-DFN-A-", "topo-cpa-DFN-A+"};
  // std::vector<std::string> validPara2 = {"rate_high", "rate_medium", "rate_dynamic"};
  // std::vector<std::string> validPara3 = {"LDA", "FLA"};

  // if (std::find(validPara1.begin(), validPara1.end(), para1) == validPara1.end() ||
  //     std::find(validPara2.begin(), validPara2.end(), para2) == validPara2.end() ||
  //     std::find(validPara3.begin(), validPara3.end(), para3) == validPara3.end()) {
  //   std::cerr << "Invalid parameters.\n";
  //   std::cerr << "Usage: ./waf --run=ndn-cpa para1 para2 para3\n";
  //   std::cerr << "para1 is topo-cpa-basic-A-, topo-cpa-basic-A+, topo-cpa-DFN-A-, topo-cpa-DFN-A+\n";
  //   std::cerr << "para2 is rate_high, rate_medium, rate_dynamic\n";
  //   std::cerr << "para3 is LDA, FLA\n";
  //   return 1;
  // }

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-cpa-basic-A-.txt");
  //topologyReader.SetFileName("src/ndnSIM/examples/topologies/" + para1 + ".txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.setCsSize(100);
  for(int i = 0; i <= 4; i++){
    ndnHelper.Install(Names::Find<Node>("Node" + std::to_string(i)));
  }
  //消费者节点要禁用缓存，不然兴趣包在消费者节点的缓存就满足了，不会转发到边缘节点
  ndnHelper.setCsSize(0);
  for (int i = 0; i <= 17; i++) {
     ndnHelper.Install(Names::Find<Node>("consumer_normal_"+std::to_string(i)));
  }
  for (int i = 0; i <= 2; i++) {
     ndnHelper.Install(Names::Find<Node>("consumer_malicious_"+std::to_string(i)));
  }
  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  Ptr<Node> producerNode = Names::Find<Node>("Node0");
  

  //正常用户
  for (int i = 0; i <= 17; i++) {
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
    consumerHelper.SetPrefix("/prefix");
    std::ostringstream oss;
    oss << 500 + 100 * (rand() % 6);
    std::string frequencyValue = oss.str();
    consumerHelper.SetAttribute("Frequency", StringValue(frequencyValue)); 
    consumerHelper.SetAttribute("Randomize", StringValue("exponential"));
    consumerHelper.SetAttribute("s", StringValue("1"));//每设置一次s或q或NumberOfContents，都会调用SetNumberOfContents进行流行度计算
    consumerHelper.SetAttribute("NumberOfContents", StringValue("10000"));
    //consumerHelper.Install(normal_consumers[i]);
    consumerHelper.Install(Names::Find<Node>("consumer_normal_"+std::to_string(i)));
  }

  //攻击者
  for (int i = 0; i <= 2; i++) {
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCPA");
    consumerHelper.SetPrefix("/prefix");
    //这个是对所有内容的请求速率之和，后续可以乘以内容数量，使得每个内容的请求速率较大。
    //现在一个攻击者对单个内容的速率是200/50(vMax/range),而活动水平最大的正常用户对最流行内容的速率是200*0.06（q=0.7,s=1.0时）。
    consumerHelper.SetAttribute("vMax", UintegerValue(1000)); 
    consumerHelper.SetAttribute("vStep", UintegerValue(100));
    consumerHelper.SetAttribute("tStep", TimeValue(Seconds(0.05)));
    consumerHelper.SetAttribute("MaxSeqA", UintegerValue(10000));
    consumerHelper.SetAttribute("range", UintegerValue(50));
    consumerHelper.SetAttribute("StartTime", TimeValue(Seconds(15)));//攻击时刻
    //consumerHelper.Install(malicious_consumers[i]);
    consumerHelper.Install(Names::Find<Node>("consumer_malicious_"+std::to_string(i)));
  }


    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetPrefix("/prefix");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.Install(producerNode);
    ndnGlobalRoutingHelper.AddOrigins("/prefix", producerNode);
  



  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();



  Simulator::Stop(Seconds(30.01));

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