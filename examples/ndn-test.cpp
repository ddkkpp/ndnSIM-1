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

/**
 * This scenario simulates a grid topology (using PointToPointGrid module)
 *
 * (consumer) -- ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) -- (producer)
 *
 * All links are 1Mbps with propagation 10ms delay.
 *
 * FIB is populated using NdnGlobalRoutingHelper.
 *
 * Consumer requests data from producer with frequency 100 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-grid
 */

int
main(int argc, char* argv[])
{
  // Setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("10p"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating 3x3 topology
  PointToPointHelper p2p;
  PointToPointGridHelper grid(3, 3, p2p);
  grid.BoundingBox(100, 100, 200, 200);

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(100);
  ndnHelper.InstallAll();

  // Set BestRoute strategy
  //ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  //ndn::StrategyChoiceHelper::Install(grid.GetNode(0,0), "/", "/localhost/nfd/strategy/multicast/%FD%03");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  Ptr<Node> producer = grid.GetNode(2, 2);
  NodeContainer consumerNodes;
  consumerNodes.Add(grid.GetNode(0, 0));

  // Install NDN applications
  std::string prefix = "/prefix";

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfFdbk");
  consumerHelper.SetPrefix(prefix);
  consumerHelper.SetAttribute("Frequency", StringValue("100")); // 10000 interests a second//不能超过带宽
  consumerHelper.SetAttribute("NumberOfContents",StringValue("1000"));
  consumerHelper.SetAttribute("q",StringValue("1"));
  consumerHelper.SetAttribute("s",StringValue("0.1"));
  consumerHelper.Install(consumerNodes);


  // ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // consumerHelper.SetPrefix(prefix);
  // consumerHelper.SetAttribute("Frequency", StringValue("100")); 
  // consumerHelper.Install(consumerNodes);

  // ndn::AppHelper consumerHelper("ns3::ndn::ConsumerBatches");
  // consumerHelper.SetPrefix(prefix);
  // consumerHelper.SetAttribute("Batches", StringValue("1s 100 3s 100"));
  // consumerHelper.Install(consumerNodes);


  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(producer);

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins(prefix, producer);
  

  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();


//以下搞了半天，因为在schedule里面使用StrategyChoiceHelper::Install总是会出现unresolved overloaded function type>error
//Simulator::Schedule(Seconds(5.0),&ndn::StrategyChoiceHelper::InstallAll, "/", "/localhost/nfd/strategy/multicast/%FD%03");//InstallAll就可以
  //Simulator::Schedule(Seconds(5.0),[=](Ptr<Node> node, const ndn::Name& namePrefix, const ndn::Name& strategy){return this->ndn::StrategyChoiceHelper::Install(node,namePrefix,strategy);}, grid.GetNode(0,1), "/", "/localhost/nfd/strategy/multicast/%FD%03");
//Simulator::Schedule(Seconds(5.0),&ndn::StrategyChoiceHelper::Install, grid.GetNode(0,1), "/", "/localhost/nfd/strategy/multicast/%FD%03");
 //Simulator::Schedule(Seconds(5.0),ndn::StrategyChoiceHelper::Install<nfd::fw::MulticastStrategy>, grid.GetNode(0,1), "/");//unresolved overloaded function type>
// Simulator::Schedule<p, Ptr<Node>, const ndn::Name&, const ndn::Name&>
//                   (Seconds(5.0),ndn::StrategyChoiceHelper::Install, grid.GetNode(0,1), "/","/localhost/nfd/strategy/multicast/%FD%03");

 void (*p)(Ptr<Node>, const ndn::Name&, const ndn::Name&)=&ndn::StrategyChoiceHelper::Install;
 Simulator::Schedule(Seconds(1),p, grid.GetNode(0,2), "/","/localhost/nfd/strategy/mal-best-route");

 void (*q)(Ptr<Node>, const ndn::Name&, const ndn::Name&)=&ndn::StrategyChoiceHelper::Install;
 //Simulator::Schedule(Seconds(3),q, grid.GetNode(0,1), "/","/localhost/nfd/strategy/verify-best-route");

    //Simulator::Schedule(Seconds(1.0), ndn::LinkControlHelper::FailLink, grid.GetNode(0,1), grid.GetNode(1,1));
  //  Simulator::Schedule(Seconds(1.0), ndn::LinkControlHelper::FailLink, grid.GetNode(0,1), grid.GetNode(0,2));
    //Simulator::Schedule(Seconds(1.0), ndn::LinkControlHelper::FailLink, grid.GetNode(0,0), grid.GetNode(0,1));
    //Simulator::Schedule(Seconds(1.0), ndn::LinkControlHelper::FailLink, Names::Find<Node>("4"), Names::Find<Node>("7"));

 ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");

  Simulator::Stop(Seconds(10.0));

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
