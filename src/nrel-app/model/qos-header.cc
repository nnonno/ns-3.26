
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "qos-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QosHeader");

NS_OBJECT_ENSURE_REGISTERED (QosHeader);

QosHeader::QosHeader ()
  : m_id (0),
    m_ts (Simulator::Now ().GetTimeStep ())
{
  NS_LOG_FUNCTION (this);
}

void
QosHeader::SetID (uint32_t id)
{
  NS_LOG_FUNCTION (this << id);
  m_id = id;
}
uint32_t
QosHeader::GetID (void) const
{
  NS_LOG_FUNCTION (this);
  return m_id;
}

Time
QosHeader::GetTs (void) const
{
  NS_LOG_FUNCTION (this);
  return TimeStep (m_ts);
}

TypeId
QosHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QosHeader")
    .SetParent<Header> ()
    .SetGroupName("nrel-app")
    .AddConstructor<QosHeader> ()
  ;
  return tid;
}
TypeId
QosHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
QosHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(id=" << m_id << " time=" << TimeStep (m_ts).GetSeconds () << ")";
}
uint32_t
QosHeader::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 4+8;
}

void
QosHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_id);
  i.WriteHtonU64 (m_ts);
}
uint32_t
QosHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_id = i.ReadNtohU32 ();
  m_ts = i.ReadNtohU64 ();
  return GetSerializedSize ();
}

} // namespace ns3
