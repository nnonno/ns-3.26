

#ifndef IPV4_MULTIPATH_ROUTING_H
#define IPV4_MULTIPATH_ROUTING_H

#include <list>
#include <utility>
#include <stdint.h>
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/socket.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ptr.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"

namespace ns3 {

class Packet;
class NetDevice;
class Ipv4Interface;
class Ipv4Address;
class Ipv4Header;
class Ipv4RoutingTableEntry;
class Node;

class Ipv4MultipathRouting : public Ipv4RoutingProtocol
{
public:

  static TypeId GetTypeId (void);

  Ipv4MultipathRouting ();
  virtual ~Ipv4MultipathRouting ();

  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

  virtual bool RouteInput  (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                            UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                            LocalDeliverCallback lcb, ErrorCallback ecb);
  
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;

  void AddNetworkRouteTo (Ipv4Address network, 
                          Ipv4Mask networkMask, 
                          Ipv4Address nextHop, 
                          uint32_t interface,
                          uint32_t metric = 0);
  void AddNetworkRouteTo (Ipv4Address network, 
                          Ipv4Mask networkMask, 
                          uint32_t interface,
                          uint32_t metric = 0);
  void AddHostRouteTo (Ipv4Address dest, 
                       Ipv4Address nextHop, 
                       uint32_t interface,
                       uint32_t metric = 0);
  void AddHostRouteTo (Ipv4Address dest, 
                       uint32_t interface,
                       uint32_t metric = 0);

  void SetDefaultRoute (Ipv4Address nextHop, 
                        uint32_t interface,
                        uint32_t metric = 0);

  void SetLinkUsageRatio (double ratio[100]);

  uint32_t GetNRoutes (void) const;

  Ipv4RoutingTableEntry GetRoute (uint32_t i) const;

  uint32_t GetMetric (uint32_t index) const;

  void RemoveRoute (uint32_t i);

protected:
  virtual void DoDispose (void);

private:

  bool m_EcmpEnable;

  bool m_ProportionEnable;

  Ptr<UniformRandomVariable> m_rand;

  typedef std::list<std::pair <Ipv4RoutingTableEntry *, uint32_t> > NetworkRoutes;

  typedef std::list<std::pair <Ipv4RoutingTableEntry *, uint32_t> >::iterator NetworkRoutesI;

  typedef std::list<std::pair <Ipv4RoutingTableEntry *, uint32_t> >::const_iterator NetworkRoutesCI;

  Ptr<Ipv4Route> LookupMultipath (Ipv4Address dest, Ptr<NetDevice> oif = 0);

  NetworkRoutes m_networkRoutes;

  Ptr<Ipv4> m_ipv4;

  double * m_ratio;
};

} // Namespace ns3

#endif /* IPV4_MULTIPATH_ROUTING_H */
