// 03/12/17 - Script updated to accept options and values from the command line during runtime - Adarsh Hasandka
// 04/21/17 - Script updated for new topology file format
// 06/20/17 - Script updated to include adjustable network parameters
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <array>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/packet-sink.h"
#include "ns3/log.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/gnuplot.h"
#include "ns3/stats-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/netanim-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/nrel-app-module.h"
#include "csv.h"
using namespace ns3;
using namespace std;


NS_LOG_COMPONENT_DEFINE ("Demo11");
//uint8_t dd[8]={"0000000"};
//uint8_t *payload= dd;
//
//class MyApp : public Application
//{
//public:
//
//  MyApp ();
//  virtual ~MyApp();
//
//  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize,
//               DataRate dataRate, uint8_t * Payload);
//
//private:
//  virtual void StartApplication (void);
//  virtual void StopApplication (void);
//
//  void ScheduleTx (void);
//  void SendPacket (void);
//
//  Ptr<Socket>     m_socket;
//  Address         m_peer;
//  uint32_t        m_packetSize;
//  uint8_t *  m_Payload;
//  DataRate        m_dataRate;
//  EventId         m_sendEvent;
//  bool            m_running;
//  uint32_t        m_packetsSent;
//};
//
//MyApp::MyApp ()
//  : m_socket (0),
//    m_peer (),
//    m_packetSize (0),
//    m_dataRate (0),
//    m_sendEvent (),
//    m_running (false),
//    m_packetsSent (0)
//{
//}
//
//MyApp::~MyApp()
//{
//  m_socket = 0;
//}
//
//void
//MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize,
//               DataRate dataRate, uint8_t * Payload)
//{
//  m_socket = socket;
//  m_peer = address;
//  m_packetSize = packetSize;
//  m_dataRate = dataRate;
//  m_Payload  = Payload;
//}
//
//void
//MyApp::StartApplication (void)
//{
//  m_running = true;
//  m_packetsSent = 0;
//  m_socket->Bind ();
//  m_socket->Connect (m_peer);
//  SendPacket ();
//}
//
//void
//MyApp::StopApplication (void)
//{
//  m_running = false;
//
//  std::cout << "STOP APPLICATION" << std::endl;
//  if (m_sendEvent.IsRunning ())
//    {
//      Simulator::Cancel (m_sendEvent);
//    }
//
//  if (m_socket)
//    {
//      m_socket->Close ();
//    }
//}
//
//void
//MyApp::SendPacket (void)
//{
//  Ptr<Packet> packet = Create<Packet> (m_Payload, m_packetSize);
//  m_socket->Send (packet);
//  // std::cout<<"Send throughput"<<m_Payload<<std::endl;
//  // std::cout<<"Send throughput(char)";
//  // for(int i=0; i< 8; i++){
//  // 	 std::cout << (int)*(m_Payload+i);
//  // }
//  // std::cout<< std::endl;
//
//
//  ScheduleTx ();
//
//}
//
//void
//MyApp::ScheduleTx (void)
//{
//  if (m_running)
//    {
//      Time tNext (Seconds (1));
//      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
//    }
//}

void checkLinkState32 (int8_t oldfeed, int8_t newfeed)
{
	// NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t receive command " << (int)newfeed);

	if(newfeed=='1'){
		NS_LOG_UNCOND("node 1 receive check command");
	}

	// Event = Simulator::Schedule (tNext, &MyApp::SendPacket, this);;
}


MobilityHelper MapReading(string arr_name[], double arr_lon[], double arr_lat[], string arr_type[], uint16_t nodeCount)
{
	double minLong=500, minLat=500, x_axis=0, y_axis=0;
	uint16_t i=0;
	// Install Mobility Model
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

	// Read topology for Longitude and Latitude positions
	for(i=0;i<nodeCount;i++) 		// Find minimum Latitude and Longitude for Position boundry
	{
		if(arr_lon[i]<minLong)
			minLong=arr_lon[i];
		if(arr_lat[i]<minLat)
			minLat=arr_lat[i];
	}
	for(i=0;i<nodeCount;i++) 		// Parse Latitude and Longitude to NS3 viewable values
	{
		x_axis=(arr_lon[i]-minLong)*72400; 	// Updated for relatively accurate distances in meters based on optimal solution of device location in degrees
		y_axis=(arr_lat[i]-minLat)*72400;
		positionAlloc->Add(Vector(x_axis, y_axis, 0));
	}
	MobilityHelper mobility;
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(positionAlloc);

	return mobility;
}

void parseCSV(string arr_name[], double arr_lon[], double arr_lat[], string arr_type[], uint16_t arr_MG[], uint16_t arr_PG[], string file)
{
	io::CSVReader<6> in(file);
	in.read_header(io::ignore_extra_column, "name", "longitude", "latitude", "node_type", "mesh_group", "pan_group");
	string name; double longitude; double latitude; string type; uint16_t meshGr; uint16_t panGr; uint16_t i=0;
	while(in.read_row(name, longitude, latitude, type, meshGr, panGr))
	{
		//std::cout << "Name: " << name << " Long: "<< longitude << " Lat: "<< latitude << " Type: " << type << " MG: "<< meshGr << " PG: " << panGr << std::endl; // print data as read by parser
		arr_name[i]=name;
		arr_lon[i]=longitude;
		arr_lat[i]=latitude;
		arr_type[i]=type;
		arr_MG[i]=meshGr;
		arr_PG[i]=panGr;
		i++;
	}
}
/*
void ThroughputMonitor (FlowMonitorHelper *flowHelper, Ptr<FlowMonitor> flowMonitor)
{
	int i=0;
//	double localThrou=0;
	std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMonitor->GetFlowStats();
	Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (flowHelper->GetClassifier());
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
	{
		// std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin ();
		// ++stats;
		i++;
		if(i==2){
			Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
			std::cout<<"Flow ID           : " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
			std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
			std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
			std::cout<<"Duration      : "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
			std::cout<<"Last Received Packet  : "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
			std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024  << " Kbps"<<std::endl;
			std::cout<<"Last Delay    : "<< stats->second.lastDelay.GetMilliSeconds()<<" ms"<<std::endl;
			std::cout<<"Sum of Jitter : "<< stats->second.jitterSum.GetSeconds()<<" seconds"<<std::endl;
			std::cout<<"Average of Delay : "<< stats->second.delaySum.GetSeconds()/stats->second.txPackets<<" seconds"<<std::endl;

//			localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024);

//			std::cout <<"localThru"<<endl;
			// updata gnuplot data
//			std::ostringstream strs;
//			strs << localThrou;
//			memcpy(payload, strs.str().c_str(), 8);


			// slope= stats->second.jitterSum.GetSeconds() -jitter;
			// jitter=stats->second.jitterSum.GetSeconds();
			// if(Simulator::Now().GetSeconds() > 30&&flag<1){
			//  if(slope > 1.020){
			//      flag++;
			//      std::cout<<num<<"At " << Simulator::Now().GetSeconds() <<"s attack start"<< std::endl;
			//      memcpy(payload, start, 8);
			//  }
			// }
			// if(flag > 1){
			//  std::ostringstream strs;
			//  strs << localThrou;
			//  memcpy(payload, strs.str().c_str(), 8);
			// }

//			DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
//			// DataSet.Add((double)Simulator::Now().GetSeconds(),(double) stats->second.lastDelay.GetSeconds());
//			// DataSet.Add((double)Simulator::Now().GetSeconds(),(double) stats->second.jitterSum.GetSeconds());
//			// DataSet.Add((double)Simulator::Now().GetSeconds(),(double) stats->second.delaySum.GetSeconds()/stats->second.txPackets);
//
//			aggregator->Write2d ("Dataset/Context/String", (double)Simulator::Now().GetSeconds(),(double) localThrou);
			// std::cout<<num<<"---------------------------------------------------------------------------"<<std::endl;
		}
	}

	Simulator::Schedule(Seconds(1),&ThroughputMonitor, flowHelper, flowMonitor);

	flowMonitor->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
}
 */
Ptr<Server> serv;
uint64_t lastTotalRx = 0;
// void delay_count ()
// {
// 	Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
// 	double delay  = serv->m_delay;
// 	NS_LOG_INFO (Simulator::Now ().GetSeconds () << "\t " << delay << " ms");
// 	Simulator::Schedule (Seconds (0.2), &delay_count);
// }

void CalculateThroughput ()
{
	Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
	double cur = (serv->GetTotalRx () - lastTotalRx) * (double) 8/1e3;     /* Convert Application RX Packets to MBits. */
	std::cout << now.GetSeconds () << "\ts:\t" << cur << "\tMbit/s" << std::endl;
	lastTotalRx = serv->GetTotalRx ();
	Simulator::Schedule (Seconds (0.2), &CalculateThroughput);
}

int main (int argc, char** argv)
{
	double simTime = 500;
	uint64_t datarate = 24000;
	//	uint32_t PacketSize=200;
	uint32_t RunNo=3;									// Default Run number
	Time::SetResolution (Time::NS);
	//	std::string Topology= ("topology-v1-1.csv");		// Default topology file location
	std::string Topology= ("topology-v1-2.csv");
	std::string TransferProtocol="UDP";
	std::string file="scratch/"+Topology;
	double ChannelDelay=3;
	std::string ChannelDataRate="1Mbps";

	//	GlobalValue::Bind ("SimulatorImplementationType",
	//			StringValue ("ns3::RealtimeSimulatorImpl"));
	CommandLine cmd;
	//-------------------Common Parameters for all network configurations--------------------------//
	//	cmd.AddValue("PacketSize", "Size in bytes of packets to echo (Bytes)", PacketSize);
	cmd.AddValue("RunNo", "Run Number to use for RNG process variables", RunNo);
	cmd.AddValue("SimTime", "Duration in seconds for which the simulation is run", simTime);
	cmd.AddValue ("Topology", "Filename of Topology CSV file within scratch folder", Topology);
	cmd.AddValue ("TransferProtocol", "Transfer Protocol to use (TCP/UDP)", TransferProtocol);
	cmd.Parse (argc, argv);

	RngSeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
	RngSeedManager::SetRun (RunNo);   // Changes run number from default of 1
	//----------------------------------Read Topology CSV file-----------------------------------------------------------------
	io::CSVReader<6> in(file);
	in.read_header(io::ignore_extra_column, "name", "longitude", "latitude", "node_type", "mesh_group", "pan_group");
	string name; double longitude; double latitude; string type; uint16_t meshGr; uint16_t panGr;
	uint16_t PVcount=0, DCcount=0, SMcount=0, ERcount=0, nodeCount=0, i,  x; //j,
	while(in.read_row(name, longitude, latitude, type, meshGr, panGr))
	{
		if((type=="Data Concentrator")||(type=="Candidate Built Node"))
			DCcount++;
		if(type=="PV Inverter")
			PVcount++;
		if(type=="Smart Meter")
			SMcount++;
		if(type=="Edge Router")
			ERcount++;
		nodeCount++;
	}

	string arr_name[nodeCount]; double arr_lon[nodeCount]; double arr_lat[nodeCount]; string arr_type[nodeCount]; uint16_t arr_MG[nodeCount]; uint16_t arr_PG[nodeCount];	// Declare array of size determined by maximum node count

	parseCSV(arr_name,arr_lon,arr_lat,arr_type,arr_MG,arr_PG,file);				// Parse CSV file and store columns in individual arrays

	MobilityHelper position = MapReading(arr_name,arr_lon,arr_lat,arr_type,nodeCount);

	std::string mode = "ConfigureLocal";
	std::string tapName = "thetap";
	std::string tapName1 = "thetap1";
	LogComponentEnable ("Demo11", LOG_LEVEL_INFO);
	LogComponentEnable ("Server", LOG_LEVEL_INFO);
	LogComponentEnable ("NetRouter", LOG_LEVEL_INFO);
	LogComponentEnable ("Client", LOG_LEVEL_INFO);
	//	LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
	//	LogComponentEnable ("Client", LOG_LEVEL_ALL);

	//------------------------ Create nodes -------------------------------//
	NodeContainer concentrator;
	concentrator.Create(DCcount);
	NodeContainer pvInvertor;
	pvInvertor.Create(PVcount);
	NodeContainer smartMeter;
	smartMeter.Create(SMcount);
	NodeContainer edgeRouter;
	edgeRouter.Create(ERcount);
	NodeContainer csmaGroup[DCcount];
	NodeContainer panGroup[PVcount];

	NodeContainer nodes;
	nodes.Add(concentrator);
	nodes.Add(pvInvertor);
	nodes.Add(smartMeter);
	nodes.Add(edgeRouter);
	position.Install(nodes);

	for(x=0;x<DCcount;x++)
	{
		for(i=0;i<nodeCount;i++)
		{
			//------------------ Create csma groups ------------------//
			if(arr_MG[i]-1==x)
			{
				if((arr_type[i]=="Data Concentrator")||(arr_type[i]=="Candidate Built Node"))
					csmaGroup[x].Add(concentrator.Get(i));
				if(arr_type[i]=="Smart Meter")
					csmaGroup[x].Add(nodes.Get(i));
			}
		}
	}

	for(x=0;x<PVcount;x++)
	{
		for(i=nodeCount;i>0;i--)
		{
			//--------------- Create sixlowpan groups ------------------//

			if(arr_PG[i-1]-1==x)
				if(arr_type[i-1]=="Smart Meter" || arr_type[i-1]=="PV Inverter")
					panGroup[x].Add(nodes.Get(i-1));
		}
	}

	//------------------ Create SixLowPan Devices ------------------//
	NetDeviceContainer lrwpanDevices[PVcount];
	NetDeviceContainer sixlowpanDevices[PVcount];
	LrWpanHelper lrWpanHelper[PVcount];

	for(i=0; i<PVcount; i++)
	{
		lrwpanDevices[i] = lrWpanHelper[i].Install(panGroup[i]);
		lrWpanHelper[i].AssociateToPan (lrwpanDevices[i], i);
	}
	SixLowPanHelper sixlowpan;
	for(i=0; i<PVcount; i++)
	{
		sixlowpanDevices[i] = sixlowpan.Install(lrwpanDevices[i]);
	}

	NetDeviceContainer csmaDevices[DCcount];
	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue (ChannelDataRate));
	csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (ChannelDelay)));

	for(i=0; i<DCcount; i++)
	{
		csmaDevices[i] = csma.Install (csmaGroup[i]);	// Install CSMA on CSMA devices for all areas
	}

	NS_LOG_INFO ("Create Internet Stack");
	InternetStackHelper internet;
	internet.Install (nodes);
	Ipv4AddressHelper ipv4;
	Ipv4InterfaceContainer csmaInterface[DCcount];
	for(i=0; i<DCcount; i++)
	{
		std::ostringstream ss;
		ss <<"10.1."<< i<< ".0";
		ipv4.SetBase (ss.str().c_str(), "255.255.255.0");
		csmaInterface[i] = ipv4.Assign (csmaDevices[i]);
	}
	Ipv6AddressHelper ipv6;
	Ipv6InterfaceContainer sixlowpanInterfaces[PVcount];
	for(i=0; i<PVcount; i++)
	{
		std::ostringstream ss;
		ss <<"2002:" << i<< "::";
		ipv6.SetBase (ss.str().c_str(), Ipv6Prefix (64));
		sixlowpanInterfaces[i] = ipv6.Assign (sixlowpanDevices[i]);
	}
	NS_LOG_INFO ("Create UDP connnection and transmit data from n0 to n1");

	uint16_t port1 = 8020;
	//	    uint16_t port2 = 8000;
	uint16_t port3 = 9090;
	Address LocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port1));
	ServerHelper serverAppHelper ("ns3::UdpSocketFactory", LocalAddress);
	ApplicationContainer serverApp;
	serverApp = serverAppHelper.Install (csmaGroup[0].Get(0));
	serverApp.Start (Seconds (0.0));
	serverApp.Stop (Seconds (simTime));
////	cout << serverApp.GetN()<<endl;
	serv = StaticCast<Server> (serverApp.Get(0));
	Simulator::Schedule (Seconds (0), &CalculateThroughput);

	Address localIn2 (Inet6SocketAddress (Ipv6Address::GetAny (), port3)); //
	Address remoteOut2 (InetSocketAddress (csmaInterface[0].GetAddress (0), port1));
	NetRouterHelper netRouterHelper1 ("ns3::UdpSocketFactory", localIn2, "ns3::UdpSocketFactory", remoteOut2);
	// netRouterHelper1.SetAttribute("MiddleWare", BooleanValue (true));
	// netRouterHelper1.SetAttribute ("NetRouterID", UintegerValue (25));
	ApplicationContainer netrouterApp1;
	netrouterApp1.Add (netRouterHelper1.Install (csmaGroup[0].Get(2)));
	netrouterApp1.Start (Seconds (0));
	netrouterApp1.Stop (Seconds (simTime));

	Address RemoteAddress (Inet6SocketAddress (sixlowpanInterfaces[0].GetAddress (1, 1), port3));
	ClientHelper clientAppHelper ("ns3::UdpSocketFactory",RemoteAddress);
	clientAppHelper.SetAttribute ("ClientID", UintegerValue (251));
	clientAppHelper.SetAttribute ("PacketSize", UintegerValue (512));
	clientAppHelper.SetAttribute ("DataRate", DataRateValue (datarate)); //bit/s
	ApplicationContainer clientApp;
	clientApp = clientAppHelper.Install (panGroup[0].Get(1));
	clientApp.Start (Seconds (0));
	clientApp.Stop (Seconds (simTime));
//
//	uint32_t Max = 1000;
//	ApplicationContainer bulkApp[Max];
////	    ;
//
//	BulkSendHelper bulkSendHelper ("ns3::TcpSocketFactory", InetSocketAddress (csmaInterface[0].GetAddress(0), 8000));
//	bulkSendHelper.SetAttribute ("MaxBytes", UintegerValue (0));
//	for ( i = 0; i< Max; i++)
//	{
//
//		bulkApp[i] = bulkSendHelper.Install(csmaGroup[0].Get(2));
//		bulkApp[i].Start (Seconds (150));
//		bulkApp[i].Stop (Seconds (simTime));
//	}
//
//	PacketSinkHelper sink ("ns3::TcpSocketFactory",
//			InetSocketAddress (Ipv4Address::GetAny (), 8000));
//	ApplicationContainer sinkApps = sink.Install (csmaGroup[0].Get(0));
//	sinkApps.Start (Seconds (150));
//	sinkApps.Stop (Seconds (simTime));
////
//    Address RemoteAddress1 (Inet6SocketAddress (sixlowpanInterfaces[0].GetAddress (0, 1), port3));
//    ClientHelper clientAppHelper1 ("ns3::UdpSocketFactory",RemoteAddress1);
//    clientAppHelper1.SetAttribute ("ClientID", UintegerValue (252));
//    clientAppHelper1.SetAttribute ("PacketSize", UintegerValue (512));
//    clientAppHelper1.SetAttribute ("DataRate", DataRateValue (datarate)); //bit/s
//    ApplicationContainer clientApp1;
//    clientApp1 = clientAppHelper1.Install (panGroup[0].Get(1));
//    clientApp1.Start (Seconds (155));
//    clientApp1.Stop (Seconds (simTime));
////
//	Address localIn1 (Inet6SocketAddress (Ipv6Address::GetAny (), port3)); //
//	Address remoteOut1 (InetSocketAddress (csmaInterface[0].GetAddress (0), port1));
//	NetRouterHelper netRouterHelper2 ("ns3::UdpSocketFactory", localIn1, "ns3::UdpSocketFactory", remoteOut1);
//	netRouterHelper2.SetAttribute("MiddleWare", BooleanValue (true));
//	netRouterHelper2.SetAttribute ("NetRouterID", UintegerValue (30));
//	ApplicationContainer netrouterApp2;
//	netrouterApp2.Add (netRouterHelper2.Install (csmaGroup[0].Get(6)));
//	netrouterApp2.Start (Seconds (155.0));
//	netrouterApp2.Stop (Seconds (simTime));
////
//	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
//	Ipv4GlobalRoutingHelper g;
//	Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>
//	("test2.routes", std::ios::out);
//	g.PrintRoutingTableAllAt (Seconds (10), routingStream);



	//-----------matlab file------------//

	//    std::ostringstream title;
	//    title << "Z-" <<(uint8_t)num/100 <<(uint16_t)num%100 << ".cvs";
	//    std::string fileName       = title.str().c_str();

	// Create an aggregator.  Note that the default type is space
	// separated.
	//    Ptr<FileAggregator> aggregator =
	//    CreateObject<FileAggregator>(fileName);

	// aggregator must be turned on
	//    aggregator->Enable ();




	//	Ptr<FlowMonitor> flowMonitor;
	//	FlowMonitorHelper flowHelper;
	//	flowMonitor = flowHelper.Install(csmaGroup[0].Get(2));
	//	//    flowMonitor = flowHelper.Install(nY);
	//	// flowMonitor = flowHelper.Install(nZ);
	//	// flowMonitor = flowHelper.Install(nQ);
	//	ThroughputMonitor(&flowHelper, flowMonitor);

	/*
    Address RemoteAddress1 (Inet6SocketAddress (sixlowpanInterfaces[0].GetAddress (0, 1), port3));
    ClientHelper clientAppHelper1 ("ns3::UdpSocketFactory",RemoteAddress1);
    clientAppHelper1.SetAttribute ("ClientID", UintegerValue (252));
    clientAppHelper1.SetAttribute ("PacketSize", UintegerValue (512));
    clientAppHelper1.SetAttribute ("DataRate", DataRateValue (datarate)); //bit/s
    ApplicationContainer clientApp1;
    clientApp1 = clientAppHelper1.Install (panGroup[0].Get(1));
    clientApp1.Start (Seconds (20));
    clientApp1.Stop (Seconds (simTime));

	Address localIn1 (Inet6SocketAddress (Ipv6Address::GetAny (), port3)); //
	Address remoteOut1 (InetSocketAddress (csmaInterface[0].GetAddress (0), port1));
	NetRouterHelper netRouterHelper2 ("ns3::UdpSocketFactory", localIn1, "ns3::UdpSocketFactory", remoteOut1);
	netRouterHelper2.SetAttribute("MiddleWare", BooleanValue (true));
	ApplicationContainer netrouterApp2;
	netrouterApp2.Add (netRouterHelper2.Install (csmaGroup[0].Get(6)));
	netrouterApp2.Start (Seconds (20.0));
	netrouterApp2.Stop (Seconds (simTime));*/

	//	Address LocalAddress1 (InetSocketAddress (Ipv4Address::GetAny (), port1));
	//	ServerHelper serverAppHelper1 ("ns3::UdpSocketFactory", LocalAddress1);
	//	ApplicationContainer serverApp1;
	//	serverApp1 = serverAppHelper1.Install (csmaGroup[0].Get(0));
	//	serverApp1.Start (Seconds (15.0));
	//	serverApp1.Stop (Seconds (simTime));


	std::cout << "configuration success"<< std::endl;
	Simulator::Stop (Seconds (simTime+5));
	Simulator::Run ();

	//	//----------gunplot execution----------/
	//	    //Gnuplot ...continued
	//	    gnuplot.AddDataset (dataset);
	//	    // Open the plot file.
	//	    std::ofstream plotFile (plotFileName.c_str());
	//	    // Write the plot file.
	//	    gnuplot.GenerateOutput (plotFile);
	//	    // Close the plot file.
	//	    plotFile.close ();
	//	    aggregator->Disable ();
	Simulator::Destroy ();
}
