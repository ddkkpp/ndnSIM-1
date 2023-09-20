#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {
int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("/home/dkp/BRITE/0.txt");
  topologyReader.Read();
/*
  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.InstallAll();
*/
  ndn::StackHelper ndnHelper;
  ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.setCsSize(100);
  ndnHelper.InstallAll();

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route/%FD%01");
  //ndn::StrategyChoiceHelper::InstallAll<nfd::fw::SelfLearningStrategy>("/");
  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  NodeContainer producerNodes;
  //producerNodes.Add(Names::Find<Node>("21"));//hop4
//  producerNodes.Add(Names::Find<Node>("46"));//hop5
producerNodes.Add(Names::Find<Node>("38"));//hop6
//producerNodes.Add(Names::Find<Node>("74"));//hop7
//producerNodes.Add(Names::Find<Node>("185"));//hop8


  
  /*
  for(int i=1;i<10;i=i+2){
    producerNodes.Add(Names::Find<Node>("i"));
  }
  */
  NodeContainer consumerNodes;
  
  consumerNodes.Add(Names::Find<Node>("27"));
  /*
  for(int i=200;i>190;i=i-2){
    consumerNodes.Add(Names::Find<Node>("i"));
  }
  */
  

  // Install NDN applications
  std::string prefix = "/prefix";

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");//1/(k+q)^s/H(n,q,s)
  consumerHelper.SetPrefix(prefix);
  consumerHelper.SetAttribute("Frequency", StringValue("10000")); // 10000 interests a second
  consumerHelper.SetAttribute("NumberOfContents",StringValue("1000"));
  consumerHelper.SetAttribute("q",StringValue("1"));
  consumerHelper.SetAttribute("s",StringValue("0.1"));
  consumerHelper.Install(consumerNodes);

  ndn::AppHelper producerHelper("ns3::ndn::ProducerRep");
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  //producerHelper.SetAttribute("honesty", DoubleValue(1));
  producerHelper.Install(producerNodes);

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins(prefix, producerNodes);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(5));

  //用户响应延迟
  ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");
  ndn::L3RateTracer::Install(Names::Find<Node>("27"),"rate-trace.txt",Seconds(0.01));
  ndn::CsTracer::InstallAll("cs.txt",Seconds(2));
  //Time t1=Simulator::Now();
  Simulator::Run();
  Simulator::Destroy();
  //Time t2=Simulator::Now();
  //std::cout<<t2.ToDouble(Time::S)<<std::endl<<t1.ToDouble(Time::S)<<std::endl;


  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
