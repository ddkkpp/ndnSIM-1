/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Example for DFN topology (topo-cpa-DFN-A- / topo-cpa-DFN-A+)
 * Automatically detects the number of routers and consumers from the topology file.
 */

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>   // add
#include <cstdio>    // add (std::remove)

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/apps/ndn-consumer-pcon.hpp"
#include "common/global.hpp"

namespace ns3 {

// 辅助函数：自动探测以 prefix 开头并以连续数字结尾的节点数量
// 例如：prefix="Node" -> Node0, Node1, ... 直到找不到为止
static uint32_t
DiscoverNodeCount(const std::string& prefix)
{
  uint32_t count = 0;
  while (Names::Find<Node>(prefix + std::to_string(count)) != nullptr) {
    count++;
  }
  return count;
}

// 辅助函数：清理并标准化拓扑文件，去除 BOM 和 Windows CRLF
static std::string
SanitizeTopologyFileOrDie(const std::string& inPath)
{
  const std::string outPath = inPath + ".sanitized";

  std::ifstream in(inPath, std::ios::binary);
  if (!in) {
    std::cerr << "Error: cannot open topology file: " << inPath << std::endl;
    std::exit(1);
  }

  std::ofstream out(outPath, std::ios::binary);
  if (!out) {
    std::cerr << "Error: cannot write sanitized topology file: " << outPath << std::endl;
    std::exit(1);
  }

  std::string line;
  bool firstLine = true;

  while (std::getline(in, line)) {
    // strip trailing CR (Windows CRLF)
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    if (firstLine) {
      firstLine = false;
      // strip UTF-8 BOM if present: EF BB BF
      const unsigned char* p = reinterpret_cast<const unsigned char*>(line.data());
      if (line.size() >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
        line.erase(0, 3);
      }
    }

    out << line << "\n";
  }

  out.flush();
  return outPath;
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

  PrintCliArgs(argc, argv, "ndn-cpa-dfn");

  if (argc < 5) {
    std::cerr << "Usage: ./waf --run=ndn-cpa-dfn para1 para2 intensity seqRange\n";
    std::cerr << "para1: topo-cpa-DFN-A- | topo-cpa-DFN-A+\n";
    std::cerr << "para2: rate-static | rate-dynamic\n";
    std::cerr << "para3: intensity (e.g. 1.0)\n";
    std::cerr << "para4: seq range (e.g. 200)\n";
    return 1;
  }

  std::string para1 = argv[1];
  std::string para2 = argv[2];
  double para3 = std::stod(argv[3]);
  uint32_t para4 = std::stoul(argv[4]);

  std::vector<std::string> validPara1 = {"topo-cpa-DFN-A-", "topo-cpa-DFN-A+"};
  std::vector<std::string> validPara2 = {"rate-static", "rate-dynamic"};

  if (std::find(validPara1.begin(), validPara1.end(), para1) == validPara1.end() ||
      std::find(validPara2.begin(), validPara2.end(), para2) == validPara2.end()) {
    std::cerr << "Invalid parameters.\n";
    return 1;
  }

  // 1. 读取拓扑
  const std::string topoPath = "src/ndnSIM/examples/topologies/" + para1 + ".txt";
  const std::string topoPathSan = SanitizeTopologyFileOrDie(topoPath);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName(topoPathSan);
  topologyReader.Read();

  // 2. 自动探测网络规模
  // DFN Topology conventions:
  // NodeP0, NodeP1 ... (Producers)
  // NodeR0, NodeR1 ... (Routers)
  
  uint32_t numNodesP = DiscoverNodeCount("NodeP");
  uint32_t numNodesR = DiscoverNodeCount("NodeR");

  uint32_t numNormal = DiscoverNodeCount("consumer_normal_");
  uint32_t numMalicious = DiscoverNodeCount("consumer_malicious_");

  std::cout << "Detected Topology Scale:" << std::endl;
  std::cout << "  Producers (NodeP*): " << numNodesP << std::endl;
  std::cout << "  Routers   (NodeR*): " << numNodesR << std::endl;
  std::cout << "  Normal Consumers:   " << numNormal << std::endl;
  std::cout << "  Malicious Consumers:" << numMalicious << std::endl;

  if (numNodesP == 0) {
    std::cerr << "Error: No producer nodes found (expected NodeP0...)" << std::endl;
    return 1;
  }

  // 3. 安装 NDN 协议栈
  ndn::StackHelper ndnHelper;
  ndnHelper.setPolicy("nfd::cs::lru");
  
  // 3.1 路由器配置 (NodeR*, NodeP*, Node*, CS=100)
  // 这里将 P 和 R 都作为骨干节点处理堆栈安装，生产者额外装 App
  ndnHelper.setCsSize(100);
  
  // Install on NodeR*
  for (uint32_t i = 0; i < numNodesR; ++i) {
    Ptr<Node> node = Names::Find<Node>("NodeR" + std::to_string(i));
    if (node) ndnHelper.Install(node);
  }
  
  // Install on NodeP*
  // Note: Producers in DFN act as routers too in the core? Or endpoints?
  // Let's assume they are core-connected content sources.
  for (uint32_t i = 0; i < numNodesP; ++i) {
     Ptr<Node> node = Names::Find<Node>("NodeP" + std::to_string(i));
     if (node) ndnHelper.Install(node);
  }

  // 3.2 终端节点配置 (Consumers, CS=0)
  ndnHelper.setCsSize(0);

  // Consumers
  for (uint32_t i = 0; i < numNormal; ++i) {
    ndnHelper.Install(Names::Find<Node>("consumer_normal_" + std::to_string(i)));
  }
  for (uint32_t i = 0; i < numMalicious; ++i) {
    ndnHelper.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
  }

  // 4. 路由策略与全局路由
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // 5. 应用层配置
  
  // 5.1 正常用户 (Normal Consumers)
  int sum_rate = 0;
  for (uint32_t i = 0; i < numNormal; ++i) {
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
    consumerHelper.SetPrefix("/prefix");
    
    // 随机频率 500-1000 Hz
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

  // 5.2 恶意用户 (Malicious Consumers)
  if (numMalicious > 0) {
    double rate_attacker = sum_rate * para3 / (double)numMalicious;
    double vStep_attacker = 30.0 / (double)numMalicious; // 保持总 step 力度
    
    std::cout << "Configuring Malicious Consumers:" << std::endl;
    std::cout << "  Total Normal Rate: " << sum_rate << std::endl;
    std::cout << "  Intensity: " << para3 << std::endl;
    std::cout << "  Per-Attacker Rate: " << rate_attacker << std::endl;

    for (uint32_t i = 0; i < numMalicious; ++i) {
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
      consumerHelper.SetAttribute("StartTime", TimeValue(Seconds(5))); // 5秒开始攻击
      
      consumerHelper.Install(Names::Find<Node>("consumer_malicious_" + std::to_string(i)));
    }
  }

  // 5.3 生产者 (Producer)
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

  // Install on NodeP0, NodeP1 ...
  if (numNodesP > 0) {
      for(uint32_t i = 0; i < numNodesP; ++i) {
          Ptr<Node> pNode = Names::Find<Node>("NodeP" + std::to_string(i));
          if(pNode) {
              producerHelper.Install(pNode);
              ndnGlobalRoutingHelper.AddOrigins("/prefix", pNode);
          }
      }
  }

  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();

  Simulator::Stop(Seconds(30.01));
  Simulator::Run();
  Simulator::Destroy();

  // 清理临时文件（失败也无所谓）
  std::remove(topoPathSan.c_str());

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
