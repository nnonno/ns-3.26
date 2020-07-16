/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "netrouter.h"
#include "qos-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NetRouter");

NS_OBJECT_ENSURE_REGISTERED (NetRouter);

TypeId 
NetRouter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetRouter")
    .SetParent<Application> ()
    .SetGroupName("nrel-app")
    .AddConstructor<NetRouter> ()
    .AddAttribute ("LocalIn",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&NetRouter::m_localin),
                   MakeAddressChecker ())
    .AddAttribute ("RemoteOut",
                   "The Address on which to Bind the tx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&NetRouter::m_remoteout),
                   MakeAddressChecker ())
    .AddAttribute ("ProtocolIn",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&NetRouter::m_tidin),
                   MakeTypeIdChecker ())
    .AddAttribute ("ProtocolOut",
                   "The type id of the protocol to use for the tx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&NetRouter::m_tidout),
                   MakeTypeIdChecker ())
  ;
  return tid;
}

NetRouter::NetRouter ()
  : m_socketin (0),
    m_socketout (0),
    m_connected (false),
    m_id (0),
    m_count(0)
{
  NS_LOG_FUNCTION (this);
}

NetRouter::~NetRouter()
{
  NS_LOG_FUNCTION (this);
}

Ptr<Socket>
NetRouter::GetSocketIn (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketin;
}

Ptr<Socket>
NetRouter::GetSocketOut (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketout;
}

std::list<Ptr<Socket> >
NetRouter::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void NetRouter::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socketin = 0;
  m_socketout = 0;
  m_socketList.clear ();
  // chain up
  Application::DoDispose ();
}


// Application Methods
void NetRouter::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // NS_LOG_INFO ("NetRouter starts");
  // Create the socket if not already
  if (!m_socketin)
    {
      m_socketin = Socket::CreateSocket (GetNode (), m_tidin);
      m_socketin->Bind (m_localin);
      m_socketin->Listen ();
      m_socketin->ShutdownSend ();
      if (addressUtils::IsMulticast (m_localin))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socketin);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_localin);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
      m_socketin->SetRecvCallback (MakeCallback (&NetRouter::HandleRead, this));
      m_socketin->SetAcceptCallback (
        MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
        MakeCallback (&NetRouter::HandleAccept, this));
      m_socketin->SetCloseCallbacks (
        MakeCallback (&NetRouter::HandlePeerClose, this),
        MakeCallback (&NetRouter::HandlePeerError, this));
    }

  if (!m_socketout)
    {
      m_socketout = Socket::CreateSocket (GetNode (), m_tidout);
      
      if (Inet6SocketAddress::IsMatchingType (m_remoteout))
        {
          m_socketout->Bind6 ();
        }
      else if (InetSocketAddress::IsMatchingType (m_remoteout))
        {
          m_socketout->Bind ();
        }

      m_socketout->Connect (m_remoteout);
      m_socketout->SetAllowBroadcast (true);
      m_socketout->ShutdownRecv ();
      
      m_socketout->SetConnectCallback (
        MakeCallback (&NetRouter::ConnectionSucceeded, this),
        MakeCallback (&NetRouter::ConnectionFailed, this));
    }

}

void NetRouter::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO("NetRouter has forwarded "<<m_count<<" packets to Server");
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socketin) 
    {
      m_socketin->Close ();
      m_socketin->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  if (m_socketout)
    {
      m_socketout->Close ();
      m_connected = false;
    }
  // NS_LOG_INFO ("NetRouter is closed");
}

void NetRouter::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Address from;
  while ((m_packet = socket->RecvFrom (from)))
    {
      if (m_packet->GetSize () == 0)
        { //EOF
          break;
        }
      QosHeader QosTx;
      m_packet->RemoveHeader (QosTx);
      if(QosTx.GetID()==0) continue;
      if(QosTx.GetTs().GetMilliSeconds()<=0) continue;
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s NetRouter received "
                       <<  m_packet->GetSize ()+12 << " bytes from Client "
                       << QosTx.GetID());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s NetRouter received "
                       <<  m_packet->GetSize ()+12 << " bytes from Client "
                       << QosTx.GetID());
          m_packet->AddHeader (QosTx);
          m_packet->RemoveAllPacketTags();
          SendData ();
        }

      // m_packet->AddHeader (QosTx);
      // m_packet->RemoveAllPacketTags();
      // SendData ();

    }
}

void NetRouter::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
void NetRouter::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void NetRouter::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&NetRouter::HandleRead, this));
  m_socketList.push_back (s);
}

void NetRouter::SendData (void)
{
  NS_LOG_FUNCTION (this);
  m_socketout->Send (m_packet);
  m_count++;
}

void NetRouter::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("NetRouter, Connection succeeded");
  m_connected = true;
}

void NetRouter::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("NetRouter, Connection Failed");
}

} // Namespace ns3
