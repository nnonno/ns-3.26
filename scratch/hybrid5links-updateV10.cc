
/*  execute the following commands to modify plc module
    cd ns-allinone-3.26/ns-3.26/src/plc
	grep -rl 'CHANNEL_IDLE' ./ | xargs sed -i 's/CHANNEL_IDLE/PLCCHANNEL_IDLE/g'
	grep -rl 'CHANNEL_ACCESS_FAILURE' ./ | xargs sed -i 's/CHANNEL_ACCESS_FAILURE/PLCCHANNEL_ACCESS_FAILURE/g'
*/

//Usage:  index1 and index2 are designed to switch between links
//index1: 0 - PLC, 1 - 6LowPan
//index2: 0 - Ethernet, 1 - WiFi Mesh,, 1 - WiMax
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <time.h>

#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/output-stream-wrapper.h"
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
#include "ns3/packet-loss-counter.h"
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
#include "ns3/packet-loss-counter.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/netanim-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h" 
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/wimax-module.h"
#include "ns3/plc.h"
#include "ns3/nrel-app-module.h"
#include "csv.h"
#include "ns3/flow-probe.h"
using namespace ns3;
using namespace std;


NS_LOG_COMPONENT_DEFINE ("hybrid5links-updateV10");
class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, double interval);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  double        m_interval;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_interval (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, double interval)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_interval = interval;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  if (InetSocketAddress::IsMatchingType (m_peer))
    {
      m_socket->Bind ();
    }
  else
    {
      m_socket->Bind6 ();
    }
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_interval));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

int index1 = 0, index2 = 0, change = 0;

//The middlewareMonitorv4 and middlewareMonitorv6 functions are used to monitor the flow of packets (throughout, delay, packet loss etc) at a specific node defined in the function
void middlewareMonitorv4 (FlowMonitorHelper *flowHelper, Ptr<FlowMonitor> flowMonitor, Gnuplot2dDataset DataSet, ApplicationContainer *serverApp, NodeContainer allnodes)
{

	double localThrou=0;
	//Gnuplot gnuplot = Gnuplot("Last_Delay.eps");//Jianhua 11/18/16
  map<FlowId, FlowMonitor::FlowStats> flowStats = flowMonitor->GetFlowStats();
  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (flowHelper->GetClassifier());
  for (map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
  {
  		// if(stats->first==1){
	  Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
	  cout<<"Flow ID : " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<endl;
	  cout<<" Tx Packets = " << stats->second.txPackets<<endl;
	  cout<<" Rx Packets = " << stats->second.rxPackets<<endl;
	  cout<<" Duration    : "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<" Seconds"<<endl;
	  cout<<" Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1000 << " kbps"<<endl;
	  cout<<" Last Delay  : "<< stats->second.lastDelay.GetMilliSeconds()<<" ms"<<endl;
	  cout<<" Lost packet  : "<< stats->second.lostPackets<<endl;
//	  cout << stats->second.droppedPacke<<endl;
	  cout<<" Sum of Jitter : "<< stats->second.jitterSum.GetSeconds()<<" seconds"<<endl;
	  cout<<" Sum of Delay : "<< stats->second.delaySum.GetMilliSeconds()/stats->second.txPackets<<" ms"<<endl;
	  cout<<" Latency : "<<(stats->second.timeFirstRxPacket.GetMilliSeconds()-stats->second.timeFirstTxPacket.GetMilliSeconds())<<" ms"<<endl;
	  localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);

	  DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
	  cout<<(double)Simulator::Now().GetSeconds()<<"---------------------------------------------------------------------------"<<endl;
		// }
  }
  Simulator::Schedule(Seconds(2), &middlewareMonitorv4, flowHelper, flowMonitor, DataSet, serverApp, allnodes); // DataSet_J,

}


void middlewareMonitorv6 (FlowMonitorHelper *flowHelper, Ptr<FlowMonitor> flowMonitor, Gnuplot2dDataset DataSet, ApplicationContainer *serverApp, NodeContainer allnodes)
{
	double localThrou=0;
	//Gnuplot gnuplot = Gnuplot("Last_Delay.eps");//Jianhua 11/18/16
	map<FlowId, FlowMonitor::FlowStats> flowStats = flowMonitor->GetFlowStats();
	Ptr<Ipv6FlowClassifier> classing = DynamicCast<Ipv6FlowClassifier> (flowHelper->GetClassifier6());
	for (map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
	{
	  // if(stats->first==1){
		Ipv6FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
		cout<<"Flow ID : " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<endl;
		cout<<" Tx Packets = " << stats->second.txPackets<<endl;
		cout<<" Rx Packets = " << stats->second.rxPackets<<endl;
		cout<<" Duration    : "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<" Seconds"<<endl;
		cout<<" Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1000 << " kbps"<<endl;
		cout<<" Last Delay  : "<< stats->second.lastDelay.GetMilliSeconds()<<" ms"<<endl;
		cout<<" Lost packet  : "<< stats->second.lostPackets<<endl;
		cout<<" Sum of Jitter : "<< stats->second.jitterSum.GetSeconds()<<" seconds"<<endl;
		cout<<" Sum of Delay : "<< stats->second.delaySum.GetMilliSeconds()/stats->second.txPackets<<" ms"<<endl;
		cout<<" Latency : "<<(stats->second.timeFirstRxPacket.GetMilliSeconds()-stats->second.timeFirstTxPacket.GetMilliSeconds())<<" ms"<<endl;
		localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
		DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
		cout<<(double)Simulator::Now().GetSeconds()<<"---------------------------------------------------------------------------"<<endl;
		// }
  }
  Simulator::Schedule(Seconds(1), &middlewareMonitorv6, flowHelper, flowMonitor, DataSet, serverApp, allnodes); // DataSet_J,

}

//The MapReading and parseCSV are used to read data from the topology file
MobilityHelper MapReading(string arr_name[], double arr_lon[], double arr_lat[], string arr_type[], uint16_t nodeCount, double arr_x[], double arr_y[])
{
	double minLong=500, minLat=500, x_axis=0, y_axis=0;
	uint16_t i=0;
	// Install Mobility Model
	//Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

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
    		//positionAlloc->Add(Vector(x_axis, y_axis, 0));
                arr_x[i]=x_axis;
                arr_y[i]=y_axis;
  	}
	MobilityHelper mobility;
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	//mobility.SetPositionAllocator(positionAlloc);

	return mobility;
}


void parseCSV(string arr_name[], double arr_lon[], double arr_lat[], string arr_type[], uint16_t arr_MG[], uint16_t arr_PG[], string file)
{
	io::CSVReader<6> in(file);
  	in.read_header(io::ignore_extra_column, "name", "longitude", "latitude", "node_type", "mesh_group", "pan_group");
  	string name; double longitude; double latitude; string type; uint16_t meshGr; uint16_t panGr; uint16_t i=0;
  	while(in.read_row(name, longitude, latitude, type, meshGr, panGr))
	{
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
	//---------------------------Set Variables-------------------------------------//
	bool verbose          = true;
	string TransferProtocol="UDP";
	std::string Topology= ("topology-v1-3.csv");
	std::string file="scratch/"+Topology;
	uint64_t datarate     = 24000;
	double simulationTime = 10;
	double distance       = 50;
	//  uint32_t PacketSize=200;
	uint32_t RunNo=3;
	int index1 = 0, index2 = 1;
	Time::SetResolution (Time::NS);
	double ChannelDelay=3.33;
	std::string ChannelDataRate="100Mbps";
	std::string EncapMode= "Dix";
	uint32_t ChannelMTU=1500;
	std::string ModulationType="QAM16_12";
	uint32_t PlcLowFreq=0;
	uint32_t PlcHiFreq=10e6;
	uint32_t PlcSubBands=300;
	std::string PlcHeaderMod="BPSK_12";
	std::string PlcPayloadMod="QAM64_RATELESS";
	std::string ServiceFlowType="UGS";
	std::string Scheduler="Simple";
	uint32_t PacketSize=200;

	//---------------------------Set Commands-------------------------------------//
	CommandLine cmd;
	cmd.AddValue ("verbose", "turn on some relevant log components", verbose);
	cmd.AddValue ("udp", "UDP if set to 1, TCP otherwise", TransferProtocol);
	cmd.AddValue ("datarate", "DataRate of application (bps)", datarate);
	cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
	cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
	cmd.AddValue ("index1", "Routing strategy index between PVinverter and SmartMeter", index1);
	cmd.AddValue ("index2", "Routing strategy index between SmartMeter and Concentrator", index2);
	cmd.AddValue("ChannelDelay", "CSMA Channel propogation delay (microseconds)", ChannelDelay);
	cmd.AddValue("ChannelDataRate", "CSMA Channel maximum data rate (example: 100Mbps)", ChannelDataRate);
	cmd.AddValue("ChannelMTU", "Set Channel Maxumum Transmission Unit", ChannelMTU);
	cmd.AddValue("EncapMode", "CSMA Channel Encapsulation Mode (Dix/Llc)", EncapMode);

	cmd.AddValue("PlcLowFreq", "PLC Spectrum Lower Frequency (Hz)", PlcLowFreq);
	cmd.AddValue("PlcHiFreq", "PLC Spectrum Upper Frequency (Hz)", PlcHiFreq);
	cmd.AddValue("PlcSubBands", "Number of PLC Sub-bands available", PlcSubBands);
	cmd.AddValue("PlcHeaderMod", "PLC Header Modulation Type", PlcHeaderMod);
	cmd.AddValue("PlcPayloadMod", "PLC Payload Modulation Type", PlcPayloadMod);
	cmd.Parse (argc, argv);
  //---------------------------Parse Protocol-specific parameters for configuration-------------------------------------//
  WimaxHelper::SchedulerType wimaxSchedType;
  if(Scheduler=="Simple")
	  wimaxSchedType=WimaxHelper::SCHED_TYPE_SIMPLE;
  if(Scheduler=="RTPS")
	  wimaxSchedType=WimaxHelper::SCHED_TYPE_RTPS;
  if(Scheduler=="MBQOS")
	  wimaxSchedType=WimaxHelper::SCHED_TYPE_MBQOS;

  ServiceFlow::SchedulingType wimaxSFType;
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
	  wimaxModType= WimaxPhy::MODULATION_TYPE_QAM64_34;


  if (verbose)
  {
	  LogComponentEnable ("hybrid5links-updateV10", LOG_LEVEL_INFO);
	  LogComponentEnable ("Server", LOG_LEVEL_INFO);
  }
  //-----------------------------------------------------------------------------------------------------------------//

  RngSeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
  RngSeedManager::SetRun (RunNo);   // Changes run number from default of 1

  //----------------------------------Read Topology CSV file-----------------------------------------------------------------
  io::CSVReader<6> in(file);
  in.read_header(io::ignore_extra_column, "name", "longitude", "latitude", "node_type", "mesh_group", "pan_group");
  string name; double longitude; double latitude; string type; uint16_t meshGr; uint16_t panGr;
  uint16_t PVcount=0, DCcount=0, SMcount=0, ERcount=0, nodeCount=0, i, j, x;
  while(in.read_row(name, longitude, latitude, type, meshGr, panGr))
  {
	  if(type=="Data Concentrator")
		  DCcount++;
	  if(type=="PV Inverter")
		  PVcount++;
	  if(type=="Smart Meter")
		  SMcount++;
	  if(type=="Edge Router")
		  ERcount++;
	  nodeCount++;
  }string arr_name[nodeCount]; double arr_lon[nodeCount]; double arr_lat[nodeCount]; string arr_type[nodeCount]; uint16_t arr_MG[nodeCount]; uint16_t arr_PG[nodeCount];double arr_x[nodeCount]; double arr_y[nodeCount];	// Declare array of size determined by maximum node count

  	parseCSV(arr_name,arr_lon,arr_lat,arr_type,arr_MG,arr_PG,file);				// Parse CSV file and store columns in individual arrays

  	MobilityHelper position = MapReading(arr_name,arr_lon,arr_lat,arr_type,nodeCount,arr_x,arr_y);            // Set position model

  	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  NS_LOG_INFO ("Create channels.");


  //------------------ Create PLC Network ------------------//
  PLC_SpectrumModelHelper smHelper;
  Ptr<const SpectrumModel> sm;
  sm = smHelper.GetSpectrumModel(0, 10e6, 100);


  Ptr<SpectrumValue> txPsd = Create<SpectrumValue> (sm);
  (*txPsd) = 1e-8; // -50dBm/Hz


  NodeContainer plcGroup[PVcount];
  NodeContainer smnodes;
  NodeContainer pvnodes;
  Ptr<PLC_Node> n0[PVcount];
  Ptr<PLC_Node> r[SMcount];
  Ptr<PLC_Cable> cable[PVcount];
  Ptr<PLC_Outlet> o1[PVcount];
  Ptr<PLC_Outlet> o2[PVcount];
  PLC_NodeList pvNodesAll;
  PLC_NodeList smNodesAll;
  PLC_NodeList pvNodes[PVcount];
  PLC_NodeList smNodes[SMcount];
  PLC_NodeList plcNodes[PVcount];
  PLC_NodeList plcNodesAll;
  Ptr<PLC_ChannelHelper>  channelHelper[PVcount];
  Ptr<PLC_Channel> channel[PVcount];
  NetDeviceContainer plcDevices_all;
  NetDeviceContainer plcDevices[PVcount];

  //------------------------Set up PLC--------------------------//
  uint16_t nodeNo=0;
  for(x=0;x<PVcount;x++)
  {
	  // Create a single Cable for each PV Inverter
	  cable[x] = CreateObject<PLC_NAYY50SE_Cable> (sm);
	  // Create PLC Nodes
	  r[x]= CreateObject<PLC_Node> ();
	  n0[x]= CreateObject<PLC_Node> ();
	  //--------------- Define PLC node lists------------------//

	  for(i=0;i<nodeCount;i++)
	  {
		  if(arr_PG[i]-1==x)
		  {
			  if(arr_type[i]=="Smart Meter")
			  {
				  // Create PLC node
				  r[x]->SetName(arr_name[i]);
				  r[x]->SetPosition(arr_x[i],arr_y[i],0);
				  smNodes[x].push_back(r[x]);
				  smNodesAll.push_back(r[x]);
				  plcNodes[x].push_back(r[x]);
				  plcNodesAll.push_back(r[x]);
				  positionAlloc->Add(Vector(arr_x[i], arr_y[i], 0));
				  nodeNo++;
			  }
			  if(arr_type[i]=="PV Inverter")
			  {
				  // Create PLC node
				  n0[x]->SetName(arr_name[i]);
				  n0[x]->SetPosition(arr_x[i],arr_y[i],0);
				  pvNodes[x].push_back(n0[x]);
				  pvNodesAll.push_back(n0[x]);
				  plcNodes[x].push_back(n0[x]);
				  plcNodesAll.push_back(n0[x]);
				  positionAlloc->Add(Vector(arr_x[i], arr_y[i], 0));
				  nodeNo++;

			  }
		  }
	  }
  }
  for(x=0;x<PVcount;x++)
  {
	  // Link nodes
	  CreateObject<PLC_Line> (cable[x], n0[x], r[x]);

	  //Set up channel
	  channelHelper[x]=CreateObject<PLC_ChannelHelper>(sm);
	  channelHelper[x]->Install(plcNodes[x]);
	  channel[x] = channelHelper[x]->GetChannel();

	  // Create outlets
	  o1[x] = CreateObject<PLC_Outlet> (n0[x]);
	  o2[x] = CreateObject<PLC_Outlet> (r[x]);
  }

  Ptr<SpectrumValue> noiseFloor = CreateWorstCaseBgNoise(sm)->GetNoisePsd();
  PLC_NetDeviceHelper deviceHelper(sm, txPsd, plcNodesAll);
  deviceHelper.SetNoiseFloor(noiseFloor);
  deviceHelper.Setup();
  PLC_NetdeviceMap devMap = deviceHelper.GetNetdeviceMap();
  plcDevices_all = deviceHelper.GetNetDevices();
  NodeContainer net1 = deviceHelper.GetNS3Nodes();

  NodeContainer dcNodes;
  dcNodes.Create(DCcount);
  NodeContainer all(net1, dcNodes);

  //------------------ Setup Mobility------------------//
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

  for(x=0;x<PVcount;x++)
  {
	  channel[x]->InitTransmissionChannels();
	  channel[x]->CalcTransmissionChannels();
  }
  PLC_NodeList::iterator nit;
  j=0;
  for (nit = plcNodesAll.begin(); nit != plcNodesAll.end(); nit++)
  {
	  for(i=0;i<nodeCount;i++)
	  {
		  if((*nit)->GetName()==arr_name[i])
		  {
			  plcGroup[arr_PG[i]-1].Add(net1.Get(j));
			  plcDevices[arr_PG[i]-1].Add(plcDevices_all.Get(j));
			  j++;
		  }
	  }
  }

  //------------------ Create 6Lowpan devices ------------------//
  NetDeviceContainer d2[PVcount];
  LrWpanHelper lrWpanHelper;
  NetDeviceContainer lrwpanDevices[PVcount];
  for(i=0; i<PVcount; i++)
  {
	  lrwpanDevices[i] = lrWpanHelper.Install(plcGroup[i]);
	  lrWpanHelper.AssociateToPan (lrwpanDevices[i], 0);
  }
  SixLowPanHelper sixlowpan;

  for(i=0; i<PVcount; i++)
  {
	  d2[i] = sixlowpan.Install(lrwpanDevices[i]);
  }

  //------------------ Create Ethernet Interfaces and Create csma groups ------------------//
  NodeContainer csmaGroup[DCcount];
  NodeContainer csmaNodes_all;
  NodeContainer ssNodes[SMcount];
  NodeContainer bsNodes[DCcount];
  for(x=0;x<DCcount;x++)
  {
	  for(i=0;i<nodeCount;i++)
	  {
		  if(arr_MG[i]-1==x)
		  {
			  if((arr_type[i]=="Data Concentrator"))
			  {
				  csmaGroup[x].Add(dcNodes.Get(x));
				  csmaNodes_all.Add(dcNodes.Get(x));
				  bsNodes[x].Add(dcNodes.Get(x));
				  positionAlloc->Add(Vector(arr_x[i], arr_y[i], 0));
			  }
			  if(arr_type[i]=="Smart Meter")
			  {
				  j = 0;
				  for (nit = plcNodesAll.begin(); nit != plcNodesAll.end(); nit++)
				  {
					  if((*nit)->GetName()==arr_name[i])
					  {
						  csmaGroup[x].Add(net1.Get(j));
						  csmaNodes_all.Add(net1.Get(j));
						  ssNodes[x].Add(net1.Get(j));
					  }
					  j++;
				  }
			  }
		  }
	  }
  }

  NetDeviceContainer d3[DCcount];
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (3.33)));
  csma.SetQueue ("ns3::DropTailQueue", "Mode", StringValue ("QUEUE_MODE_PACKETS"), "MaxPackets", UintegerValue (1));

  for(i=0; i<DCcount; i++)
  {
	  d3[i] = csma.Install (csmaGroup[i]);	// Install CSMA on CSMA devices for all areas
  }

  //------------------ Create WiFi Mesh Network ------------------//

  YansWifiPhyHelper wifiPhy[DCcount];
  for(i=0; i<DCcount; i++)
  {
	  wifiPhy[i] = YansWifiPhyHelper::Default ();
  }

  YansWifiChannelHelper wifiChannel[DCcount];
  for(i=0; i<DCcount; i++)
  {
	  wifiChannel[i] = YansWifiChannelHelper::Default ();  //Jianhua, Nov11,2016
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
	  mesh[i].SetNumberOfInterfaces(2);
  }


  NetDeviceContainer d4[DCcount];
  for(i=0; i<DCcount; i++)
  {
	  d4[i] = mesh[i].Install (wifiPhy[i], csmaGroup[i]);	// Install WiFi on mesh devices for Area 2
  }
  //------------------ Create Wimax Network ------------------//
  WimaxHelper wimax[DCcount];
  for(i=0; i<DCcount; i++)
  {
	  wimax[i].SetPropagationLossModel (SimpleOfdmWimaxChannel::FRIIS_PROPAGATION);
  }
  NetDeviceContainer ssDevs[DCcount], bsDevs[DCcount];
  for(i=0; i<DCcount; i++)
  {
	  bsDevs[i] = wimax[i].Install(bsNodes[i], WimaxHelper::DEVICE_TYPE_BASE_STATION, WimaxHelper::SIMPLE_PHY_TYPE_OFDM, wimaxSchedType);
  }

  for(i=0; i<DCcount; i++)
  {
	  ssDevs[i] = wimax[i].Install (ssNodes[i], WimaxHelper::DEVICE_TYPE_SUBSCRIBER_STATION, WimaxHelper::SIMPLE_PHY_TYPE_OFDM, wimaxSchedType);
  }
  Ptr<SubscriberStationNetDevice> ssw[SMcount];
  Ptr<BaseStationNetDevice> bsw[DCcount];
  j=0;
  for(x=0; x<DCcount; x++)
  {
	  for (i = 0; i < ssDevs[x].GetN(); i++)
	  {
		  ssw[j] = ssDevs[x].Get(i)->GetObject<SubscriberStationNetDevice> ();
		  ssw[j]->SetModulationType (wimaxModType);
		  j++;
	  }
	  bsw[x] = bsDevs[x].Get(0)->GetObject<BaseStationNetDevice> ();
  }
  NS_LOG_INFO ("Create IPv6&v4 Internet Stack");
  Ipv4MultipathRoutingHelper ipv4MultipathRoutingHelper;
  Ipv4GlobalRoutingHelper ipv4globalRoutingHelper;
  Ipv4StaticRoutingHelper ipv4staticRoutingHelper;

  Ipv4ListRoutingHelper listRoutingHelper;
  listRoutingHelper.Add (ipv4MultipathRoutingHelper, 0);
  listRoutingHelper.Add (ipv4staticRoutingHelper, -5);
  listRoutingHelper.Add (ipv4globalRoutingHelper, -10);
  Config::SetDefault("ns3::Ipv4MultipathRouting::EcmpEnable",BooleanValue(true));

  InternetStackHelper stack;
  stack.SetRoutingHelper (listRoutingHelper);
  stack.Install (all);

  NS_LOG_INFO ("Create networks and assign IPv6&IPv4 Addresses.");
  Ipv6AddressHelper ipv6;
  Ipv6InterfaceContainer i1[PVcount];
  for(i=0; i<PVcount; i++)
  {
	  std::ostringstream ss;
	  ss <<"2001:" << i<< "::";
	  ipv6.SetBase (ss.str().c_str(), Ipv6Prefix (64));
	  i1[i] = ipv6.Assign (plcDevices[i]);
  }

  Ipv6InterfaceContainer i2[PVcount];
  for(i=0; i<PVcount; i++)
  {
	  std::ostringstream ss;
	  ss <<"2002:" << i<< "::";
	  ipv6.SetBase (ss.str().c_str(), Ipv6Prefix (64));
	  i2[i] = ipv6.Assign (d2[i]);
  }

  Ipv4AddressHelper ipv4;
  Ipv4InterfaceContainer i3[DCcount];
  for(i=0; i<DCcount; i++)
  {
	  std::ostringstream ss;
	  ss <<"10.1."<< i<< ".0";
	  ipv4.SetBase (ss.str().c_str(), "255.255.255.0");
	  i3[i] = ipv4.Assign (d3[i]);
  }

  Ipv4InterfaceContainer i4[DCcount];
  for(i=0; i<DCcount; i++)
  {
	  std::ostringstream ss;
	  ss <<"10.2."<< i<< ".0";
	  ipv4.SetBase (ss.str().c_str(), "255.255.255.0");
	  i4[i] = ipv4.Assign (d4[i]);
  }
  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
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

  //------------------Set UP Link Switch Middleware Configuration------------------//


  Ipv6StaticRoutingHelper ipv6staticRoutingHelper;
  Ptr<Ipv6StaticRouting> ipv6StaticRouting[PVcount];

  for (i = 0; i < PVcount ; i++)
  {
	  ipv6StaticRouting[i] = ipv6staticRoutingHelper.GetStaticRouting(plcGroup[i].Get(0)->GetObject<Ipv6> ());
	  for(uint16_t j=0; j < ipv6StaticRouting[i]->GetNRoutes(); j++)
	    {
	      ipv6StaticRouting[i]->RemoveRoute(0);
	    }
	  if(index1==0){
	     ipv6StaticRouting[i]->SetDefaultRoute (i1[i].GetAddress (1,1), 1);
	   }
	  else if(index1==1){
	     ipv6StaticRouting[i]->SetDefaultRoute (i2[i].GetAddress (1,1), 2);
	   }

  }

  Ptr<Ipv4MultipathRouting> MultipathRouting[DCcount];
  for (i = 0; i < DCcount ; i++)
    {
	  MultipathRouting[i] = ipv4MultipathRoutingHelper.GetMultipathRouting (csmaGroup[i].Get(0)->GetObject<Ipv4> ());
	  for(uint16_t j=0; j < MultipathRouting[i]->GetNRoutes(); j++)
	    {
	      MultipathRouting[i]->RemoveRoute(0);
	    }

	  if(index2==0){
	     MultipathRouting[i]->SetDefaultRoute (i3[i].GetAddress (1), 1);
	   }
	  else if(index2==1){
	      MultipathRouting[i]->SetDefaultRoute (i4[i].GetAddress (1), 2);
	    }

	  else if(index2==2){
	    MultipathRouting[i]->SetDefaultRoute (ssInterface[i].GetAddress (1), 3);
	  }


    }
  NS_LOG_INFO ("Create application and transmit data from n0 to n1");

  std::string socketFactory;
  if(TransferProtocol=="UDP")
	  socketFactory="ns3::UdpSocketFactory";
  else
	  socketFactory="ns3::TcpSocketFactory";

  ApplicationContainer sinkAppv4[SMcount], appsv4[SMcount], sinkAppv41[SMcount], appsv41[SMcount];
  x=0;
  if(index2 == 0 || index2 == 1)
  {
	  for(i=0; i<DCcount; i++)
	  {
		  for(j=1; j<csmaGroup[i].GetN(); j++)
		  {
			  uint16_t port = 50000;
			  Address apLocalAddress;
			  if(index2 == 0)
				  apLocalAddress = InetSocketAddress (i3[i].GetAddress(0), port+x);
			  else if(index2 == 1)
				  apLocalAddress = InetSocketAddress (i4[i].GetAddress(0), port+x);
			  PacketSinkHelper packetSinkHelper (socketFactory, apLocalAddress);
			  sinkAppv4[x] = packetSinkHelper.Install (csmaGroup[i].Get(0));
			  sinkAppv4[x].Start (Seconds (0.0));
			  sinkAppv4[x].Stop (Seconds (simulationTime));

			  if(index2 == 0)
			  {
				  OnOffHelper onoff(socketFactory,i3[i].GetAddress(j));
				  onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
				  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
				  onoff.SetAttribute ("PacketSize", UintegerValue (PacketSize));
				  onoff.SetAttribute ("DataRate", DataRateValue (datarate)); //bit/s

				  AddressValue remoteAddress(InetSocketAddress (i3[i].GetAddress(0), port+x));
				  onoff.SetAttribute ("Remote", remoteAddress);
				  appsv4[x].Add (onoff.Install (csmaGroup[i].Get(j)));
				  appsv4[x].Start (Seconds (0+x*0.033));
				  appsv4[x].Stop (Seconds (simulationTime));

			  }
			  if(index2 == 1)
			  {
				  OnOffHelper onoff(socketFactory,i4[i].GetAddress(j));
				  onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
				  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
				  onoff.SetAttribute ("PacketSize", UintegerValue (PacketSize));
				  onoff.SetAttribute ("DataRate", DataRateValue (datarate)); //bit/s

				  AddressValue remoteAddress(InetSocketAddress (i4[i].GetAddress(0), port+x));
				  onoff.SetAttribute ("Remote", remoteAddress);
				  appsv4[x].Add (onoff.Install (csmaGroup[i].Get(j)));
			  }
			  appsv4[x].Start (Seconds (0+x*0.033));
			  appsv4[x].Stop (Seconds (simulationTime));
			  x++;
		  }
	  }
  }
  x = 0;
  if (index2 == 2)
  {
	  for(i=0; i<DCcount; i++)
	  {
		  for(j=0; j<ssNodes[i].GetN(); j++)
		  {
			  uint16_t port = 50000;
			  Address apLocalAddress (InetSocketAddress (bsInterface[i].GetAddress(0), port+x));
			  PacketSinkHelper packetSinkHelper (socketFactory, apLocalAddress);
			  sinkAppv41[x] = packetSinkHelper.Install (bsNodes[i].Get(0));
			  sinkAppv41[x].Start (Seconds (0.0));
			  sinkAppv41[x].Stop (Seconds (simulationTime));

			  OnOffHelper onoff (socketFactory,ssInterface[i].GetAddress(j));
			  onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
			  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
			  onoff.SetAttribute ("PacketSize", UintegerValue (PacketSize));
			  onoff.SetAttribute ("DataRate", DataRateValue (datarate)); //bit/s

			  AddressValue remoteAddress (InetSocketAddress (bsInterface[i].GetAddress(0), port+x));
			  onoff.SetAttribute ("Remote", remoteAddress);
			  appsv41[x].Add (onoff.Install (ssNodes[i].Get(j)));
			  appsv41[x].Start (Seconds (0+x*0.033));
			  appsv41[x].Stop (Seconds (simulationTime));
			  x++;
		  }
	  }
  }
  ApplicationContainer sinkAppv6[PVcount], appsv6[PVcount];
  x=0;
  for(i=0; i<PVcount; i++)
	{
		for(j=1; j<plcGroup[i].GetN(); j++)
		{
			uint16_t port = 80;
			Address apLocalAddress;

			if (index1 == 0)
				apLocalAddress = Inet6SocketAddress (i1[i].GetAddress(0,1), port+x);
			else if (index1 == 1)
				apLocalAddress = Inet6SocketAddress (i2[i].GetAddress(0,1), port+x);
			PacketSinkHelper packetSinkHelper (socketFactory, apLocalAddress);
			sinkAppv6[x] = packetSinkHelper.Install (plcGroup[i].Get(0));
			sinkAppv6[x].Start (Seconds (0.0));
			sinkAppv6[x].Stop (Seconds (simulationTime));
			if (index1 == 0)
			{

				OnOffHelper onoff(socketFactory,i1[i].GetAddress(j,1));
				onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
				onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
				onoff.SetAttribute ("PacketSize", UintegerValue (PacketSize));
				onoff.SetAttribute ("DataRate", DataRateValue (datarate));
				AddressValue remoteAddress(Inet6SocketAddress (i1[i].GetAddress(0,1), port+x));
				onoff.SetAttribute ("Remote", remoteAddress);
				appsv6[x].Add (onoff.Install (plcGroup[i].Get(j)));
			}
			if (index1 == 1)
			{
				OnOffHelper onoff(socketFactory,i2[i].GetAddress(j,1));
				onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
				onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
				onoff.SetAttribute ("PacketSize", UintegerValue (PacketSize));
				onoff.SetAttribute ("DataRate", DataRateValue (datarate));
				AddressValue remoteAddress(Inet6SocketAddress (i2[i].GetAddress(0,1), port+x));
				onoff.SetAttribute ("Remote", remoteAddress);
				appsv6[x].Add (onoff.Install (plcGroup[i].Get(j)));

			}
			appsv6[x].Start (Seconds (0+x*0.002));
			appsv6[j].Stop (Seconds (simulationTime));
			x++;

		}
	}
	uint16_t protoNumber;
	if(TransferProtocol=="UDP")
		protoNumber=17;
	else
		protoNumber=6;
	j=0;
	for(x=0; x<DCcount; x++)
	{
		for(i=0; i<(ssInterface[x].GetN()); i++)
		{
			IpcsClassifierRecord UlClassifierUgs (ssInterface[x].GetAddress(i),Ipv4Mask ("255.255.255.255"),Ipv4Address ("0.0.0.0"),Ipv4Mask ("0.0.0.0"),0,65000,100,100,protoNumber,1);
  			ServiceFlow UlServiceFlowUgs = wimax[x].CreateServiceFlow (ServiceFlow::SF_DIRECTION_UP, wimaxSFType, UlClassifierUgs);
			ssw[j]->AddServiceFlow(UlServiceFlowUgs);
			j++;
		}
  	}


	//-----------Gnuplot------------//
	std::string fileNameWithNoExtension = "hybrid5LINKS_Throuhput";
	std::string graphicsFileName        = fileNameWithNoExtension + ".png";
	std::string plotFileName            = fileNameWithNoExtension + ".plt";
	std::string plotTitle               = "Throughput vs Time";
	std::string dataTitle               = "Throughput";

// Instantiate the plot and set its title.
	Gnuplot gnuplot (graphicsFileName);
	gnuplot.SetTitle (plotTitle);

// Make the graphics file a .png file. This file will be used by gnuplot
	gnuplot.SetTerminal ("png");

// Set the labels for each axis.
	gnuplot.SetLegend ("Time(Seconds)", "Throughput(Mbps)");

	Gnuplot2dDataset dataset;
	dataset.SetTitle (dataTitle);
	dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);
	NS_LOG_INFO ("Run Simulation.");

// Setup FlowMonitor to call middlewareMonitorv4 and middlewareMonitorv6 functions
	Ptr<FlowMonitor> flowMonitor;
	FlowMonitorHelper flowHelper;
	if(TransferProtocol == "UDP")
	{
		flowMonitor = flowHelper.InstallAll();
		middlewareMonitorv4(&flowHelper, flowMonitor, dataset, appsv4, all);

	}
	else
	{
		flowMonitor = flowHelper.InstallAll();
		middlewareMonitorv6(&flowHelper, flowMonitor, dataset, appsv4, all);
	}

	Simulator::Stop (Seconds (60.));
	Simulator::Run ();
 	//----------gunplot execution----------/
	//Gnuplot ...continued
	gnuplot.AddDataset (dataset);
	// Open the plot file.
	std::ofstream plotFile (plotFileName.c_str());
	// Write the plot file.
	gnuplot.GenerateOutput (plotFile);
	// Close the plot file.
	plotFile.close ();
  Simulator::Destroy ();

  NS_LOG_INFO ("Done.");
  return 0;
}

