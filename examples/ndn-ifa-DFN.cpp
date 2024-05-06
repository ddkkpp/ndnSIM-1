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

void IFA(Ptr<Node> consumer){
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
    consumerHelper.SetAttribute("Frequency", StringValue("5000")); 
    consumerHelper.SetPrefix("/prefix0");
    consumerHelper.Install(consumer);
}


void IFA2(Ptr<Node> consumer, std::string prefix){
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
    consumerHelper.SetAttribute("Frequency", StringValue("600")); 
    consumerHelper.SetPrefix(prefix);
    consumerHelper.Install(consumer);
}
    

int
main(int argc, char* argv[])
{

  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  //topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-ifa.txt");
  topologyReader.SetFileName("/home/dkp/BRITE/DFN.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(1);
  ndnHelper.InstallAll();

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  NodeContainer producerNodes;
  producerNodes.Add(Names::Find<Node>("15"));
  producerNodes.Add(Names::Find<Node>("55"));
  producerNodes.Add(Names::Find<Node>("12"));
  producerNodes.Add(Names::Find<Node>("10"));
  producerNodes.Add(Names::Find<Node>("16"));


 Ptr<Node> consumers[5] = {Names::Find<Node>("5"), Names::Find<Node>("6"),
                            Names::Find<Node>("50"), Names::Find<Node>("49"), Names::Find<Node>("7")};
 Ptr<Node> producer[5] = {Names::Find<Node>("15"), Names::Find<Node>("55"),
                            Names::Find<Node>("12"), Names::Find<Node>("10"), Names::Find<Node>("16")};
  
 
  // for (int i = 1; i < 5; i++) {
  //   //正常用户pcon
  //   ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPcon");
  //   consumerHelper.SetAttribute("CcAlgorithm",EnumValue(ndn::CcAlgorithm::CUBIC));
  //   consumerHelper.SetAttribute("Beta",DoubleValue(0.5));//StringValue(std::to_string(0.5))
  //   //consumerHelper.SetAttribute("CubicBeta",DoubleValue(0.8));
  //   consumerHelper.SetPrefix("/prefix" + Names::FindName(consumers[i]));
  //   consumerHelper.SetAttribute("ReactToCongestionMarks",BooleanValue(true));
  //   consumerHelper.Install(consumers[i]);

  //   //正常用户contant rate
  //   // ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  //   // consumerHelper.SetAttribute("Frequency", StringValue("100")); 
  //   // consumerHelper.SetPrefix("/prefix" + Names::FindName(consumers[i]));
  //   // consumerHelper.Install(consumers[i]);
  // }

    //正常用户pcon
    // ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPcon");
    // consumerHelper.SetAttribute("CcAlgorithm",EnumValue(ndn::CcAlgorithm::CUBIC));
    // consumerHelper.SetAttribute("Beta",DoubleValue(0.5));//StringValue(std::to_string(0.5))
    // //consumerHelper.SetAttribute("CubicBeta",DoubleValue(0.8));
    // consumerHelper.SetPrefix("/prefix1");
    // consumerHelper.SetAttribute("ReactToCongestionMarks",BooleanValue(true));
    // consumerHelper.Install(Names::Find<Node>("5"));


    //正常用户contant rate
    ndn::AppHelper consumerHelper1("ns3::ndn::ConsumerCbr");
    consumerHelper1.SetAttribute("Frequency", StringValue("200")); 
    consumerHelper1.SetPrefix("/prefix1");
    consumerHelper1.Install(Names::Find<Node>("5"));
    consumerHelper1.SetPrefix("/prefix2");
    consumerHelper1.Install(Names::Find<Node>("6"));
    consumerHelper1.SetPrefix("/prefix3");
    consumerHelper1.Install(Names::Find<Node>("50"));

    //攻击者
    ndn::AppHelper consumerHelper2("ns3::ndn::ConsumerCbr");
    consumerHelper2.SetAttribute("StartTime", TimeValue(Seconds(5)));
    //consumerHelper1.SetAttribute("StopTime", TimeValue(Seconds(35.01)));
    consumerHelper2.SetAttribute("Frequency", StringValue("5000")); 
    consumerHelper2.SetPrefix("/prefix4");
    consumerHelper2.Install(Names::Find<Node>("49"));
    // consumerHelper2.SetPrefix("/prefix5");
    // consumerHelper2.Install(Names::Find<Node>("7"));

//正常发布商
  for (int i = 0; i < 3; i++) {
    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetPrefix("/prefix" + std::to_string(i+1));
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.Install(producer[i]);
    ndnGlobalRoutingHelper.AddOrigins("/prefix" + std::to_string(i+1), producer[i]);
  }
  
  //高速延迟2000ms发布商
    ndn::AppHelper producerHelper("ns3::ndn::ProducerDelay");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.SetAttribute("Delay", TimeValue(MilliSeconds(2000)));
    producerHelper.SetPrefix("/prefix4");
    producerHelper.Install(producer[3]);
    ndnGlobalRoutingHelper.AddOrigins("/prefix4", producer[3]);
    producerHelper.SetPrefix("/prefix5");
    producerHelper.Install(producer[4]);
    ndnGlobalRoutingHelper.AddOrigins("/prefix5", producer[4]);

  //低速延迟1900ms发布商
    // ndn::AppHelper producerHelper("ns3::ndn::ProducerDelay");
    // producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    // producerHelper.SetAttribute("Delay", TimeValue(MilliSeconds(1900)));
    // producerHelper.SetPrefix("/prefix4");
    // producerHelper.Install(producer[3]);
    // ndnGlobalRoutingHelper.AddOrigins("/prefix4", producer[3]);
    // producerHelper.SetPrefix("/prefix5");
    // producerHelper.Install(producer[4]);
    // ndnGlobalRoutingHelper.AddOrigins("/prefix5", producer[4]);
  

  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();

 //void (*p)(Ptr<Node>, const ndn::Name&, const ndn::Name&)=&ndn::AppHelper::Install;
 //一个高速攻击者
 // Simulator::Schedule(Seconds(5),[consumers]{IFA(consumers[0]);});
  //两个低速攻击者
  // Simulator::Schedule(MilliSeconds(5000),[consumers]{IFA2(consumers[0], "/prefix0");});
  // Simulator::Schedule(MilliSeconds(5001),[consumers]{IFA2(consumers[5], "/prefix5");});



  Simulator::Stop(Seconds(15.01));

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
