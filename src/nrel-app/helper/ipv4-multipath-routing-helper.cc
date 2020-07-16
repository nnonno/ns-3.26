
#include <vector>
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/assert.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ipv4-multipath-routing-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4MultipathRoutingHelper");

Ipv4MultipathRoutingHelper::Ipv4MultipathRoutingHelper()
{
}

Ipv4MultipathRoutingHelper::Ipv4MultipathRoutingHelper (const Ipv4MultipathRoutingHelper &o)
{
}

Ipv4MultipathRoutingHelper*
Ipv4MultipathRoutingHelper::Copy (void) const
{
  return new Ipv4MultipathRoutingHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
Ipv4MultipathRoutingHelper::Create (Ptr<Node> node) const
{
  return CreateObject<Ipv4MultipathRouting> ();
}

Ptr<Ipv4MultipathRouting>
Ipv4MultipathRoutingHelper::GetMultipathRouting (Ptr<Ipv4> ipv4) const
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv4RoutingProtocol> ipv4rp = ipv4->GetRoutingProtocol ();
  NS_ASSERT_MSG (ipv4rp, "No routing protocol associated with Ipv4");
  if (DynamicCast<Ipv4MultipathRouting> (ipv4rp))
    {
      NS_LOG_LOGIC ("Multipath routing found as the main IPv4 routing protocol.");
      return DynamicCast<Ipv4MultipathRouting> (ipv4rp); 
    } 
  if (DynamicCast<Ipv4ListRouting> (ipv4rp))
    {
      Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (ipv4rp);
      int16_t priority;
      for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++)
        {
          NS_LOG_LOGIC ("Searching for Multipath routing in list");
          Ptr<Ipv4RoutingProtocol> temp = lrp->GetRoutingProtocol (i, priority);
          if (DynamicCast<Ipv4MultipathRouting> (temp))
            {
              NS_LOG_LOGIC ("Found Multipath routing in list");
              return DynamicCast<Ipv4MultipathRouting> (temp);
            }
        }
    }
  NS_LOG_LOGIC ("Multipath routing not found");
  return 0;
}

} // namespace ns3
