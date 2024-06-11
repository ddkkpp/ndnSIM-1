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
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-ifa.txt");
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
  producerNodes.Add(Names::Find<Node>("6"));
  producerNodes.Add(Names::Find<Node>("7"));
  producerNodes.Add(Names::Find<Node>("8"));
  producerNodes.Add(Names::Find<Node>("9"));
  producerNodes.Add(Names::Find<Node>("10"));
  producerNodes.Add(Names::Find<Node>("12"));


 Ptr<Node> consumers[6] = {Names::Find<Node>("0"), Names::Find<Node>("1"),
                            Names::Find<Node>("2"), Names::Find<Node>("3"), Names::Find<Node>("4"), Names::Find<Node>("11")};
 Ptr<Node> producer[6] = {Names::Find<Node>("6"), Names::Find<Node>("7"),
                            Names::Find<Node>("8"), Names::Find<Node>("9"), Names::Find<Node>("10"), Names::Find<Node>("12")};
  
 
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

//两恒速用户，两pcon用户
  for (int i = 1; i < 3; i++) {
    //正常用户pcon
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPcon");
    consumerHelper.SetAttribute("CcAlgorithm",EnumValue(ndn::CcAlgorithm::CUBIC));
    consumerHelper.SetAttribute("Beta",DoubleValue(0.5));//StringValue(std::to_string(0.5))
    //consumerHelper.SetAttribute("CubicBeta",DoubleValue(0.8));
    consumerHelper.SetPrefix("/prefix" + Names::FindName(consumers[i]));
    consumerHelper.SetAttribute("ReactToCongestionMarks",BooleanValue(true));
    consumerHelper.Install(consumers[i]);
  }

    //正常用户contant rate
    ndn::AppHelper consumerHelper2("ns3::ndn::ConsumerCbr");
    consumerHelper2.SetAttribute("Frequency", StringValue("100")); 
    consumerHelper2.SetPrefix("/prefix" + Names::FindName(consumers[3]));
    consumerHelper2.Install(consumers[3]);
    consumerHelper2.SetAttribute("Frequency", StringValue("150")); 
    consumerHelper2.SetPrefix("/prefix" + Names::FindName(consumers[4]));
    consumerHelper2.Install(consumers[4]);

    //攻击者
    ndn::AppHelper consumerHelper1("ns3::ndn::ConsumerCbr");
    //consumerHelper1.SetAttribute("StopTime", TimeValue(Seconds(35.01)));
    //高速
    // consumerHelper1.SetAttribute("StartTime", TimeValue(Seconds(5)));
    // consumerHelper1.SetAttribute("Frequency", StringValue("5000")); 
    // consumerHelper1.SetPrefix("/prefix0");
    // consumerHelper1.Install(consumers[0]);
    //低速
    // consumerHelper1.SetAttribute("StartTime", TimeValue(Seconds(5)));
    // consumerHelper1.SetAttribute("Frequency", StringValue("600")); 
    // consumerHelper1.SetPrefix("/prefix0");
    // consumerHelper1.Install(consumers[0]);
    // consumerHelper1.SetAttribute("StartTime", TimeValue(Seconds(5.001)));
    // consumerHelper1.SetPrefix("/prefix5");
    // consumerHelper1.Install(consumers[5]);
    //m-cifa
    ndn::AppHelper consumerHelper3("ns3::ndn::ConsumerPulse");
    consumerHelper3.SetAttribute("StartTime", TimeValue(Seconds(5)));
    consumerHelper3.SetAttribute("period", TimeValue(Seconds(1.5))); 
    consumerHelper3.SetAttribute("sendTime", TimeValue(Seconds(1))); 
    consumerHelper3.SetAttribute("sendRate", UintegerValue(750)); 
    consumerHelper3.SetAttribute("periodNum", UintegerValue(20)); 
    consumerHelper3.SetPrefix("/prefix0");
    consumerHelper3.Install(consumers[0]);
    consumerHelper3.SetAttribute("StartTime", TimeValue(Seconds(5.001)));
    consumerHelper3.SetPrefix("/prefix5");
    consumerHelper3.Install(consumers[5]);

  //正常发布商
  for (int i = 1; i < 5; i++) {
    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetPrefix("/prefix" + Names::FindName(consumers[i]));
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.Install(producer[i]);
    ndnGlobalRoutingHelper.AddOrigins("/prefix" + Names::FindName(consumers[i]), producer[i]);
  }
  
    
  //高速延迟2000ms发布商
    // ndn::AppHelper producerHelper("ns3::ndn::ProducerDelay");
    // producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    // producerHelper.SetAttribute("Delay", TimeValue(MilliSeconds(2000)));
    // producerHelper.SetPrefix("/prefix" + Names::FindName(consumers[0]));
    // producerHelper.Install(producer[0]);
    // ndnGlobalRoutingHelper.AddOrigins("/prefix" + Names::FindName(consumers[0]), producer[0]);
    // producerHelper.SetPrefix("/prefix5");
    // producerHelper.Install(producer[5]);
    // ndnGlobalRoutingHelper.AddOrigins("/prefix5", producer[5]);

  //低速延迟1900ms发布商
    // ndn::AppHelper producerHelper("ns3::ndn::ProducerDelay");
    // producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    // producerHelper.SetAttribute("Delay", TimeValue(MilliSeconds(1900)));
    // producerHelper.SetPrefix("/prefix" + Names::FindName(consumers[0]));
    // producerHelper.Install(producer[0]);
    // ndnGlobalRoutingHelper.AddOrigins("/prefix" + Names::FindName(consumers[0]), producer[0]);
    // producerHelper.SetPrefix("/prefix5");
    // producerHelper.Install(producer[5]);
    // ndnGlobalRoutingHelper.AddOrigins("/prefix5", producer[5]);

    //m-cifa延迟发布商
    ndn::AppHelper producerHelper("ns3::ndn::ProducerDelay");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.SetAttribute("Delay", TimeValue(MilliSeconds(1300)));
    producerHelper.SetPrefix("/prefix" + Names::FindName(consumers[0]));
    producerHelper.Install(producer[0]);
    ndnGlobalRoutingHelper.AddOrigins("/prefix" + Names::FindName(consumers[0]), producer[0]);
    producerHelper.SetPrefix("/prefix5");
    producerHelper.Install(producer[5]);
    ndnGlobalRoutingHelper.AddOrigins("/prefix5", producer[5]);


  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();

 //void (*p)(Ptr<Node>, const ndn::Name&, const ndn::Name&)=&ndn::AppHelper::Install;
 //一个高速攻击者
 // Simulator::Schedule(Seconds(5),[consumers]{IFA(consumers[0]);});
  //两个低速攻击者
  // Simulator::Schedule(MilliSeconds(5000),[consumers]{IFA2(consumers[0], "/prefix0");});
  // Simulator::Schedule(MilliSeconds(5001),[consumers]{IFA2(consumers[5], "/prefix5");});



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
