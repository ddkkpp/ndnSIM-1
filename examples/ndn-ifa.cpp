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
namespace ns3 {

void IFA(Ptr<Node> consumer){
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
    consumerHelper.SetAttribute("Frequency", StringValue("10000")); // 100 interests a second
    consumerHelper.SetAttribute("NumberOfContents", StringValue("1000000"));
    consumerHelper.SetPrefix("/prefix0");
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
  ndnHelper.setCsSize(1000);
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


 Ptr<Node> consumers[5] = {Names::Find<Node>("0"), Names::Find<Node>("1"),
                            Names::Find<Node>("2"), Names::Find<Node>("3"), Names::Find<Node>("4")};
 Ptr<Node> producer[5] = {Names::Find<Node>("6"), Names::Find<Node>("7"),
                            Names::Find<Node>("8"), Names::Find<Node>("9"), Names::Find<Node>("10")};
  

  for (int i = 1; i < 5; i++) {
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
    consumerHelper.SetAttribute("Frequency", StringValue("100")); // 100 interests a second
    consumerHelper.SetAttribute("NumberOfContents", StringValue("100000"));
    consumerHelper.SetPrefix("/prefix" + Names::FindName(consumers[i]));
    consumerHelper.Install(consumers[i]);
  }

  for (int i = 0; i < 5; i++) {
    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetPrefix("/prefix" + Names::FindName(consumers[i]));
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.Install(producer[i]);
    ndnGlobalRoutingHelper.AddOrigins("/prefix" + Names::FindName(consumers[i]), producer[i]);
  }
  

  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();

 //void (*p)(Ptr<Node>, const ndn::Name&, const ndn::Name&)=&ndn::AppHelper::Install;
  Simulator::Schedule(Seconds(5),[consumers]{IFA(consumers[0]);});



  Simulator::Stop(Seconds(50));

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
