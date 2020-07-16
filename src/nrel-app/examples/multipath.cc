

// Main link:      WIFI mesh (Bandwidth: about 15Mbps)
// Redundant link: CSMA      (Bandwidth: about 10Mbps)
// TCP Throughput of SinglePath: 12.966 Mbit/s
// UDP Throughput of SinglePath: 14.9485 Mbit/s
// TCP Throughput of MultiPath: 18.195 Mbit/s
// UDP Throughput of MultiPath: 24.5624 Mbit/s


#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/nrel-app-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("multipath");

int main (int argc, char** argv)
{
  bool verbose          = true;
  uint64_t datarate     = 24000;
  double simulationTime = 10;
  double distance       = 50;
  Time::SetResolution (Time::NS);

  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on some relevant log components", verbose);
  cmd.AddValue ("datarate", "DataRate of application (bps)", datarate);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
  cmd.Parse (argc, argv);

  std::cout << "Main link:      WIFI mesh (Bandwidth: about 15Mbps)"  << std::endl;
  std::cout << "Redundant link: CSMA      (Bandwidth: about 10Mbps)"  << std::endl;

  for(uint8_t hybrid = 0;hybrid < 2; hybrid++){

    for (uint8_t udp = 0; udp <= 1; udp++){
      if (verbose)
      {
        LogComponentEnable ("multipath", LOG_LEVEL_INFO);
      }

      Config::SetDefault("ns3::Ipv4MultipathRouting::EcmpEnable",BooleanValue(hybrid));
      // NS_LOG_INFO ("Create nodes.");
      Ptr<Node> n0 = CreateObject<Node> ();
      Ptr<Node> n1 = CreateObject<Node> ();

      NodeContainer all (n0, n1);

      MobilityHelper mobility;
      mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (0.0),
                                     "MinY", DoubleValue (0.0),
                                     "DeltaX", DoubleValue (distance),
                                     "DeltaY", DoubleValue (distance),
                                     "GridWidth", UintegerValue (10),
                                     "LayoutType", StringValue ("RowFirst"));
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (all);

      // NS_LOG_INFO ("Create IPv4 Internet Stack");

      Ipv4MultipathRoutingHelper multipathRoutingHelper;
      Ipv4StaticRoutingHelper staticRoutingHelper;

      Ipv4ListRoutingHelper listRoutingHelper;
      listRoutingHelper.Add (multipathRoutingHelper, 0);
      listRoutingHelper.Add (staticRoutingHelper, -10);

      InternetStackHelper internet;
      internet.SetRoutingHelper (listRoutingHelper);
      internet.Install (all);

      // NS_LOG_INFO ("Create channels.");

      YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
      YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
      wifiPhy.SetChannel (wifiChannel.Create ());

      MeshHelper mesh;
      mesh = MeshHelper::Default ();
      mesh.SetStackInstaller ("ns3::Dot11sStack");
      mesh.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
      mesh.SetMacType ("RandomStart", TimeValue (Seconds (0.1)));
      mesh.SetNumberOfInterfaces (1);

      NetDeviceContainer d1 = mesh.Install (wifiPhy, all);

      CsmaHelper csma;
      csma.SetChannelAttribute ("DataRate", StringValue ("10Mbps"));
      csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

      NetDeviceContainer d2;
      d2 = csma.Install (all);
      
      Ipv4AddressHelper ipv4;
      ipv4.SetBase ("10.1.1.0", "255.255.255.0");
      Ipv4InterfaceContainer i1 = ipv4.Assign (d1);

      ipv4.SetBase ("10.1.2.0", "255.255.255.0");
      Ipv4InterfaceContainer i2 = ipv4.Assign (d2);

      // NS_LOG_INFO ("Create application and transmit data from n0 to n1");

      
      Ptr<Ipv4MultipathRouting> n0MultipathRouting = multipathRoutingHelper.GetMultipathRouting (n0->GetObject<Ipv4> ());
      // n0MultipathRouting->AddNetworkRouteTo ("10.1.1.0", "255.255.255.0", 1);
      n0MultipathRouting->RemoveRoute(0);
      n0MultipathRouting->RemoveRoute(0);
      n0MultipathRouting->SetDefaultRoute (i1.GetAddress (1), 1);
      n0MultipathRouting->SetDefaultRoute (i2.GetAddress (1), 2);

      // Ipv4RoutingHelper ipv4RoutingHelper;
      Ptr<OutputStreamWrapper> allroutingtable = Create<OutputStreamWrapper>("routingtableallnodes",std::ios::out);
      staticRoutingHelper.PrintRoutingTableAllAt(Seconds(2), allroutingtable);

      ApplicationContainer serverApp, clientApp;
      uint32_t payloadSize; //1500 byte IP packet
      if (udp)
      {
        payloadSize = 1472; //bytes
      }
      else
      {
        payloadSize = 1448; //bytes
        Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
      }
      if (udp)
      {
        //UDP flow
        UdpServerHelper myServer (9);
        serverApp = myServer.Install (n1);
        serverApp.Start (Seconds (0.0));
        serverApp.Stop (Seconds (simulationTime + 1));

        UdpClientHelper myClient (i1.GetAddress (1), 9);
        myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
        myClient.SetAttribute ("Interval", TimeValue (Time ("0.00001"))); //packets/s
        myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize));

        clientApp = myClient.Install (n0);
        clientApp.Start (Seconds (1.0));
        clientApp.Stop (Seconds (simulationTime + 1));
      }
      else
      {
        //TCP flow
        uint16_t port = 50000;
        Address apLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", apLocalAddress);
        serverApp = packetSinkHelper.Install (n1);

        serverApp.Start (Seconds (0.0));
        serverApp.Stop (Seconds (simulationTime + 1));

        OnOffHelper onoff ("ns3::TcpSocketFactory",Ipv4Address::GetAny ());
        onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetAttribute ("PacketSize", UintegerValue (payloadSize));
        onoff.SetAttribute ("DataRate", DataRateValue (1000000000)); //bit/s

        AddressValue remoteAddress (InetSocketAddress (i1.GetAddress (1), port));
        onoff.SetAttribute ("Remote", remoteAddress);
        clientApp.Add (onoff.Install (n0));
        clientApp.Start (Seconds (1.0));
        clientApp.Stop (Seconds (simulationTime + 1));
      }

      // NS_LOG_INFO ("Run Simulation.");
      Simulator::Stop (Seconds (simulationTime+4));
      Simulator::Run ();
      Simulator::Destroy ();
      double throughput = 0;
      if (udp)
      {
        //UDP
        uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
        throughput = totalPacketsThrough * payloadSize * 8 / (simulationTime * 1000000.0); //Mbit/s
        if(hybrid)
          std::cout << "UDP Throughput of MultiPath: " << throughput << " Mbit/s" << std::endl;
        else
          std::cout << "UDP Throughput of SinglePath: " << throughput << " Mbit/s" << std::endl;
      }
      else
      {
        //TCP
        uint32_t totalPacketsThrough = DynamicCast<PacketSink> (serverApp.Get (0))->GetTotalRx ();
        throughput = totalPacketsThrough * 8 / (simulationTime * 1000000.0); //Mbit/s
        if(hybrid)
          std::cout << "TCP Throughput of MultiPath: " << throughput << " Mbit/s" << std::endl;
        else
          std::cout << "TCP Throughput of SinglePath: " << throughput << " Mbit/s" << std::endl;
      }
      // NS_LOG_INFO ("Done.");
    }
  }
  return 0;
}

