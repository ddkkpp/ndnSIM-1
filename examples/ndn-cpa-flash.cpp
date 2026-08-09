/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * ndn-cpa-flash.cpp
 *
 * ndn-cpa + flash window:
 * - baseline：正常用户全时段 Zipf 访问 /prefix，NumberOfContents=10000
 * - flash：在 [3s,4s] 窗口，正常用户额外叠加一组 Zipf 请求（更高频率+可控内容区间）
 * - CPA：保留 ndn-cpa 的攻击者参数与 5s 开始攻击逻辑
 */

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

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

  PrintCliArgs(argc, argv, "ndn-cpa-flash");

  if (argc < 5) {
    std::cerr << "Usage: ./waf --run=ndn-cpa-flash topo rateMode intensity range [flashMultiplier] [flashNumberOfContents] [flashStartSeq]\n";
    std::cerr << "  topo: topo-cpa-basic-A- | topo-cpa-basic-A+\n";
    std::cerr << "  rateMode: rate-static | rate-dynamic\n";
    std::cerr << "  intensity: CPA intensity (same as ndn-cpa)\n";
    std::cerr << "  range: CPA seq range (same as ndn-cpa)\n";
    std::cerr << "  flashMultiplier (optional, default 0.5): overlay rate multiplier for each normal consumer\n";
    std::cerr << "  flashNumberOfContents (optional, default 5000): overlay content count N\n";
    std::cerr << "  flashStartSeq (optional, default 20): overlay starting seq (>=1 recommended)\n";
    return 1;
  }

  if (argc > 8) {
    std::cerr << "Too many parameters.\n";
    return 1;
  }

  std::string topo = argv[1];
  std::string rateMode = argv[2];
  double intensity = std::stod(argv[3]);
  uint32_t seqRange = std::stoul(argv[4]);

  // flash 参数可选：默认值满足“轻量叠加”场景
  double flashMultiplier = 0.5;
  uint32_t flashNumberOfContents = 5000;
  uint32_t flashStartSeq = 20;

  if (argc >= 6) {
    flashMultiplier = std::stod(argv[5]);
  }
  if (argc >= 7) {
    flashNumberOfContents = std::stoul(argv[6]);
  }
  if (argc >= 8) {
    flashStartSeq = std::stoul(argv[7]);
  }

  std::cout << "topo: " << topo << std::endl;
  std::cout << "rateMode: " << rateMode << std::endl;
  std::cout << "intensity: " << intensity << std::endl;
  std::cout << "seqRange: " << seqRange << std::endl;
  std::cout << "flashMultiplier: " << flashMultiplier << std::endl;
  std::cout << "flashNumberOfContents: " << flashNumberOfContents << std::endl;
  std::cout << "flashStartSeq: " << flashStartSeq << std::endl;

  const std::vector<std::string> validTopos = {"topo-cpa-basic-A-", "topo-cpa-basic-A+"};
  const std::vector<std::string> validRateModes = {"rate-static", "rate-dynamic"};

  if (std::find(validTopos.begin(), validTopos.end(), topo) == validTopos.end() ||
      std::find(validRateModes.begin(), validRateModes.end(), rateMode) == validRateModes.end()) {
    std::cerr << "Invalid parameters.\n";
    return 1;
  }

  if (flashMultiplier < 0.0) {
    std::cerr << "Invalid: flashMultiplier must be >= 0\n";
    return 1;
  }

  if (flashNumberOfContents == 0) {
    std::cerr << "Invalid: flashNumberOfContents must be > 0\n";
    return 1;
  }

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/" + topo + ".txt");
  topologyReader.Read();

  // 当前实现与 ndn-cpa.cpp 对齐：仅对 basic 拓扑安装应用
  if (topo == "topo-cpa-basic-A-" || topo == "topo-cpa-basic-A+") {
    // Install NDN stack on all nodes
    ndn::StackHelper ndnHelper;
    ndnHelper.setPolicy("nfd::cs::lru");
    ndnHelper.setCsSize(100);
    for (int i = 1; i <= 4; i++) {
      ndnHelper.Install(Names::Find<Node>("Node" + std::to_string(i)));
    }

    // 消费者/producer禁用缓存：兴趣必须穿过边缘节点
    ndnHelper.setCsSize(0);
    ndnHelper.Install(Names::Find<Node>("Node0"));
    for (int i = 0; i <= 17; i++) {
      ndnHelper.Install(Names::Find<Node>("consumer_normal_" + std::to_string(i)));
    }

    // malicious 也必须安装 NDN 栈（否则全局路由/策略安装可能触发断言）
    if (topo == "topo-cpa-basic-A-") {
      for (int i = 0; i <= 2; i++) {
        ndnHelper.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
      }
    }
    else {
      for (int i = 0; i <= 17; i++) {
        ndnHelper.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
      }
    }

    // Set BestRoute strategy
    ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

    // Installing global routing interface on all nodes
    ndn::GlobalRoutingHelper gr;
    gr.InstallAll();

    Ptr<Node> producerNode = Names::Find<Node>("Node0");

    // 正常用户 baseline（全时段）
    int sum_rate = 0;
    std::vector<int> consumerRates;
    consumerRates.reserve(18);
    for (int i = 0; i <= 17; i++) {
      ndn::AppHelper base("ns3::ndn::ConsumerZipfMandelbrot");
      base.SetPrefix("/prefix");

      std::ostringstream oss;
      oss << 500 + 100 * (rand() % 6);
      std::string baseFrequency = oss.str();
      int currentRate = std::stoi(baseFrequency);
      sum_rate += currentRate;
      consumerRates.push_back(currentRate);

      base.SetAttribute("Frequency", StringValue(baseFrequency));
      base.SetAttribute("Randomize", StringValue("exponential"));
      base.SetAttribute("s", StringValue("1"));
      base.SetAttribute("NumberOfContents", StringValue("10000"));
      base.Install(Names::Find<Node>("consumer_normal_" + std::to_string(i)));
    }

    // flash overlay（叠加流量，仅 [3,4] 秒）
    const double tFlashStart = 3.0;
    const double tFlashEnd = 4.0;
    for (int i = 0; i <= 17; i++) {
      const double flashRate = std::max(1.0, (double)consumerRates[i] * flashMultiplier);
      ndn::AppHelper flash("ns3::ndn::ConsumerZipfMandelbrot");
      flash.SetPrefix("/prefix");
      flash.SetAttribute("Frequency", StringValue(std::to_string((int)flashRate)));
      flash.SetAttribute("Randomize", StringValue("exponential"));
      // 内容近似均匀：s=0；并可控内容区间
      flash.SetAttribute("s", StringValue("0"));
      flash.SetAttribute("NumberOfContents", StringValue(std::to_string(flashNumberOfContents)));
      flash.SetAttribute("ZipfStartSeq", UintegerValue(flashStartSeq));
      flash.SetAttribute("StartTime", TimeValue(Seconds(tFlashStart)));
      flash.SetAttribute("StopTime", TimeValue(Seconds(tFlashEnd)));
      flash.Install(Names::Find<Node>("consumer_normal_" + std::to_string(i)));
    }

    // CPA 攻击者（保留 ndn-cpa.cpp 逻辑，5s 开始）
    const int numAttackers = (topo == "topo-cpa-basic-A-") ? 3 : 18;
    const double rate_attacker = sum_rate * intensity / (double)numAttackers;
    const double vStep_attacker = 30.0 / (double)numAttackers;
    for (int i = 0; i < numAttackers; i++) {
      ndn::AppHelper attacker("ns3::ndn::ConsumerCPA");
      attacker.SetPrefix("/prefix");
      attacker.SetAttribute("vMax", DoubleValue(rate_attacker));
      if (rateMode == "rate-dynamic") {
        attacker.SetAttribute("isDynamic", BooleanValue(true));
      }
      attacker.SetAttribute("vStep", DoubleValue(vStep_attacker));
      attacker.SetAttribute("tStep", TimeValue(Seconds(0.05)));
      attacker.SetAttribute("MaxSeqA", UintegerValue(10000));
      attacker.SetAttribute("range", UintegerValue(seqRange));
      attacker.SetAttribute("StartTime", TimeValue(Seconds(5)));
      attacker.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
    }

    // Producer
    ndn::AppHelper producer("ns3::ndn::Producer");
    producer.SetPrefix("/prefix");
    producer.SetAttribute("PayloadSize", StringValue("1024"));
    producer.Install(producerNode);
    gr.AddOrigins("/prefix", producerNode);

    gr.CalculateAllPossibleRoutes();
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
