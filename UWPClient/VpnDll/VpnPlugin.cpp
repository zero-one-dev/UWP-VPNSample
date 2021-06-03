#include "pch.h"
#include "VpnPlugin.h"
#include "strutil.h"
#include <Windows.h>
#include <string>
#include "winrt/Windows.Foundation.h"


using namespace winrt;
using namespace Windows::Networking;
using namespace Windows::Networking::Vpn;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;


namespace winrt::VpnDll::implementation
{	
	void VpnPlugin::Connect(VpnChannel const& channel)
	{
		try
		{
			DatagramSocket tunnel;
			channel.AssociateTransport(tunnel, nullptr);
			HostName serverHostname = channel.Configuration().ServerHostNameList().GetAt(0);							
			std::wstring custom_field(channel.Configuration().CustomField());
			std::map<std::wstring, std::wstring> map_param = parse_param(custom_field);
			std::wstring ipv4 = map_param[L"ipv4"];		
			std::wstring port = map_param[L"port"];	
			tunnel.ConnectAsync(serverHostname, port.c_str()).get();			
			Configure(channel, ipv4);
		}
		catch (hresult_error const& ex)
		{
			channel.TerminateConnection(ex.message());
		}
	}


	void VpnPlugin::Disconnect(VpnChannel const& channel)
	{
		try
		{
			channel.Stop();
		}
		catch (winrt::hresult_error const& ex)
		{
		}
	}

	void VpnPlugin::GetKeepAlivePayload(VpnChannel const& /*channel*/, VpnPacketBuffer& /*keepAlivePacket*/)
	{
	}





	void VpnPlugin::Encapsulate(VpnChannel const& /*channel*/, VpnPacketBufferList const& packets, VpnPacketBufferList const& encapulatedPackets)
	{	
		while (packets.Size() > 0)
		{
			VpnPacketBuffer buffer = packets.RemoveAtBegin();	
			encapulatedPackets.Append(buffer);
		}
	}

	void VpnPlugin::Decapsulate(VpnChannel const& channel, VpnPacketBuffer const& encapBuffer, VpnPacketBufferList const& decapsulatedPackets, VpnPacketBufferList const& /*controlPacketsToSend*/)
	{		
		uint8_t* p2 = encapBuffer.Buffer().data();
		int p2Len = encapBuffer.Buffer().Length();
		

		VpnPacketBuffer buf = channel.GetVpnReceivePacketBuffer();
		buf.Buffer().Length(p2Len);
		uint8_t* p1 = buf.Buffer().data();
		memcpy(p1, p2, p2Len);
		decapsulatedPackets.Append(buf);

	}

	void VpnPlugin::Configure(Windows::Networking::Vpn::VpnChannel const & channel, std::wstring ipv4)
	{		
		std::vector<HostName> IPv4AddrList;
		IPv4AddrList.push_back(HostName(ipv4));

		VpnRouteAssignment route;
		auto IPv4Routes = route.Ipv4InclusionRoutes();
		IPv4Routes.Append(VpnRoute(HostName(L"0.0.0.0"), 0));

		std::vector<HostName> dnsServerList;
		dnsServerList.push_back(HostName(L"8.8.8.8"));
		
		VpnDomainNameAssignment assignment;
		assignment.DomainNameList().Append({ L".", VpnDomainNameType::Suffix, dnsServerList, nullptr });

		channel.StartExistingTransports(
			IPv4AddrList,
			nullptr,
			nullptr,
			route,
			assignment,
			1400,
			UINT16_MAX,
			false
		);
	}

}
