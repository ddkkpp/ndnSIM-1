/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Async normal consumer join version of ndn-cpa.cpp
 */

#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"
#include "common/global.hpp"

namespace ns3 {

static void
installNormalConsumerAt(const std::string& nodeName, const Time& startTime, int& sum_rate)
{
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
  consumerHelper.SetAttribute("StartTime", TimeValue(startTime));

  consumerHelper.Install(Names::Find<Node>(nodeName));
}

static void
PrintCliArgs(int argc, char* argv[], const char* progName)
{
  std::cout << "[" << progName << "] argc=" << argc << "\n";
  std::cout << "[" << progName << "] argv:";
  for (int i = 0; i < argc; ++i) {
    std::cout << " " << argv[i];
  }
  std::cout << std::endl;
}

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  PrintCliArgs(argc, argv, "ndn-cpa-async-normal");

  if (argc < 5) {
    std::cerr << "Usage: ./waf --run=ndn-cpa-async-normal para1 para2 para3 para4\n";
    std::cerr << "para1: topo-cpa-basic-A-, topo-cpa-basic-A+, topo-cpa-DFN-A-, topo-cpa-DFN-A+\n";
    std::cerr << "para2: rate-static, rate-dynamic\n";
    std::cerr << "para3: intensity\n";
    std::cerr << "para4: seq range\n";
    return 1;
  }

  std::string para1 = argv[1];
  std::string para2 = argv[2];
  double para3 = std::stod(argv[3]);
  uint32_t para4 = std::stoul(argv[4]);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/" + para1 + ".txt");
  topologyReader.Read();

  if (para1 == "topo-cpa-basic-A-" || para1 == "topo-cpa-basic-A+") {
    ndn::StackHelper ndnHelper;
    ndnHelper.setPolicy("nfd::cs::lru");
    ndnHelper.setCsSize(100);
    for (int i = 1; i <= 4; i++) {
      ndnHelper.Install(Names::Find<Node>("Node" + std::to_string(i)));
    }

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

    // -------------------------
    // 正常用户异步加入网络（按你给的分组）
    // 0s: 0 1 4 5 6 10 11 12 13
    // 1s: 2 7 14 15
    // 2s: 3 8 9 16 17
    // -------------------------
    int sum_rate = 0;

    const std::vector<int> g0 = {0, 1, 4, 5, 6, 10, 11, 12, 13};
    const std::vector<int> g1 = {2, 7, 14, 15};
    const std::vector<int> g2 = {3, 8, 9, 16, 17};

    for (int i : g0) {
      installNormalConsumerAt("consumer_normal_" + std::to_string(i), Seconds(0.0), sum_rate);
    }
    for (int i : g1) {
      installNormalConsumerAt("consumer_normal_" + std::to_string(i), Seconds(1.0), sum_rate);
    }
    for (int i : g2) {
      installNormalConsumerAt("consumer_normal_" + std::to_string(i), Seconds(2.0), sum_rate);
    }

    // 恶意用户（保持与你 ndn-cpa.cpp 一致的启动时间：5s）
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
  }

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
