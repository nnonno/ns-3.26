
#include <iomanip>
#include <cstring>
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-route.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ipv4-multipath-routing.h"

using std::make_pair;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4MultipathRouting");

NS_OBJECT_ENSURE_REGISTERED (Ipv4MultipathRouting);

TypeId
Ipv4MultipathRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4MultipathRouting")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("nrel-app")
    .AddConstructor<Ipv4MultipathRouting> ()
    .AddAttribute ("EcmpEnable",
                   "Set to true if packets are randomly routed among Multipath; set to false for using only one route consistently",
                   BooleanValue (false),
                   MakeBooleanAccessor (&Ipv4MultipathRouting::m_EcmpEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("ProportionEnable",
                   "Set to true if packets are proportional routed among Multipath; set to false for using only one route consistently",
                   BooleanValue (false),
                   MakeBooleanAccessor (&Ipv4MultipathRouting::m_ProportionEnable),
                   MakeBooleanChecker ())
  ;
  return tid;
}

Ipv4MultipathRouting::Ipv4MultipathRouting () 
  : m_EcmpEnable (false),
    m_ProportionEnable(false),
    m_ipv4 (0)
{
  NS_LOG_FUNCTION (this);
  m_rand = CreateObject<UniformRandomVariable> ();
}

void 
Ipv4MultipathRouting::AddNetworkRouteTo (Ipv4Address network, 
                                      Ipv4Mask networkMask, 
                                      Ipv4Address nextHop, 
                                      uint32_t interface,
                                      uint32_t metric)
{
  NS_LOG_FUNCTION (this << network << " " << networkMask << " " << nextHop << " " << interface << " " << metric);
  Ipv4RoutingTableEntry *route = new Ipv4RoutingTableEntry ();
  *route = Ipv4RoutingTableEntry::CreateNetworkRouteTo (network,
                                                        networkMask,
                                                        nextHop,
                                                        interface);
  m_networkRoutes.push_back (make_pair (route,metric));
}

void 
Ipv4MultipathRouting::AddNetworkRouteTo (Ipv4Address network, 
                                      Ipv4Mask networkMask, 
                                      uint32_t interface,
                                      uint32_t metric)
{
  NS_LOG_FUNCTION (this << network << " " << networkMask << " " << interface << " " << metric);
  Ipv4RoutingTableEntry *route = new Ipv4RoutingTableEntry ();
  *route = Ipv4RoutingTableEntry::CreateNetworkRouteTo (network,
                                                        networkMask,
                                                        interface);
  m_networkRoutes.push_back (make_pair (route,metric));
}

void 
Ipv4MultipathRouting::AddHostRouteTo (Ipv4Address dest, 
                                   Ipv4Address nextHop,
                                   uint32_t interface,
                                   uint32_t metric)
{
  NS_LOG_FUNCTION (this << dest << " " << nextHop << " " << interface << " " << metric);
  AddNetworkRouteTo (dest, Ipv4Mask::GetOnes (), nextHop, interface, metric);
}

void 
Ipv4MultipathRouting::AddHostRouteTo (Ipv4Address dest, 
                                   uint32_t interface,
                                   uint32_t metric)
{
  NS_LOG_FUNCTION (this << dest << " " << interface << " " << metric);
  AddNetworkRouteTo (dest, Ipv4Mask::GetOnes (), interface, metric);
}

void 
Ipv4MultipathRouting::SetDefaultRoute (Ipv4Address nextHop, 
                                    uint32_t interface,
                                    uint32_t metric)
{
  NS_LOG_FUNCTION (this << nextHop << " " << interface << " " << metric);
  AddNetworkRouteTo (Ipv4Address ("0.0.0.0"), Ipv4Mask::GetZero (), nextHop, interface, metric);
}

void 
Ipv4MultipathRouting::SetLinkUsageRatio (double * ratio)
{
  NS_LOG_FUNCTION (this);
  m_ratio = ratio;
  // memcpy(m_ratio,ratio,10);
}



Ptr<Ipv4Route>
Ipv4MultipathRouting::LookupMultipath (Ipv4Address dest, Ptr<NetDevice> oif)
{
  NS_LOG_FUNCTION (this << dest << " " << oif);
  Ptr<Ipv4Route> rtentry = 0;
  uint16_t longest_mask = 0;
  uint32_t shortest_metric = 0xffffffff;
  typedef std::vector<Ipv4RoutingTableEntry*> RouteVec_t;
  RouteVec_t allRoutes;

  for (NetworkRoutesI i = m_networkRoutes.begin (); 
       i != m_networkRoutes.end (); 
       i++) 
    {
      Ipv4RoutingTableEntry *j=i->first;
      uint32_t metric =i->second;
      Ipv4Mask mask = (j)->GetDestNetworkMask ();
      uint16_t masklen = mask.GetPrefixLength ();
      Ipv4Address entry = (j)->GetDestNetwork ();
      NS_LOG_LOGIC ("Searching for route to " << dest << ", checking against route to " << entry << "/" << masklen);
      if (mask.IsMatch (dest, entry)) 
        {
          NS_LOG_LOGIC ("Found global network route " << j << ", mask length " << masklen << ", metric " << metric);
          if (oif != 0)
            {
              if (oif != m_ipv4->GetNetDevice (j->GetInterface ()))
                {
                  NS_LOG_LOGIC ("Not on requested interface, skipping");
                  continue;
                }
            }
          if (masklen < longest_mask) // Not interested if got shorter mask
            {
              NS_LOG_LOGIC ("Previous match longer, skipping");
              continue;
            }
          if (masklen > longest_mask) // Reset metric if longer masklen
            {
              shortest_metric = 0xffffffff;
            }
          longest_mask = masklen;
          if (metric > shortest_metric)
            {
              NS_LOG_LOGIC ("Equal mask length, but previous metric shorter, skipping");
              continue;
            }
          shortest_metric = metric;
          
          allRoutes.push_back (j);

        }
    }

  if (allRoutes.size () > 0 ) // if route(s) is found
    {

      // pick up one of the routes uniformly at random if random
      // ECMP routing is enabled, or always select the first route
      // consistently if random ECMP routing is disabled
      uint32_t selectIndex;
      if (m_EcmpEnable)
        {
          selectIndex = m_rand->GetInteger (0, allRoutes.size ()-1);
        }
      else if(m_ProportionEnable)
        {
          // NS_LOG_INFO("done");
          double temp = m_rand->GetInteger (0, 1000)/1000.0;
          uint16_t i = 0;
          NS_LOG_INFO(allRoutes.size ());
          NS_LOG_INFO(sizeof(m_ratio));

          for(i = 0; i < allRoutes.size (); i=i+1){
              if(*(m_ratio+i)>temp)
                break;
          }
          selectIndex = i-1;


        }
      else 
        {
          selectIndex = 0;
        }
      Ipv4RoutingTableEntry* route = allRoutes.at (selectIndex); 
      // create a Ipv4Route object from the selected routing table entry
      uint32_t interfaceIdx = route->GetInterface ();
      rtentry = Create<Ipv4Route> ();
      rtentry->SetDestination (route->GetDest ());
      rtentry->SetSource (m_ipv4->GetAddress (route->GetInterface (), 0).GetLocal ());
      rtentry->SetGateway (route->GetGateway ());
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
    }

  if (rtentry != 0)
    {
      NS_LOG_LOGIC ("Matching route via " << rtentry->GetGateway () << " at the end");
    }
  else
    {
      NS_LOG_LOGIC ("No matching route to " << dest << " found");
    }
  return rtentry;
}

uint32_t 
Ipv4MultipathRouting::GetNRoutes (void) const
{
  NS_LOG_FUNCTION (this);
  return m_networkRoutes.size ();;
}

Ipv4RoutingTableEntry 
Ipv4MultipathRouting::GetRoute (uint32_t index) const
{
  NS_LOG_FUNCTION (this << index);
  uint32_t tmp = 0;
  for (NetworkRoutesCI j = m_networkRoutes.begin (); 
       j != m_networkRoutes.end (); 
       j++) 
    {
      if (tmp  == index)
        {
          return j->first;
        }
      tmp++;
    }
  NS_ASSERT (false);
  // quiet compiler.
  return 0;
}

uint32_t
Ipv4MultipathRouting::GetMetric (uint32_t index) const
{
  NS_LOG_FUNCTION (this << index);
  uint32_t tmp = 0;
  for (NetworkRoutesCI j = m_networkRoutes.begin ();
       j != m_networkRoutes.end (); 
       j++) 
    {
      if (tmp == index)
        {
          return j->second;
        }
      tmp++;
    }
  NS_ASSERT (false);
  // quiet compiler.
  return 0;
}

void 
Ipv4MultipathRouting::RemoveRoute (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  uint32_t tmp = 0;
  for (NetworkRoutesI j = m_networkRoutes.begin (); 
       j != m_networkRoutes.end (); 
       j++) 
    {
      if (tmp == index)
        {
          delete j->first;
          m_networkRoutes.erase (j);
          return;
        }
      tmp++;
    }
  NS_ASSERT (false);
}


Ptr<Ipv4Route> 
Ipv4MultipathRouting::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << p<< header << oif << sockerr);
  Ipv4Address destination = header.GetDestination ();
  Ptr<Ipv4Route> rtentry = 0;

  rtentry = LookupMultipath (destination, oif);
  if (rtentry)
    { 
      sockerr = Socket::ERROR_NOTERROR;
    }
  else
    { 
      sockerr = Socket::ERROR_NOROUTETOHOST;
    }
  return rtentry;
}

bool 
Ipv4MultipathRouting::RouteInput  (Ptr<const Packet> p, const Ipv4Header &ipHeader, Ptr<const NetDevice> idev,
                                UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                LocalDeliverCallback lcb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << p << ipHeader << ipHeader.GetSource () << ipHeader.GetDestination () << idev << &ucb << &mcb << &lcb << &ecb);

  NS_ASSERT (m_ipv4 != 0);
  // Check if input device supports IP 
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  uint32_t iif = m_ipv4->GetInterfaceForDevice (idev); 


  NS_LOG_LOGIC ("Unicast destination");
  for (uint32_t j = 0; j < m_ipv4->GetNInterfaces (); j++)
    {
      for (uint32_t i = 0; i < m_ipv4->GetNAddresses (j); i++)
        {
          Ipv4InterfaceAddress iaddr = m_ipv4->GetAddress (j, i);
          Ipv4Address addr = iaddr.GetLocal ();
          if (addr.IsEqual (ipHeader.GetDestination ()))
            {
              if (j == iif)
                {
                  NS_LOG_LOGIC ("For me (destination " << addr << " match)");
                }
              else
                {
                  NS_LOG_LOGIC ("For me (destination " << addr << " match) on another interface " << ipHeader.GetDestination ());
                }
              lcb (p, ipHeader, iif);
              return true;
            }
          if (ipHeader.GetDestination ().IsEqual (iaddr.GetBroadcast ()))
            {
              NS_LOG_LOGIC ("For me (interface broadcast address)");
              lcb (p, ipHeader, iif);
              return true;
            }
          NS_LOG_LOGIC ("Address "<< addr << " not a match");
        }
    }
  // Check if input device supports IP forwarding
  if (m_ipv4->IsForwarding (iif) == false)
    {
      NS_LOG_LOGIC ("Forwarding disabled for this interface");
      ecb (p, ipHeader, Socket::ERROR_NOROUTETOHOST);
      return false;
    }
  // Next, try to find a route
  Ptr<Ipv4Route> rtentry = LookupMultipath (ipHeader.GetDestination ());
  if (rtentry != 0)
    {
      NS_LOG_LOGIC ("Found unicast destination- calling unicast callback");
      ucb (rtentry, p, ipHeader);  // unicast forwarding callback
      return true;
    }
  else
    {
      NS_LOG_LOGIC ("Did not find unicast destination- returning false");
      return false; // Let other routing protocols try to handle this
    }
}

Ipv4MultipathRouting::~Ipv4MultipathRouting ()
{
  NS_LOG_FUNCTION (this);
}

void
Ipv4MultipathRouting::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  for (NetworkRoutesI j = m_networkRoutes.begin (); 
       j != m_networkRoutes.end (); 
       j = m_networkRoutes.erase (j)) 
    {
      delete (j->first);
    }
  m_ipv4 = 0;
  Ipv4RoutingProtocol::DoDispose ();
}

void 
Ipv4MultipathRouting::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);
  // If interface address and network mask have been set, add a route
  // to the network of the interface (like e.g. ifconfig does on a
  // Linux box)
  for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); j++)
    {
      if (m_ipv4->GetAddress (i,j).GetLocal () != Ipv4Address () &&
          m_ipv4->GetAddress (i,j).GetMask () != Ipv4Mask () &&
          m_ipv4->GetAddress (i,j).GetMask () != Ipv4Mask::GetOnes ())
        {
          AddNetworkRouteTo (m_ipv4->GetAddress (i,j).GetLocal ().CombineMask (m_ipv4->GetAddress (i,j).GetMask ()),
                             m_ipv4->GetAddress (i,j).GetMask (), i);
        }
    }
}

void 
Ipv4MultipathRouting::NotifyInterfaceDown (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);
  // Remove all static routes that are going through this interface
  for (NetworkRoutesI it = m_networkRoutes.begin (); it != m_networkRoutes.end (); )
    {
      if (it->first->GetInterface () == i)
        {
          delete it->first;
          it = m_networkRoutes.erase (it);
        }
      else
        {
          it++;
        }
    }
}

void 
Ipv4MultipathRouting::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << interface << " " << address.GetLocal ());
  if (!m_ipv4->IsUp (interface))
    {
      return;
    }

  Ipv4Address networkAddress = address.GetLocal ().CombineMask (address.GetMask ());
  Ipv4Mask networkMask = address.GetMask ();
  if (address.GetLocal () != Ipv4Address () &&
      address.GetMask () != Ipv4Mask ())
    {
      AddNetworkRouteTo (networkAddress,
                         networkMask, interface);
    }
}
void 
Ipv4MultipathRouting::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << interface << " " << address.GetLocal ());
  if (!m_ipv4->IsUp (interface))
    {
      return;
    }
  Ipv4Address networkAddress = address.GetLocal ().CombineMask (address.GetMask ());
  Ipv4Mask networkMask = address.GetMask ();
  // Remove all static routes that are going through this interface
  // which reference this network
  for (NetworkRoutesI it = m_networkRoutes.begin (); it != m_networkRoutes.end (); )
    {
      if (it->first->GetInterface () == interface
          && it->first->IsNetwork ()
          && it->first->GetDestNetwork () == networkAddress
          && it->first->GetDestNetworkMask () == networkMask)
        {
          delete it->first;
          it = m_networkRoutes.erase (it);
        }
      else
        {
          it++;
        }
    }
}

void 
Ipv4MultipathRouting::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_LOG_FUNCTION (this << ipv4);
  NS_ASSERT (m_ipv4 == 0 && ipv4 != 0);
  m_ipv4 = ipv4;
}

void
Ipv4MultipathRouting::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{
  NS_LOG_FUNCTION (this << stream);
  std::ostream* os = stream->GetStream ();
  if (GetNRoutes () > 0)
    {
      *os << "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface" << std::endl;
      for (uint32_t j = 0; j < GetNRoutes (); j++)
        {
          std::ostringstream dest, gw, mask, flags;
          Ipv4RoutingTableEntry route = GetRoute (j);
          dest << route.GetDest ();
          *os << std::setiosflags (std::ios::left) << std::setw (16) << dest.str ();
          gw << route.GetGateway ();
          *os << std::setiosflags (std::ios::left) << std::setw (16) << gw.str ();
          mask << route.GetDestNetworkMask ();
          *os << std::setiosflags (std::ios::left) << std::setw (16) << mask.str ();
          flags << "U";
          if (route.IsHost ())
            {
              flags << "HS";
            }
          else if (route.IsGateway ())
            {
              flags << "GS";
            }
          *os << std::setiosflags (std::ios::left) << std::setw (6) << flags.str ();
          *os << std::setiosflags (std::ios::left) << std::setw (7) << GetMetric (j);
          // Ref ct not implemented
          *os << "-" << "      ";
          // Use not implemented
          *os << "-" << "   ";
          if (Names::FindName (m_ipv4->GetNetDevice (route.GetInterface ())) != "")
            {
              *os << Names::FindName (m_ipv4->GetNetDevice (route.GetInterface ()));
            }
          else
            {
              *os << route.GetInterface ();
            }
          *os << std::endl;
        }
    }
}

} // namespace ns3
