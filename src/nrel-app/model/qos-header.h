
#ifndef QOS_HEADER_H
#define QOS_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3 {
/**
 * \ingroup nrel-app
 * \class QosHeader
 * \QoS Packet header for TCP/UDP application.
 *
 * The header is made of a 32bits sequence number followed by
 * a 64bits time stamp.
 */
class QosHeader : public Header
{
public:
  QosHeader ();
  /**
   * \param seq the ID number
   */
  void SetID (uint32_t id);
  /**
   * \return the ID number
   */
  uint32_t GetID (void) const;
  /**
   * \return the time stamp
   */
  Time GetTs (void) const;
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint32_t m_id; //!< ID number
  uint64_t m_ts; //!< Timestamp
};

} // namespace ns3

#endif /* QOS_HEADER_H */
