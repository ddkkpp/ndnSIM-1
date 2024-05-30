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
namespace ns3 {


int
main(int argc, char* argv[])
{
  // Setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("100Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("10p"));

  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("/home/dkp/BRITE/1.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(800);
  ndnHelper.InstallAll();

  // Set BestRoute strategy
  //ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  //ndn::StrategyChoiceHelper::Install(grid.GetNode(0,0), "/", "/localhost/nfd/strategy/multicast/%FD%03");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  NodeContainer producerNodes;
  producerNodes.Add(Names::Find<Node>("6"));
  producerNodes.Add(Names::Find<Node>("50"));


  NodeContainer consumerNodes;
  //consumerNodes.Add(Names::Find<Node>("15"));
  // consumerNodes.Add(Names::Find<Node>("55"));
  consumerNodes.Add(Names::Find<Node>("8"));

  // Install NDN applications
  std::string prefix = "/prefix";

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfFdbk");
  consumerHelper.SetPrefix(prefix);
  consumerHelper.SetAttribute("Frequency", StringValue("100")); // 10000 interests a second//不能超过带宽
  consumerHelper.SetAttribute("NumberOfContents",StringValue("100000"));
  consumerHelper.SetAttribute("q",StringValue("1"));
  consumerHelper.SetAttribute("s",StringValue("0.9"));
  consumerHelper.Install(consumerNodes);


  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(producerNodes);

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins(prefix, producerNodes);
  

  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();

 void (*p)(Ptr<Node>, const ndn::Name&, const ndn::Name&)=&ndn::StrategyChoiceHelper::Install;
Simulator::Schedule(Seconds(10),p, Names::Find<Node>("49"), "/","/localhost/nfd/strategy/mal-best-route");
//Simulator::Schedule(Seconds(10),p, Names::Find<Node>("40"), "/","/localhost/nfd/strategy/mal-best-route");

 void (*q)(Ptr<Node>, const ndn::Name&, const ndn::Name&)=&ndn::StrategyChoiceHelper::Install;
 //ELC
//Simulator::Schedule(Seconds(1),q, Names::Find<Node>("22"), "/","/localhost/nfd/strategy/verify-multicast");
// ndn::StrategyChoiceHelper::Install(Names::Find<Node>("41"), "/","/localhost/nfd/strategy/verify-best-route");
// ndn::StrategyChoiceHelper::Install(Names::Find<Node>("22"), "/","/localhost/nfd/strategy/verify-best-route");
 //others
 Simulator::Schedule(Seconds(10),q, Names::Find<Node>("22"), "/","/localhost/nfd/strategy/multicast");

 ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");

  Simulator::Stop(Seconds(50.0));

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
