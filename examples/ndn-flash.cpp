/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * ndn-flash.cpp
 *
 * 独立场景：正常用户的 flash event
 * - baseline：所有正常用户全时段 Zipf 访问 /prefix，NumberOfContents=10000
 * - flash：在 [tFlashStart, tFlashEnd] 时间窗口，所有正常用户额外叠加一组更高频率、更集中内容范围的 Zipf 请求
 */

#include <iostream>
#include <algorithm>
#include <sstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // 添加调试打印，确保程序启动即有输出（参考 ndn-cpa.cpp）
  std::cout << "命令行参数数量: " << argc << std::endl;
  std::cout << "所有参数: ";
  for (int i = 0; i < argc; i++) {
    std::cout << argv[i] << " ";
  }
  std::cout << std::endl;

  // 使用 ConsumerZipfMandelbrot 近似均匀即可：只需要指定 flash 内容规模（NumberOfContents）
  if (argc < 5) {
    std::cerr << "Usage: ./waf --run=ndn-flash para1 para2 para3 para4\n";
    std::cerr << "para1: topo name, e.g., topo-cpa-basic-A-, topo-cpa-basic-A+, topo-cpa-DFN-A-, topo-cpa-DFN-A+\n";
    std::cerr << "para2: flash倍率 (建议 2~10)\n";
    std::cerr << "para3: flashNumberOfContents (新前缀 /prefix 下内容规模，seq ∈ [0, N-1])\n";
    std::cerr << "para4: flashStartSeq (flash 内容起始序列号)\n";
    return 1;
  }

  std::string para1 = argv[1];
  double para2 = std::stod(argv[2]);
  uint32_t flashNumberOfContents = std::stoul(argv[3]);
  uint32_t flashStartSeq = std::stoul(argv[4]);

  std::cout << "para1: topo= " << para1 << std::endl;
  std::cout << "para2: flashRateMultiplier= " << para2 << std::endl;
  std::cout << "para3: flashNumberOfContents= " << flashNumberOfContents << std::endl;
  std::cout << "para4: flashStartSeq= " << flashStartSeq << std::endl;

  if (flashNumberOfContents == 0) {
    std::cerr << "Invalid: flashNumberOfContents must be > 0\n";
    return 1;
  }

  std::vector<std::string> validPara1 = {
    "topo-cpa-basic-A-", "topo-cpa-basic-A+", "topo-cpa-DFN-A-", "topo-cpa-DFN-A+"
  };
  if (std::find(validPara1.begin(), validPara1.end(), para1) == validPara1.end()) {
    std::cerr << "Invalid topo: " << para1 << "\n";
    return 1;
  }

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

    // 消费者/producer禁用缓存（保持你当前实验假设：兴趣必须穿过边缘节点）
    ndnHelper.setCsSize(0);
    ndnHelper.Install(Names::Find<Node>("Node0"));
    for (int i = 0; i <= 17; i++) {
      ndnHelper.Install(Names::Find<Node>("consumer_normal_" + std::to_string(i)));
    }

    // 修复 m_ptr 断言：必须在拓扑中所有节点上安装 NDN 栈（包括 malicious），
    // 否则 GlobalRoutingHelper::InstallAll() 遍历到无栈节点时会崩溃。
    if (para1 == "topo-cpa-basic-A-") {
      for (int i = 0; i <= 2; i++) {
        ndnHelper.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
      }
    }
    else if (para1 == "topo-cpa-basic-A+") {
      for (int i = 0; i <= 17; i++) {
        ndnHelper.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
      }
    }

    ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

    ndn::GlobalRoutingHelper gr;
    gr.InstallAll();

    Ptr<Node> producerNode = Names::Find<Node>("Node0");

    // baseline（全时段）
    int sum_rate = 0;
    std::vector<int> consumerRates; // 存储每个用户的基准速率
    for (int i = 0; i <= 17; i++) {
      ndn::AppHelper base("ns3::ndn::ConsumerZipfMandelbrot");
      base.SetPrefix("/prefix");

      std::ostringstream oss;
      oss << 500 + 100 * (rand() % 6);
      std::string baseFrequency = oss.str();
      int currentRate = std::stoi(baseFrequency);
      sum_rate += currentRate;
      consumerRates.push_back(currentRate);
      std::cout << "currentRate: " << currentRate << std::endl;

      base.SetAttribute("Frequency", StringValue(baseFrequency));
      base.SetAttribute("Randomize", StringValue("exponential"));
      base.SetAttribute("s", StringValue("1"));
      base.SetAttribute("NumberOfContents", StringValue("10000"));
      base.Install(Names::Find<Node>("consumer_normal_" + std::to_string(i)));
    }

    // flash（叠加流量，只在窗口内运行）
    const double tFlashStart = 5.0;
    const double tFlashEnd = 6.0;

    // 叠加流量：内容近似均匀（s=0），间隔指数分布（Randomize=exponential）
    for (int i = 0; i <= 17; i++) {
      // 叠加流量应该是自己原来的多少倍
      double flashRate = std::max(1.0, (double)consumerRates[i] * para2);
      std::cout << "flashRate: " << flashRate << std::endl; // 使用 std::endl 强制刷新

      ndn::AppHelper flash("ns3::ndn::ConsumerZipfMandelbrot");
      flash.SetPrefix("/prefix");
      flash.SetAttribute("Frequency", StringValue(std::to_string((int)flashRate)));
      flash.SetAttribute("Randomize", StringValue("exponential"));
      flash.SetAttribute("s", StringValue("0"));
      flash.SetAttribute("NumberOfContents", StringValue(std::to_string(flashNumberOfContents)));
      // 设置起始序列号，ConsumerZipfMandelbrot 需支持 ContentStartSeq 属性
      flash.SetAttribute("ZipfStartSeq", UintegerValue(flashStartSeq));
      flash.SetAttribute("StartTime", TimeValue(Seconds(tFlashStart)));
      flash.SetAttribute("StopTime", TimeValue(Seconds(tFlashEnd)));
      flash.Install(Names::Find<Node>("consumer_normal_" + std::to_string(i)));
    }

    ndn::AppHelper producer("ns3::ndn::Producer");
    producer.SetPrefix("/prefix");
    producer.SetAttribute("PayloadSize", StringValue("1024"));
    producer.Install(producerNode);

    // producer 同时服务 baseline 和 flash 前缀
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
