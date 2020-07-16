//Usage:  index2 are designed to switch between links
//index2: 0 - Ethernet, 1 - WiFi Mesh,, 1 - WiMax


// Network topology
//
//	+----------+
//  | virtual  |
//  |  Linux   |
//  |   Host   |
//  |          |
//  |   eth0   |
//  +----------+
//       |
//  +----------+
//  |  Linux   |
//  |  Bridge  |
//  +----------+
//       |
//  +------------+
//  | "tap-left" |
//  +------------+
//       |         tapNode1
//       |       +--------+
//       +-------|  tap   |
//               | bridge |
//               +--------+
//               |  CSMA  |
//               +--------+
//                   |
//                   |
//                   |
//                   ====================
//                      CSMA LAN		|
//										n0                                      	n1													n2
//										+-------------------+    +---------------------------------------+     +-----------------------+
//										| 		 UDP        |    |               NetRouter               |     |          UDP          |
//										+-------------------+    +---------------+-----------------------+     +-----------------------+
//										|        IPv6  		|    | 		IPv6     |           IPv4        |     |          IPv4         |
//										+---------+---------+    +---------+-----+--------+------+-------+     +--------+------+-------+
//										| 		6LoWPAN   	|    | 		6LoWPAN  | 		  | WiFi |		 |	   |        | WiFi |       |
//										+-------------------+    +---------------+Ethernet+      + WiMax +	   +Ethernet+      + WiMax +
//										| 		LoWPAN      |    | 		LoWPAN   |  	  | Mesh |	(ss) |	   |        | Mesh |  (bs) |
//										+---------+---------+    +---------+-----+--------+------+-------+     +--------+------+-------+
//											 |         |              |        |      |      |       |              |      |       |
//										     ==========|==============         |      =======|=======|===============      |       |
//											 		   |                       |             ========|======================       |
//											 		   =========================                     ===============================
// Script Updated and developed for new topology file format and adjustable netwrok routing By
// The University of Akron

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
#include "ns3/wimax-module.h"
#include "ns3/nrel-app-module.h"
#include "ns3/fd-net-device-module.h"
#include "csv.h"
using namespace ns3;
using namespace std;

int index2 = 1, index1 = 0, change = 0;
NS_LOG_COMPONENT_DEFINE ("tap_delay_throughput_testbed");

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


int main (int argc, char** argv)
{
	double simTime = 47;
	uint32_t PacketSize=200;
	uint32_t RunNo=3;
	Time::SetResolution (Time::NS);
	std::string Topology= ("topology-v1-2.csv");// Default topology file location
	std::string TransferProtocol="UDP";
	std::string file="scratch/"+Topology;
	double ChannelDelay=3.33;
	std::string ChannelDataRate="100Mbps";
	std::string ModulationType="QAM16_12";
	std::string ServiceFlowType="UGS";
	std::string Scheduler="Simple";
	std::string mode = "ConfigureLocal";
	std::string tapName = "thetap";
	GlobalValue::Bind ("SimulatorImplementationType",
			StringValue ("ns3::RealtimeSimulatorImpl"));
	CommandLine cmd;
	//-------------------Common Parameters for all network configurations--------------------------//
	cmd.AddValue("PacketSize", "Size in bytes of packets to echo (Bytes)", PacketSize);
	cmd.AddValue("RunNo", "Run Number to use for RNG process variables", RunNo);
	cmd.AddValue("SimTime", "Duration in seconds for which the simulation is run", simTime);
	cmd.AddValue ("Topology", "Filename of Topology CSV file within scratch folder", Topology);
	cmd.AddValue ("TransferProtocol", "Transfer Protocol to use (TCP/UDP)", TransferProtocol);
	cmd.Parse (argc, argv);
	//---------------------------Parse Protocol-specific parameters for configuration-------------------------------------//
	WimaxHelper::SchedulerType wimaxSchedType;
	if(Scheduler=="Simple")
		wimaxSchedType=WimaxHelper::SCHED_TYPE_SIMPLE;
	if(Scheduler=="RTPS")
		wimaxSchedType=WimaxHelper::SCHED_TYPE_RTPS;
	if(Scheduler=="MBQOS")
		wimaxSchedType=WimaxHelper::SCHED_TYPE_MBQOS;

/*	ServiceFlow::SchedulingType wimaxSFType;
	if(ServiceFlowType=="None")
		wimaxSFType= ServiceFlow::SF_TYPE_NONE;
	if(ServiceFlowType=="Undef")
		wimaxSFType= ServiceFlow::SF_TYPE_UNDEF;
	if(ServiceFlowType=="BE")
		wimaxSFType= ServiceFlow::SF_TYPE_BE;
	if(ServiceFlowType=="NRTPS")
		wimaxSFType= ServiceFlow::SF_TYPE_NRTPS;
	if(ServiceFlowType=="RTPS")
		wimaxSFType= ServiceFlow::SF_TYPE_RTPS;
	if(ServiceFlowType=="UGS")
		wimaxSFType= ServiceFlow::SF_TYPE_UGS;
	if(ServiceFlowType=="All")
		wimaxSFType= ServiceFlow::SF_TYPE_ALL;

	WimaxPhy::ModulationType wimaxModType;
	if(ModulationType=="BPSK_12")
		wimaxModType= WimaxPhy::MODULATION_TYPE_BPSK_12;
	if(ModulationType=="QPSK_12")
		wimaxModType= WimaxPhy::MODULATION_TYPE_QPSK_12;
	if(ModulationType=="QPSK_34")
		wimaxModType= WimaxPhy::MODULATION_TYPE_QPSK_34;
	if(ModulationType=="QAM16_12")
		wimaxModType= WimaxPhy::MODULATION_TYPE_QAM16_12;
	if(ModulationType=="QAM16_34")
		wimaxModType= WimaxPhy::MODULATION_TYPE_QAM16_34;
	if(ModulationType=="QAM64_23")
		wimaxModType= WimaxPhy::MODULATION_TYPE_QAM64_23;
	if(ModulationType=="QAM64_34")
		wimaxModType= WimaxPhy::MODULATION_TYPE_QAM64_34;*/


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
	string arr_name[nodeCount];
	double arr_lon[nodeCount];
	double arr_lat[nodeCount];
	string arr_type[nodeCount];
	uint16_t arr_MG[nodeCount];
	uint16_t arr_PG[nodeCount];	// Declare array of size determined by maximum node count

	parseCSV(arr_name,arr_lon,arr_lat,arr_type,arr_MG,arr_PG,file);				// Parse CSV file and store columns in individual arrays

	MobilityHelper position = MapReading(arr_name,arr_lon,arr_lat,arr_type,nodeCount);

	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	LogComponentEnable ("tap_delay_throughput_testbed", LOG_LEVEL_INFO);
	LogComponentEnable ("Server", LOG_LEVEL_INFO);
//	LogComponentEnable ("NetRouter", LOG_LEVEL_INFO);
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
	NodeContainer ssNodes[SMcount];
	NodeContainer bsNodes[DCcount];
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
	csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (ChannelDelay)));

	for(i=0; i<DCcount; i++)
	{
		csmaDevices[i] = csma.Install (csmaGroup[i]);	// Install CSMA on CSMA devices for all areas
	}
	//------------------ Create Net devices ------------------//
	NetDeviceContainer meshDevices[DCcount];
	//------------------ Create WiFi Mesh Devices ------------------//

	// Configure YansWifiChannel
	YansWifiPhyHelper wifiPhy[DCcount];
	for(i=0; i<DCcount; i++)
	{
		wifiPhy[i] = YansWifiPhyHelper::Default ();
	}

	YansWifiChannelHelper wifiChannel[DCcount];
	for(i=0; i<DCcount; i++)
	{
		wifiChannel[i] = YansWifiChannelHelper::Default ();
	}

	for(i=0; i<DCcount; i++)
	{
		wifiChannel[i].SetPropagationDelay("ns3::RandomPropagationDelayModel","Variable", StringValue ("ns3::UniformRandomVariable[Min=0.0000001|Max=0.0000008]"));
		wifiChannel[i].AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (-20));
	}
	for(i=0; i<DCcount; i++)
	{
		wifiPhy[i].SetChannel (wifiChannel[i].Create ());
	}
	MeshHelper mesh[DCcount];
	for(i=0; i<DCcount; i++)
	{
		mesh[i] = MeshHelper::Default ();
		mesh[i].SetStackInstaller ("ns3::Dot11sStack");
		mesh[i].SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
		mesh[i].SetMacType ("RandomStart", TimeValue (Seconds (0.1)));
		mesh[i].SetStandard(WIFI_PHY_STANDARD_80211a);
		mesh[i].SetRemoteStationManager("ns3::AarfWifiManager");
		mesh[i].SetNumberOfInterfaces (2);
	}

	for(i=0; i<DCcount; i++)
	{
		meshDevices[i] = mesh[i].Install (wifiPhy[i], csmaGroup[i]);	// Install WiFi on mesh devices for Area 2
	}

	WimaxHelper wimax[DCcount];
	for(i=0; i<DCcount; i++)
	{
		wimax[i].SetPropagationLossModel (SimpleOfdmWimaxChannel::FRIIS_PROPAGATION);
	}
	NS_LOG_INFO ("Create Internet Stack");
	NetDeviceContainer ssDevs[DCcount], bsDevs[DCcount];
	for(i=0; i<DCcount; i++)
	{
		bsDevs[i] = wimax[i].Install(bsNodes[i], WimaxHelper::DEVICE_TYPE_BASE_STATION, WimaxHelper::SIMPLE_PHY_TYPE_OFDM, wimaxSchedType);
	}

	for(i=0; i<DCcount; i++)
	{
		ssDevs[i] = wimax[i].Install (ssNodes[i], WimaxHelper::DEVICE_TYPE_SUBSCRIBER_STATION, WimaxHelper::SIMPLE_PHY_TYPE_OFDM, wimaxSchedType);
	}
//	NS_LOG_INFO ("Create Internet Stack1");
//	Ptr<SubscriberStationNetDevice> ssw[SMcount];
//	Ptr<BaseStationNetDevice> bsw[DCcount];
//	int j=0;
//	for(x=0; x<DCcount; x++)
//	{
//		for (i = 0; i < ssDevs[x].GetN(); i++)
//		{
//			ssw[j] = ssDevs[x].Get(i)->GetObject<SubscriberStationNetDevice> ();
//			ssw[j]->SetModulationType (wimaxModType);
//			j++;
//		}
//		bsw[x] = bsDevs[x].Get(0)->GetObject<BaseStationNetDevice> ();
//	}

	NS_LOG_INFO ("Create Internet Stack2");
	NodeContainer tap;
	tap.Create (1);
	NodeContainer tapNodes;
	tapNodes.Add(tap.Get (0));
	tapNodes.Add(panGroup[0].Get(1));

	NetDeviceContainer tapDevices = csma.Install(tapNodes);

	Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

	InternetStackHelper internet;
	internet.Install (nodes);
	internet.Install(tap);
	NS_LOG_INFO ("Create networks and assign IPv6&IPv4 Addresses.");
	Ipv4AddressHelper ipv41;
	ipv41.SetBase ("10.4.1.0", "255.255.255.0");
	Ipv4InterfaceContainer tapInterface = ipv41.Assign (tapDevices);

	Ipv4AddressHelper ipv4;
	Ipv4InterfaceContainer csmaInterface[DCcount];
	Ipv4InterfaceContainer wifiInterface[DCcount];
	for(i=0; i<DCcount; i++)
	{
		std::ostringstream ss;
		ss <<"10.1."<< i<< ".0";
		ipv4.SetBase (ss.str().c_str(), "255.255.255.0");
		csmaInterface[i] = ipv4.Assign (csmaDevices[i]);
	}
	for(i=0; i<DCcount; i++)
	{
		std::ostringstream ss;
		ss <<"10.2."<< i<< ".0";
		ipv4.SetBase (ss.str().c_str(), "255.255.255.0");
		wifiInterface[i] = ipv4.Assign (meshDevices[i]);
	}
	Ipv4InterfaceContainer ssInterface[DCcount];
	Ipv4InterfaceContainer bsInterface[DCcount];;
	for(i=0; i<DCcount; i++)
	{
		std::ostringstream sstr4;
		sstr4 <<"10.3."<< i<< ".0";
		ipv4.SetBase (sstr4.str().c_str(), "255.255.255.0");
		bsInterface[i] = ipv4.Assign (bsDevs[i]);
		ssInterface[i] = ipv4.Assign (ssDevs[i]);
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
	//	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	TapBridgeHelper tapBridge;
	tapBridge.SetAttribute ("Mode", StringValue (mode));
	tapBridge.SetAttribute ("DeviceName", StringValue (tapName));
	tapBridge.Install (tapNodes.Get (0), tapDevices.Get (0));

	csma.EnablePcap ("matlab-ns3", tapNodes, true);
	NS_LOG_INFO ("Create UDP connnection and transmit data from n0 to n1");
	uint16_t port1 = 80;
	uint16_t port3 = 9090;
	Address LocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port1));
	ServerHelper serverAppHelper ("ns3::UdpSocketFactory", LocalAddress);

	ApplicationContainer serverApp;
	if(index2 == 0 || index2 == 1)
		serverApp = serverAppHelper.Install (csmaGroup[0].Get(0));
	else if (index2 == 2)
		serverApp = serverAppHelper.Install (bsNodes[0].Get(0));
	serverApp.Start (Seconds (1.0));
	serverApp.Stop (Seconds (simTime+3));

	Address localIn1 (InetSocketAddress (Ipv4Address::GetAny (), port1));
	Address remoteOut1 (Inet6SocketAddress (sixlowpanInterfaces[0].GetAddress (0, 1), port3)); //

	NetRouterHelper netRouterHelper1 ("ns3::UdpSocketFactory", localIn1, "ns3::UdpSocketFactory", remoteOut1);
	netRouterHelper1.SetAttribute("MiddleWare", BooleanValue (true));
	ApplicationContainer netrouterApp1;
	netrouterApp1.Add (netRouterHelper1.Install (panGroup[0].Get(1)));
	netrouterApp1.Start (Seconds (0.0));
	netrouterApp1.Stop (Seconds (simTime+3));

	Address localIn2 (Inet6SocketAddress (Ipv6Address::GetAny (), port3)); //
	Address remoteOut2;
	if(index2 == 0)
		remoteOut2 =  (InetSocketAddress (csmaInterface[0].GetAddress (0), port1));
	else if (index2 == 1)
		remoteOut2 =  (InetSocketAddress (wifiInterface[0].GetAddress (0), port1));
	else if (index2 == 2)
		remoteOut2 =  (InetSocketAddress (bsInterface[0].GetAddress (0), port1));
	NetRouterHelper netRouterHelper2 ("ns3::UdpSocketFactory", localIn2, "ns3::UdpSocketFactory", remoteOut2);
	netRouterHelper2.SetAttribute("MiddleWare", BooleanValue (true));
	netRouterHelper2.SetAttribute("NetRouterID", UintegerValue (1));
	ApplicationContainer netrouterApp2;
	if(index2 == 0 || index2 == 1)
	netrouterApp2.Add (netRouterHelper2.Install (csmaGroup[0].Get(6)));
	else if(index2 == 2)
			netrouterApp2.Add (netRouterHelper2.Install (ssNodes[0].Get(2)));
	netrouterApp2.Start (Seconds (0.0));
	netrouterApp2.Stop (Seconds (simTime+3));
	/*

	Ptr<FlowMonitor> flowMonitor;
	FlowMonitorHelper flowHelper;
	if(TransferProtocol == "UDP")
	{
		flowMonitor = flowHelper.InstallAll();
		middlewareMonitorv4(&flowHelper, flowMonitor, serverApp, nodes);
		switchpath1(plcGroup, ipv6StaticRouting, ipv6staticRoutingHelper, i1,i2);
		switchpath2(csmaGroup, ipv4MultipathRoutingHelper, i3,i4,ssInterface);
	}
	else
	{
		flowMonitor = flowHelper.InstallAll();
		middlewareMonitorv6(&flowHelper, flowMonitor, dataset, appsv4, all);
		switchpath1(plcGroup, ipv6StaticRouting, ipv6staticRoutingHelper, i1,i2);
		switchpath2(csmaGroup, ipv4MultipathRoutingHelper, i3,i4,ssInterface);
	}

	 */
//	uint16_t protoNumber;
//	if(TransferProtocol=="UDP")
//		protoNumber=17;
//	else
//		protoNumber=6;
//	j=0;
//	for(x=0; x<DCcount; x++)
//	{
//		for(i=0; i<(ssInterface[x].GetN()); i++)
//		{
//			IpcsClassifierRecord UlClassifierUgs (ssInterface[x].GetAddress(i),Ipv4Mask ("255.255.255.255"),Ipv4Address ("0.0.0.0"),Ipv4Mask ("0.0.0.0"),0,65000,100,100,protoNumber,1);
//			ServiceFlow UlServiceFlowUgs = wimax[x].CreateServiceFlow (ServiceFlow::SF_DIRECTION_UP, wimaxSFType, UlClassifierUgs);
//			ssw[j]->AddServiceFlow(UlServiceFlowUgs);
//			j++;
//		}
//	}
	if(index2 == 0)
	{
		Ptr<Ipv4> ipv = csmaGroup[0].Get(2)->GetObject<Ipv4> ();
		Simulator::Schedule (Seconds (2),&Ipv4::SetDown,ipv, 2);
		index2 = 1;
	}
//	else if(index2 == 1)
//	{
//		Ptr<Ipv4> ipv = csmaGroup[0].Get(2)->GetObject<Ipv4> ();
//		Simulator::Schedule (Seconds (2),&Ipv4::SetDown,ipv, 2);
//		index2 = 0;
//	}
	else if(index2 == 2)
	{
		Ptr<Ipv4> ipv = ssNodes[0].Get(2)->GetObject<Ipv4> ();
		Simulator::Schedule (Seconds (2),&Ipv4::SetDown,ipv, 2);
		index2 = 0;
	}


	std::cout << "configuration success"<< std::endl;
	Simulator::Stop (Seconds (simTime+5));
	Simulator::Run ();
	Simulator::Destroy ();
}
