/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * This example is based on `ndn-cpa.cpp`, but uses `ns3::RandomPropagationDelayModel`
 * on WiFi access links (consumer/attacker <-> edge router) instead of fixed p2p delay.
 *
 * Notes:
 * - The core topology (routers + producer) is still created via AnnotatedTopologyReader.
 * - The topology files `topo-cpa-basic-A--random-propdelay.txt` and
 *   `topo-cpa-basic-A+-random-propdelay.txt` intentionally omit access links; those
 *   are created here as WiFi links.
 */

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ns3/wifi-module.h"
#include "ns3/propagation-module.h"

#include "ns3/ndnSIM/apps/ndn-consumer-pcon.hpp"
#include "common/global.hpp"

namespace ns3 {

static void
InstallWifiAccessLink(const std::string& leafName, const std::string& routerName,
                      double meanDelayMs, double stddevDelayMs, double boundMs)
{
  Ptr<Node> leaf = Names::Find<Node>(leafName);
  Ptr<Node> router = Names::Find<Node>(routerName);
  NS_ABORT_MSG_IF(leaf == nullptr, "Node not found: " << leafName);
  NS_ABORT_MSG_IF(router == nullptr, "Node not found: " << routerName);

  const double meanS = meanDelayMs / 1000.0;
  const double varianceS2 = std::pow(stddevDelayMs / 1000.0, 2);
  const double boundS = boundMs / 1000.0;

  Ptr<NormalRandomVariable> normal = CreateObject<NormalRandomVariable>();
  normal->SetAttribute("Mean", DoubleValue(meanS));
  normal->SetAttribute("Variance", DoubleValue(varianceS2));
  normal->SetAttribute("Bound", DoubleValue(boundS));

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::RandomPropagationDelayModel", "Variable", PointerValue(normal));
  wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel", "Rss", DoubleValue(-40.0));

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
  phy.SetChannel(wifiChannel.Create());

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211g);
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue("ErpOfdmRate54Mbps"),
                               "ControlMode", StringValue("ErpOfdmRate24Mbps"));

  WifiMacHelper mac;
  mac.SetType("ns3::AdhocWifiMac");

  NodeContainer nodes;
  nodes.Add(router);
  nodes.Add(leaf);
  wifi.Install(phy, mac, nodes);
}

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  if (argc < 5) {
    std::cerr << "Usage: ./waf --run=ndn-cpa-random-propdelay para1 para2 intensity seqRange [muMs sigmaMs boundMs]\n";
    std::cerr << "para1: topo-cpa-basic-A- | topo-cpa-basic-A+\n";
    std::cerr << "para2: rate-static | rate-dynamic\n";
    return 1;
  }

  std::string para1 = argv[1];
  std::string para2 = argv[2];
  double para3 = std::stod(argv[3]);
  uint32_t para4 = std::stoul(argv[4]);

  // Access-link random propagation delay (Normal) in milliseconds.
  // RandomPropagationDelayModel uses the RV output as seconds.
  double muMs = (argc >= 6) ? std::stod(argv[5]) : 15.0;
  double sigmaMs = (argc >= 7) ? std::stod(argv[6]) : 3.0;
  // Bound is applied as |x-mean| <= bound, so setting bound=mu keeps samples >= 0.
  double boundMs = (argc >= 8) ? std::stod(argv[7]) : muMs;

  std::vector<std::string> validPara1 = {"topo-cpa-basic-A-", "topo-cpa-basic-A+"};
  std::vector<std::string> validPara2 = {"rate-static", "rate-dynamic"};

  if (std::find(validPara1.begin(), validPara1.end(), para1) == validPara1.end() ||
      std::find(validPara2.begin(), validPara2.end(), para2) == validPara2.end()) {
    std::cerr << "Invalid parameters.\n";
    return 1;
  }

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/" + para1 + "-random-propdelay.txt");
  topologyReader.Read();

  // Create WiFi access links (consumer/attacker <-> edge router) with RandomPropagationDelayModel.
  for (int i = 0; i <= 3; i++) {
    InstallWifiAccessLink("consumer_normal_" + std::to_string(i), "Node2", muMs, sigmaMs, boundMs);
  }
  for (int i = 4; i <= 9; i++) {
    InstallWifiAccessLink("consumer_normal_" + std::to_string(i), "Node3", muMs, sigmaMs, boundMs);
  }
  for (int i = 10; i <= 17; i++) {
    InstallWifiAccessLink("consumer_normal_" + std::to_string(i), "Node4", muMs, sigmaMs, boundMs);
  }

  if (para1 == "topo-cpa-basic-A-") {
    InstallWifiAccessLink("consumer_malicious_0", "Node2", muMs, sigmaMs, boundMs);
    InstallWifiAccessLink("consumer_malicious_1", "Node3", muMs, sigmaMs, boundMs);
    InstallWifiAccessLink("consumer_malicious_2", "Node4", muMs, sigmaMs, boundMs);
  }
  else if (para1 == "topo-cpa-basic-A+") {
    for (int i = 0; i <= 3; i++) {
      InstallWifiAccessLink("consumer_malicious_" + std::to_string(i), "Node2", muMs, sigmaMs, boundMs);
    }
    for (int i = 4; i <= 9; i++) {
      InstallWifiAccessLink("consumer_malicious_" + std::to_string(i), "Node3", muMs, sigmaMs, boundMs);
    }
    for (int i = 10; i <= 17; i++) {
      InstallWifiAccessLink("consumer_malicious_" + std::to_string(i), "Node4", muMs, sigmaMs, boundMs);
    }
  }

  // Install NDN stack
  ndn::StackHelper ndnHelper;
  ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.setCsSize(100);
  for (int i = 1; i <= 4; i++) {
    ndnHelper.Install(Names::Find<Node>("Node" + std::to_string(i)));
  }

  // Disable CS on producer + consumers
  ndnHelper.setCsSize(0);
  ndnHelper.Install(Names::Find<Node>("Node0"));
  for (int i = 0; i <= 17; i++) {
    ndnHelper.Install(Names::Find<Node>("consumer_normal_" + std::to_string(i)));
  }
  if (para1 == "topo-cpa-basic-A-") {
    for (int i = 0; i <= 2; i++) {
      ndnHelper.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
    }
  }
  if (para1 == "topo-cpa-basic-A+") {
    for (int i = 0; i <= 17; i++) {
      ndnHelper.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
    }
  }

  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  Ptr<Node> producerNode = Names::Find<Node>("Node0");

  // Normal users
  int sum_rate = 0;
  for (int i = 0; i <= 17; i++) {
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
    consumerHelper.SetPrefix("/prefix");
    std::ostringstream oss;
    oss << 500 + 100 * (rand() % 6);
    std::string frequencyValue = oss.str();
    sum_rate += std::stoi(frequencyValue);
    consumerHelper.SetAttribute("Frequency", StringValue(frequencyValue));
    consumerHelper.SetAttribute("Randomize", StringValue("exponential"));
    consumerHelper.SetAttribute("s", StringValue("1"));
    consumerHelper.SetAttribute("NumberOfContents", StringValue("10000"));
    consumerHelper.Install(Names::Find<Node>("consumer_normal_" + std::to_string(i)));
  }

  // Attackers
  if (para1 == "topo-cpa-basic-A-") {
    double rate_attacker = sum_rate * para3 / 3.0;
    double vStep_attacker = 30.0 / 3.0;
    for (int i = 0; i <= 2; i++) {
      ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCPA");
      consumerHelper.SetPrefix("/prefix");
      consumerHelper.SetAttribute("vMax", DoubleValue(rate_attacker));
      if (para2 == "rate-dynamic") {
        consumerHelper.SetAttribute("isDynamic", BooleanValue(true));
      }
      consumerHelper.SetAttribute("vStep", DoubleValue(vStep_attacker));
      consumerHelper.SetAttribute("tStep", TimeValue(Seconds(0.05)));
      consumerHelper.SetAttribute("MaxSeqA", UintegerValue(10000));
      consumerHelper.SetAttribute("range", UintegerValue(para4));
      consumerHelper.SetAttribute("StartTime", TimeValue(Seconds(5)));
      consumerHelper.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
    }
  }

  if (para1 == "topo-cpa-basic-A+") {
    double rate_attacker = sum_rate * para3 / 18.0;
    double vStep_attacker = 30.0 / 18.0;
    for (int i = 0; i <= 17; i++) {
      ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCPA");
      consumerHelper.SetPrefix("/prefix");
      consumerHelper.SetAttribute("vMax", DoubleValue(rate_attacker));
      if (para2 == "rate-dynamic") {
        consumerHelper.SetAttribute("isDynamic", BooleanValue(true));
      }
      consumerHelper.SetAttribute("vStep", DoubleValue(vStep_attacker));
      consumerHelper.SetAttribute("tStep", TimeValue(Seconds(0.05)));
      consumerHelper.SetAttribute("MaxSeqA", UintegerValue(10000));
      consumerHelper.SetAttribute("range", UintegerValue(para4));
      consumerHelper.SetAttribute("StartTime", TimeValue(Seconds(5)));
      consumerHelper.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
    }
  }

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(producerNode);
  ndnGlobalRoutingHelper.AddOrigins("/prefix", producerNode);

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
