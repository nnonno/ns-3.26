
#ifndef IPV4_MULTIPATH_ROUTING_HELPER_H
#define IPV4_MULTIPATH_ROUTING_HELPER_H

#include "ns3/ipv4.h"
#include "ns3/ipv4-multipath-routing.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"

namespace ns3 {

class Ipv4MultipathRoutingHelper : public Ipv4RoutingHelper
{
public:

  Ipv4MultipathRoutingHelper ();

  Ipv4MultipathRoutingHelper (const Ipv4MultipathRoutingHelper &);

  Ipv4MultipathRoutingHelper* Copy (void) const;

  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

  Ptr<Ipv4MultipathRouting> GetMultipathRouting (Ptr<Ipv4> ipv4) const;

private:

  Ipv4MultipathRoutingHelper &operator = (const Ipv4MultipathRoutingHelper &);
};

} // namespace ns3

#endif /* IPV4_MULTIPATH_ROUTING_HELPER_H */
